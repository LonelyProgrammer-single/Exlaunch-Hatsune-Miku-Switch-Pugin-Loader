#include "ModLoader.hpp"
#include "DatabaseLoader.hpp"
#include "Config.hpp"
#include "toml.hpp"
#include "lib.hpp"
#include "fs.hpp"
#include "patches.hpp"
#include "Allocator.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>

#define ADDR_INIT_ROM_DIR    FIX(0x001F1940) 
#define ADDR_ROM_DIR_PATHS   0x00CDF7C0

/**
 * Re-implementation of libc++ std::string for memory-safe interaction 
 * with the game's internal data structures.
 */
struct libcxx_string {
    union {
        struct { uint64_t cap; uint64_t size; char* data; } l;
        struct { unsigned char size_flag; char data[23]; } s;
    } u;

    bool is_long() const { return u.s.size_flag & 1; }
    const char* c_str() const { return is_long() ? u.l.data : u.s.data; }
    
    void assign(const char* str, size_t len) {
        if (is_long()) GameOperatorDelete(u.l.data);
        
        if (len < 23) {
            u.s.size_flag = static_cast<unsigned char>(len << 1);
            if (len > 0) std::memcpy(u.s.data, str, len);
            u.s.data[len] = 0;
        } else {
            size_t capacity = (len + 16) & ~15;
            u.l.data = (char*)GameOperatorNew(capacity);
            u.l.size = len;
            u.l.cap = capacity | 1; 
            std::memcpy(u.l.data, str, len);
            u.l.data[len] = 0;
        }
    }
};

/**
 * Re-implementation of libc++ std::vector for memory-safe interaction 
 * with the game's internal data structures.
 */
struct libcxx_vector {
    libcxx_string* begin_;
    libcxx_string* end_;
    libcxx_string* cap_;

    size_t size() const { return end_ - begin_; }
    size_t capacity() const { return cap_ - begin_; }

    void insert_front(const std::vector<std::string>& strings) {
        size_t count = strings.size();
        if (count == 0) return;

        size_t old_size = size();
        size_t new_size = old_size + count;
        
        if (new_size > capacity()) {
            size_t new_cap = capacity() * 2;
            if (new_cap < new_size) new_cap = new_size;
            
            libcxx_string* new_buffer = (libcxx_string*)GameOperatorNew(new_cap * sizeof(libcxx_string));
            std::memset(new_buffer, 0, new_cap * sizeof(libcxx_string));
            
            for (size_t i = 0; i < old_size; ++i) {
                std::memcpy(&new_buffer[i + count], &begin_[i], sizeof(libcxx_string));
            }
            
            begin_ = new_buffer;
            end_ = new_buffer + new_size;
            cap_ = new_buffer + new_cap;
        } else {
            std::memmove(&begin_[count], &begin_[0], old_size * sizeof(libcxx_string));
            end_ += count;
        }
        
        for (size_t i = 0; i < count; ++i) {
            begin_[i].u.s.size_flag = 0; 
            begin_[i].assign(strings[i].c_str(), strings[i].length());
        }
    }
};

std::vector<std::string> ModLoader::modDirectoryPaths;
static std::vector<std::string> s_modRomPaths;

void ModLoader::initMod(const std::string& path) {
    std::string configPath = path + "/config.toml";
    nn::fs::FileHandle h;
    
    // If no mod-specific config exists, treat the mod as enabled by default
    if (R_FAILED(nn::fs::OpenFile(&h, configPath.c_str(), nn::fs::OpenMode_Read))) {
        modDirectoryPaths.push_back(path);
        return; 
    }

    int64_t size = 0;
    nn::fs::GetFileSize(&size, h);
    std::string content(size, '\0');
    nn::fs::ReadFile(h, 0, content.data(), size);
    nn::fs::CloseFile(h);

    toml::parse_result result = toml::parse(content);
    if (!result) return;
    
    toml::table config = std::move(result).table();
    if (!config["enabled"].value_or(true)) return;

    // Handle "include" array if present
    if (auto includeArr = config["include"].as_array()) {
        for (auto& elem : *includeArr) {
            std::string sub = elem.value_or("");
            if (!sub.empty()) {
                if (sub == ".") modDirectoryPaths.push_back(path);
                else modDirectoryPaths.push_back(path + "/" + sub);
            }
        }
    } else {
        modDirectoryPaths.push_back(path);
    }
}

/**
 * Hook to inject our mod paths into the game's internal RomFS search list.
 */
HOOK_DEFINE_TRAMPOLINE(InitRomDirectoryPathsHook) {
    static void Callback() {
        Orig(); 
        uintptr_t base = exl::util::GetMainModuleInfo().m_Total.m_Start;
        auto romDirectoryPaths = *reinterpret_cast<libcxx_vector**>(base + ADDR_ROM_DIR_PATHS);
        
        // Inject mod paths at the beginning of the list for highest priority
        if (romDirectoryPaths && !s_modRomPaths.empty()) {
            romDirectoryPaths->insert_front(s_modRomPaths);
        }
    }
};

void ModLoader::init() {
    if (Config::modsDirectoryPath.empty()) return;

    // 1. Process mods based on the synchronized priority list
    for (const auto& modName : Config::priorityPaths) {
        std::string modDirectory = Config::modsDirectoryPath + "/" + modName;
        nn::fs::DirectoryHandle dh;
        if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, modDirectory.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
            nn::fs::CloseDirectory(dh);
            initMod(modDirectory);
        }
    }

    // 2. Convert physical SD paths to virtual RomFS paths for optimized loading
    for (auto& modDir : modDirectoryPaths) {
        std::string checkPath = modDir + "/rom";
        
        nn::fs::DirectoryHandle dh;
        if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, checkPath.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
            nn::fs::CloseDirectory(dh);
            
            // Convert "ExlSD:/atmosphere/.../romfs/mods/ModName" to "rom:/mods/ModName"
            // This triggers Atmosphere's LayeredFS RAM caching for maximum speed.
            std::string virtualPath = modDir;
            size_t romfsPos = virtualPath.find("/romfs/");
            if (romfsPos != std::string::npos) {
                virtualPath = "rom:/" + virtualPath.substr(romfsPos + 7);
            }
            s_modRomPaths.push_back(virtualPath);
        }
    }

    // Initialize mod prefixes for the Database (MdataMgr)
    if (!s_modRomPaths.empty()) {
        DatabaseLoader::initMdataMgr(s_modRomPaths);
        InitRomDirectoryPathsHook::InstallAtOffset(ADDR_INIT_ROM_DIR);
    }
}