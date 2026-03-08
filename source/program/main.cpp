#include "lib.hpp"
#include "fs.hpp"
#include "ScoreData.hpp" 
#include "StrArray.hpp"
#include <map>
#include <deque>
#include <set>
#include <mutex>
#include <cstring>
#include <vector>
#include "patches.hpp"
#include "SpriteLoader.hpp"
#include "Config.hpp"
#include "ModLoader.hpp"
#include "DatabaseLoader.hpp"

// =========================================================
// ADDRESSES & CONSTANTS (NSO = Ghidra - 0x100)
// =========================================================

#define SCORE_SIZE          0x11F4
#define MOUNT_NAME          "ExlSD"
#define SAVE_PATH           "ExlSD:/DMLSwitchPort/Save/DivaModData.dat"

// Hook Addresses
#define ADDR_FIND_OR_CREATE    FIX(0x0C62E0)
#define ADDR_FIND_SCORE        FIX(0x0C7990)
#define ADDR_REGISTER_SCORE    FIX(0x0C87F0)
#define ADDR_SAVE_MANAGER      FIX(0x0C9820)
#define ADDR_INIT_BOOT_2       FIX(0x0C5950)
#define ADDR_SYNC_MANAGER      FIX(0x0C6440) 

// New Addresses for Modules and Custom Items
#define ADDR_FIND_MODULE       FIX(0x0C8950) 
#define ADDR_FIND_CSTM_ITEM    FIX(0x0C8970) 
#define ADDR_FIND_CSTM_GALLERY FIX(0x0C8D50)

// =========================================================
// STRUCTURES
// =========================================================
struct Score {
    int32_t pvId;
    uint8_t data[SCORE_SIZE - 4];
};

struct Module {
    uint8_t unknown0;
    uint8_t unknown1;
};

struct ModuleEx {
    uint32_t moduleId;
    Module module;
};

struct CstmItem {
    uint8_t unknown0;
};

struct CstmItemEx {
    uint32_t cstmItemId;
    CstmItem cstmItem;
};

struct SaveDataEx {
    static constexpr uint32_t MAX_VERSION = 1;
    uint32_t version;
    uint32_t headerSize;
    uint32_t scoreCount;
    uint32_t moduleCount;
    uint32_t cstmItemCount;
};

// =========================================================
// GLOBALS
// =========================================================
std::recursive_mutex g_SaveMtx;
std::map<int32_t, Score*> g_scoreMap;
std::deque<Score> g_modPool; 
std::set<int32_t> g_systemSlots;

// New maps for modules and custom items. std::map is safe as it doesn't invalidate pointers upon insertion.
std::map<uint32_t, Module> g_moduleMap;
std::map<uint32_t, CstmItem> g_cstmItemMap;

typedef void (*RegisterScoreT)(int32_t id, void* scorePtr);

// Manually register a song into the game engine
void DoForceRegister(int32_t id, void* ptr) {
    if (id <= 0 || !ptr) return;
    auto RegFunc = (RegisterScoreT)(exl::util::GetMainModuleInfo().m_Total.m_Start + ADDR_REGISTER_SCORE);
    RegFunc(id, ptr);
}

// =========================================================
// FILE SYSTEM
// =========================================================
void LoadSD() {
    std::scoped_lock lock(g_SaveMtx);
    nn::fs::FileHandle h;
    
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, SAVE_PATH, nn::fs::OpenMode_Read))) {
        int64_t sz = 0; 
        nn::fs::GetFileSize(&sz, h);
        
        if (sz > 0) {
            std::vector<uint8_t> buf(sz);
            nn::fs::ReadFile(h, 0, buf.data(), sz);
            
            bool isNewFormat = false;
            if (sz >= (int64_t)sizeof(SaveDataEx)) {
                SaveDataEx* header = reinterpret_cast<SaveDataEx*>(buf.data());
                // Check if this is the new format or the old raw array of scores
                if (header->version == SaveDataEx::MAX_VERSION && header->headerSize == sizeof(SaveDataEx)) {
                    isNewFormat = true;
                    
                    size_t offset = header->headerSize;
                    
                    // Read Scores
                    for (uint32_t i = 0; i < header->scoreCount; i++) {
                        Score s;
                        std::memcpy(&s, buf.data() + offset, sizeof(Score));
                        offset += sizeof(Score);
                        
                        g_modPool.push_back(s);
                        g_scoreMap[s.pvId] = &g_modPool.back();
                        DoForceRegister(s.pvId, g_scoreMap[s.pvId]);
                    }
                    
                    // Read Modules
                    for (uint32_t i = 0; i < header->moduleCount; i++) {
                        ModuleEx m;
                        std::memcpy(&m, buf.data() + offset, sizeof(ModuleEx));
                        offset += sizeof(ModuleEx);
                        g_moduleMap[m.moduleId] = m.module;
                    }
                    
                    // Read CstmItems
                    for (uint32_t i = 0; i < header->cstmItemCount; i++) {
                        CstmItemEx c;
                        std::memcpy(&c, buf.data() + offset, sizeof(CstmItemEx));
                        offset += sizeof(CstmItemEx);
                        g_cstmItemMap[c.cstmItemId] = c.cstmItem;
                    }
                }
            }
            
            // If there's no header, load as old save (DATA MIGRATION)
            if (!isNewFormat) {
                int count = (int)(sz / SCORE_SIZE);
                for (int i = 0; i < count; i++) {
                    Score s;
                    std::memcpy(&s, buf.data() + (i * SCORE_SIZE), SCORE_SIZE);
                    if (s.pvId > 0) {
                        g_modPool.push_back(s);
                        g_scoreMap[s.pvId] = &g_modPool.back();
                        DoForceRegister(s.pvId, g_scoreMap[s.pvId]);
                    }
                }
            }
        }
        nn::fs::CloseFile(h);
    }
}

