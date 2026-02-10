#include "lib.hpp"
#include "fs.hpp"
#include "ScoreData.hpp" 
#include <map>
#include <deque>
#include <set>
#include <mutex>
#include <cstring>
#include <cstdarg>
#include "patches.hpp"

// =========================================================
// ADDRESSES & CONSTANTS (NSO = Ghidra - 0x100)
// =========================================================

#define SCORE_SIZE          0x11F4
#define MOUNT_NAME          "ExlSD"
#define LOG_PATH            "ExlSD:/DivaLog.txt"
#define SAVE_PATH           "ExlSD:/DivaModData.dat"

// Hook Addresses
#define ADDR_FIND_OR_CREATE FIX(0x0C62E0)
#define ADDR_FIND_SCORE     FIX(0x0C7990)
#define ADDR_REGISTER_SCORE FIX(0x0C87F0)
#define ADDR_SAVE_MANAGER   FIX(0x0C9820)
#define ADDR_INIT_BOOT_2    FIX(0x0C5950)
#define ADDR_SYNC_MANAGER   FIX(0x0C6440) 


// =========================================================
// GLOBALS
// =========================================================
struct Score {
    int32_t pvId;
    uint8_t data[SCORE_SIZE - 4];
};

std::recursive_mutex g_SaveMtx;
std::map<int32_t, Score*> g_scoreMap;
std::deque<Score> g_modPool; 
std::set<int32_t> g_systemSlots;
bool g_FsReady = false;

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
        int64_t sz = 0; nn::fs::GetFileSize(&sz, h);
        int count = (int)(sz / SCORE_SIZE);
        for (int i = 0; i < count; i++) {
            Score s;
            nn::fs::ReadFile(h, (int64_t)i * SCORE_SIZE, &s, SCORE_SIZE);
            if (s.pvId > 0) {
                g_modPool.push_back(s);
                g_scoreMap[s.pvId] = &g_modPool.back();
                DoForceRegister(s.pvId, g_scoreMap[s.pvId]);
            }
        }
        nn::fs::CloseFile(h);
    }
}

void SaveSD() {
    std::scoped_lock lock(g_SaveMtx);
    if (g_scoreMap.empty()) return;
    nn::fs::DeleteFile(SAVE_PATH);
    nn::fs::CreateFile(SAVE_PATH, (int64_t)g_scoreMap.size() * SCORE_SIZE);
    nn::fs::FileHandle h;
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, SAVE_PATH, nn::fs::OpenMode_Write))) {
        int64_t off = 0;
        for (auto const& [id, s] : g_scoreMap) {
            nn::fs::WriteFile(h, off, s, SCORE_SIZE, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
            off += SCORE_SIZE;
        }
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

// !!! CRITICAL FIX !!!
HOOK_DEFINE_TRAMPOLINE(SyncManagerHook) {
    static void Callback(void* arg) {
        // Orig(arg); <--- REMOVED TO PREVENT CRASH

        std::scoped_lock lock(g_SaveMtx);
        for (auto& [id, s] : g_scoreMap) {
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
        g_FsReady = true;
        Orig(); 
    }
};

extern "C" void exl_main(void* x0, void* x1) {
    exl::hook::Initialize();
    ApplyCustomPatches();  
    MainHook::InstallAtFuncPtr(nnMain);

    FindOrCreateScoreHook::InstallAtOffset(ADDR_FIND_OR_CREATE);
    FindScoreHook::InstallAtOffset(ADDR_FIND_SCORE);
    SaveManagerHook::InstallAtOffset(ADDR_SAVE_MANAGER);
    InitBoot2Hook::InstallAtOffset(ADDR_INIT_BOOT_2);
    SyncManagerHook::InstallAtOffset(ADDR_SYNC_MANAGER);
};
