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

// =========================================================
// ADDRESSES & CONSTANTS (NSO = Ghidra - 0x100)
// =========================================================
#define ADDR_INIT_ROM_DIR    FIX(0x001F1940) 
#define ADDR_ROM_DIR_PATHS   0x00CDF7C0

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

// Global vector to store validated mod ROM paths
static std::vector<std::string> s_modRomPaths;

void ModLoader::initMod(const std::string& path) {
    std::string configPath = path + "/config.toml";
    nn::fs::FileHandle h;
    
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

HOOK_DEFINE_TRAMPOLINE(InitRomDirectoryPathsHook) {
    static void Callback() {
        Orig(); 
        
        uintptr_t base = exl::util::GetMainModuleInfo().m_Total.m_Start;
        auto romDirectoryPaths = *reinterpret_cast<libcxx_vector**>(base + ADDR_ROM_DIR_PATHS);
        if (!romDirectoryPaths) return;

        // Inject mod ROM paths into the game's internal directory list
        if (!s_modRomPaths.empty()) {
            romDirectoryPaths->insert_front(s_modRomPaths);
        }
    }
};

void ModLoader::init() {
    if (!Config::priorityPaths.empty()) {
        for (auto& path : Config::priorityPaths) {
            std::string modDirectory = Config::modsDirectoryPath + "/" + path;
            nn::fs::DirectoryHandle dh;
            if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, modDirectory.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
                nn::fs::CloseDirectory(dh);
                initMod(modDirectory);
            }
        }
    } 
    else {
        nn::fs::DirectoryHandle dh;
        if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, Config::modsDirectoryPath.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
            int64_t count = 0;
            nn::fs::DirectoryEntry entry;
            std::vector<std::string> discoveredMods;
            while (R_SUCCEEDED(nn::fs::ReadDirectory(&count, &entry, dh, 1)) && count > 0) {
                if ((int)entry.m_Type == (int)nn::fs::DirectoryEntryType_Directory) {
                    discoveredMods.push_back(entry.m_Name);
                }
            }
            nn::fs::CloseDirectory(dh);
            std::sort(discoveredMods.begin(), discoveredMods.end());
            for (const auto& modName : discoveredMods) {
                initMod(Config::modsDirectoryPath + "/" + modName);
            }
        }
    }

    // Identify which mods contain valid ROM subfolders
    for (auto& modDir : modDirectoryPaths) {
        std::string checkPath = modDir + "/rom";
        size_t pos;
        while ((pos = checkPath.find("//")) != std::string::npos) checkPath.replace(pos, 2, "/");
        if (!checkPath.empty() && checkPath.back() == '/') checkPath.pop_back();
        
        nn::fs::DirectoryHandle dh;
        if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, checkPath.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
            nn::fs::CloseDirectory(dh);
            s_modRomPaths.push_back(modDir);
        }
    }

    // Late-bind mod prefixes to the captured MdataMgr instance
    if (!s_modRomPaths.empty()) {
        DatabaseLoader::initMdataMgr(s_modRomPaths);
    }

    if (!modDirectoryPaths.empty()) {
        InitRomDirectoryPathsHook::InstallAtOffset(ADDR_INIT_ROM_DIR);
    }
}