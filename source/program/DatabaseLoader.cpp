#include "DatabaseLoader.hpp"
#include "lib.hpp"
#include "fs.hpp"
#include "patches.hpp"
#include "Allocator.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#define ADDR_RESOLVE_FILE_PATH FIX(0x001F4580)
#define ADDR_INIT_MDATA_MGR    FIX(0x0041AFC0)

static void* g_mdataMgrPtr = nullptr;

/**
 * Performance Cache: Prevents excessive SD card polling.
 * First check hits the SD card, subsequent checks hit RAM.
 */
static std::unordered_map<std::string, bool> g_dbCache;
static std::recursive_mutex g_cacheMtx;

bool FastFileExists(const std::string& path) {
    {
        std::lock_guard<std::recursive_mutex> lock(g_cacheMtx);
        auto it = g_dbCache.find(path);
        if (it != g_dbCache.end()) return it->second;
    }

    nn::fs::FileHandle h;
    bool exists = false;
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, path.c_str(), nn::fs::OpenMode_Read))) {
        nn::fs::CloseFile(h);
        exists = true;
    }

    std::lock_guard<std::recursive_mutex> lock(g_cacheMtx);
    g_dbCache[path] = exists;
    return exists;
}

/**
 * Re-implementation of libc++ std::string for hooking game internals.
 */
struct libcxx_string {
    union {
        struct { uint64_t cap; uint64_t size; char* data; } l;
        struct { unsigned char size_flag; char data[23]; } s;
    } u;

    bool is_long() const { return u.s.size_flag & 1; }
    const char* c_str() const { return is_long() ? u.l.data : u.s.data; }
    size_t length() const { return is_long() ? u.l.size : (u.s.size_flag >> 1); }

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
 * Re-implementation of libc++ std::list for hooking game internals.
 */
struct libcxx_list_node {
    libcxx_list_node* prev; 
    libcxx_list_node* next; 
    libcxx_string value;    
};

struct libcxx_list {
    libcxx_list_node* end_prev; 
    libcxx_list_node* end_next; 
    size_t size;                
    
    void push_back(const char* str) {
        size_t len = std::strlen(str);
        libcxx_list_node* node = (libcxx_list_node*)GameOperatorNew(sizeof(libcxx_list_node));
        
        std::memset(node, 0, sizeof(libcxx_list_node));
        node->value.u.s.size_flag = 0;
        node->value.assign(str, len);

        libcxx_list_node* end_node = reinterpret_cast<libcxx_list_node*>(this); 
        libcxx_list_node* last = this->end_prev; 
        
        node->prev = last;
        node->next = end_node;
        
        last->next = node;
        this->end_prev = node; 
        
        this->size++;
    }
};

constexpr char MAGIC = 0x01;

/**
 * Resolves the path for mod-specific database files (e.g., song_db.mod.txt).
 */
bool resolveModDatabaseFilePath(const libcxx_string& filePath, std::string& destFilePath) {
    std::string pathStr = filePath.c_str();
    const size_t magicIdx0 = pathStr.find(MAGIC);
    if (magicIdx0 == std::string::npos) return false;

    const size_t magicIdx1 = pathStr.find(MAGIC, magicIdx0 + 1);
    if (magicIdx1 == std::string::npos) return false;

    const std::string left = pathStr.substr(0, magicIdx0);
    const std::string center = pathStr.substr(magicIdx0 + 1, magicIdx1 - magicIdx0 - 1);
    const std::string right = pathStr.substr(magicIdx1 + 1);

    destFilePath = center + "/" + left + "mod" + right;

    size_t pos;
    while ((pos = destFilePath.find("//")) != std::string::npos) {
        destFilePath.replace(pos, 2, "/");
    }

    return true;
}

/**
 * Hook to override the game's file resolution logic to prioritize modded files.
 */
HOOK_DEFINE_TRAMPOLINE(ResolveFilePathObserverHook) {
    static uint64_t Callback(libcxx_string* filePath, libcxx_string* destFilePath) {
        std::string destPathTmp;

        if (filePath && resolveModDatabaseFilePath(*filePath, destPathTmp)) {
            // Check performance cache before hitting physical SD
            if (FastFileExists(destPathTmp)) {
                if (destFilePath) {
                    destFilePath->assign(destPathTmp.c_str(), destPathTmp.length());
                }
                return 1; 
            }
            return 0; 
        }

        return Orig(filePath, destFilePath);
    }
};

HOOK_DEFINE_TRAMPOLINE(InitMdataMgrHook) {
    static void Callback(void* this_ptr) {
        Orig(this_ptr);
        g_mdataMgrPtr = this_ptr;
    }
};

void DatabaseLoader::init() {
    ResolveFilePathObserverHook::InstallAtOffset(ADDR_RESOLVE_FILE_PATH);
    InitMdataMgrHook::InstallAtOffset(ADDR_INIT_MDATA_MGR);
}

void DatabaseLoader::initMdataMgr(const std::vector<std::string>& modRomDirectoryPaths) {
    if (!g_mdataMgrPtr) return; 
    
    auto list = reinterpret_cast<libcxx_list*>((uintptr_t)g_mdataMgrPtr + 0x178);
    
    // Inject mod directory markers using the special MAGIC character
    for (auto it = modRomDirectoryPaths.rbegin(); it != modRomDirectoryPaths.rend(); ++it) {
        std::string path;
        path += MAGIC; path += *it; path += MAGIC; path += "_";
        list->push_back(path.c_str());
    }
}