void SaveSD() {
    std::scoped_lock lock(g_SaveMtx);
    if (g_scoreMap.empty() && g_moduleMap.empty() && g_cstmItemMap.empty()) return;

    // Calculate total file size
    size_t totalSize = sizeof(SaveDataEx) 
                     + (g_scoreMap.size() * sizeof(Score)) 
                     + (g_moduleMap.size() * sizeof(ModuleEx)) 
                     + (g_cstmItemMap.size() * sizeof(CstmItemEx));

    // Assemble everything into a single memory buffer
    std::vector<uint8_t> buffer;
    buffer.reserve(totalSize);
    buffer.resize(sizeof(SaveDataEx));

    // Set up the header
    SaveDataEx* header = reinterpret_cast<SaveDataEx*>(buffer.data());
    header->version = SaveDataEx::MAX_VERSION;
    header->headerSize = sizeof(SaveDataEx);
    header->scoreCount = static_cast<uint32_t>(g_scoreMap.size());
    header->moduleCount = static_cast<uint32_t>(g_moduleMap.size());
    header->cstmItemCount = static_cast<uint32_t>(g_cstmItemMap.size());

    // Copy Scores
    for (const auto& [id, sPtr] : g_scoreMap) {
        size_t off = buffer.size();
        buffer.resize(off + sizeof(Score));
        std::memcpy(buffer.data() + off, sPtr, sizeof(Score));
    }

    // Copy Modules
    for (const auto& [id, mod] : g_moduleMap) {
        size_t off = buffer.size();
        buffer.resize(off + sizeof(ModuleEx));
        ModuleEx mEx = { id, mod };
        std::memcpy(buffer.data() + off, &mEx, sizeof(ModuleEx));
    }

    // Copy CstmItems
    for (const auto& [id, cstm] : g_cstmItemMap) {
        size_t off = buffer.size();
        buffer.resize(off + sizeof(CstmItemEx));
        CstmItemEx cEx = { id, cstm };
        std::memcpy(buffer.data() + off, &cEx, sizeof(CstmItemEx));
    }

    // Delete the old file and write the new one in one go (VERY FAST!)
    nn::fs::DeleteFile(SAVE_PATH);
    nn::fs::CreateFile(SAVE_PATH, totalSize);
    
    nn::fs::FileHandle h;
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, SAVE_PATH, nn::fs::OpenMode_Write))) {
        nn::fs::WriteFile(h, 0, buffer.data(), totalSize, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
        nn::fs::CloseFile(h);
    }
}

// =========================================================
// HOOKS
// =========================================================

HOOK_DEFINE_TRAMPOLINE(FindOrCreateScoreHook) {
    static void* Callback(void* mgr, int32_t id) {
        if (id <= 0) return Orig(mgr, id);
        std::scoped_lock lock(g_SaveMtx);
        if (g_scoreMap.count(id)) return g_scoreMap[id];
        
        bool useSystem = false;
        if (g_systemSlots.count(id) > 0 || g_systemSlots.size() < 300) {
            useSystem = true;
        }
        if (useSystem) {
            void* res = Orig(mgr, id);
            if (res) {
                g_systemSlots.insert(id);
                return res;
            }
        }

        Score s;
        std::memcpy(&s, EMPTY_SCORE_DATA, SCORE_SIZE);
        s.pvId = id;
        
        g_modPool.push_back(s);
        g_scoreMap[id] = &g_modPool.back();
        
        DoForceRegister(id, g_scoreMap[id]);
        
        return g_scoreMap[id];
    }
};

HOOK_DEFINE_TRAMPOLINE(FindScoreHook) {
    static void* Callback(void* mgr, int32_t id) {
        void* res = Orig(mgr, id);
        if (res || id <= 0) return res;
        
        std::scoped_lock lock(g_SaveMtx);
        return g_scoreMap.count(id) ? g_scoreMap[id] : nullptr;
    }
};

// --- NEW HOOKS FOR MODULES AND ITEMS ---

HOOK_DEFINE_TRAMPOLINE(FindModuleHook) {
    static Module* Callback(void* mgr, uint32_t id) {
        std::scoped_lock lock(g_SaveMtx);
        if (g_moduleMap.count(id)) return &g_moduleMap[id];

        Module* res = Orig(mgr, id);
        if (res == nullptr) {
            auto& mod = g_moduleMap[id];
            mod.unknown0 = 3;
            mod.unknown1 = 0;
            res = &mod;
        }
        return res;
    }
};

HOOK_DEFINE_TRAMPOLINE(FindCstmItemHook) {
    static CstmItem* Callback(void* mgr, uint32_t id) {
        std::scoped_lock lock(g_SaveMtx);
        if (g_cstmItemMap.count(id)) return &g_cstmItemMap[id];

        CstmItem* res = Orig(mgr, id);
        if (res == nullptr) {
            auto& cstm = g_cstmItemMap[id];
            cstm.unknown0 = 3;
            res = &cstm;
        }
        return res;
    }
};

HOOK_DEFINE_TRAMPOLINE(FindCstmItemGalleryHook) {
    static CstmItem* Callback(void* mgr, uint32_t id) {
        std::scoped_lock lock(g_SaveMtx);
        if (g_cstmItemMap.count(id)) return &g_cstmItemMap[id];

        CstmItem* res = Orig(mgr, id);
        if (res == nullptr) {
            auto& cstm = g_cstmItemMap[id];
            cstm.unknown0 = 3;
            res = &cstm;
        }
        return res;
    }
};

HOOK_DEFINE_TRAMPOLINE(SaveManagerHook) {
    static uint64_t Callback(int mode) {
        uint64_t res = Orig(mode);
        if (mode == 0) LoadSD(); 
        else if (mode == 1 || mode == 2) SaveSD();
        return res;
    }
};

HOOK_DEFINE_TRAMPOLINE(InitBoot2Hook) {
    static void Callback(void* arg) {
        Orig(arg);
        std::scoped_lock lock(g_SaveMtx);
        for (auto& [id, s] : g_scoreMap) DoForceRegister(id, s);
    }
};

HOOK_DEFINE_TRAMPOLINE(SyncManagerHook) {
    static void Callback(void* arg) {
        std::scoped_lock lock(g_SaveMtx);
        for (auto&[id, s] : g_scoreMap) {
             DoForceRegister(id, s);
        }
    }
};

// =========================================================
// INITIALIZATION
// =========================================================

extern "C" void nnMain();
HOOK_DEFINE_TRAMPOLINE(MainHook) {
    static void Callback() {
        nn::fs::MountSdCardForDebug(MOUNT_NAME);
        if (Config::init()) {
            ModLoader::init(); 
        }
        Orig(); 
    }
};

extern "C" void exl_main(void* x0, void* x1) {
    exl::hook::Initialize();

    MainHook::InstallAtFuncPtr(nnMain);

    ApplyCustomPatches();
    DatabaseLoader::init();
    StrArray::init();
    SpriteLoader::init();


    FindOrCreateScoreHook::InstallAtOffset(ADDR_FIND_OR_CREATE);
    FindScoreHook::InstallAtOffset(ADDR_FIND_SCORE);
    SaveManagerHook::InstallAtOffset(ADDR_SAVE_MANAGER);
    InitBoot2Hook::InstallAtOffset(ADDR_INIT_BOOT_2);
    SyncManagerHook::InstallAtOffset(ADDR_SYNC_MANAGER);
    
    // Install new hooks
    FindModuleHook::InstallAtOffset(ADDR_FIND_MODULE);
    FindCstmItemHook::InstallAtOffset(ADDR_FIND_CSTM_ITEM);
    FindCstmItemGalleryHook::InstallAtOffset(ADDR_FIND_CSTM_GALLERY);
};
