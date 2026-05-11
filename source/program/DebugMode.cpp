#include "DebugMode.hpp"
#include "Config.hpp"
#include "patches.hpp"
#include "fs.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <algorithm>
#include <nn/os.hpp>
#include "MotionImGui.hpp"
#include "InputOverlay.hpp"

// =========================================================
// ADDRESSES & CONSTANTS (NSO = Ghidra - 0x100)
// =========================================================
#define ADDR_CHANGE_GAME_STATE      FIX(0x00217010)
#define ADDR_CHANGE_GAME_SUB_STATE  FIX(0x00216810)
#define ADDR_ENGINE_UPDATE_TICK     FIX(0x00201010)
#define ADDR_GET_INPUT_STATE        FIX(0x001FD580)
#define ADDR_BOUNDING_BOX           FIX(0x00341BB0)

#define ADDR_PATCH_GUI_DRAW         FIX(0x00201658)
#define ADDR_PATCH_SHOW_SPRITES_1   FIX(0x0041EC10)
#define ADDR_PATCH_SHOW_SPRITES_2   FIX(0x0041EC30)
#define ADDR_PATCH_NOP_1            FIX(0x003354AC)
#define ADDR_PATCH_NOP_2            FIX(0x003354C4)

namespace DebugMode {

    bool g_DebugModeEnabled = false;

    // Safe transition queue (PC delay simulation)
    static GameState g_targetState = GameState::MAX;
    static GameSubState g_targetSubState = GameSubState::MAX;
    static int g_transitionTimer = 0;

    typedef void (*ChangeGameStateT)(int32_t state);
    typedef void (*ChangeGameSubStateT)(uint32_t state, int32_t substate);
    typedef DivaInputState* (*GetInputStateT)(uint32_t player);

    void ChangeGameState(GameState state) {
        auto func = (ChangeGameStateT)(exl::util::GetMainModuleInfo().m_Total.m_Start + ADDR_CHANGE_GAME_STATE);
        func(static_cast<int32_t>(state));
    }

    void ChangeGameSubState(GameState state, GameSubState substate) {
        auto func = (ChangeGameSubStateT)(exl::util::GetMainModuleInfo().m_Total.m_Start + ADDR_CHANGE_GAME_SUB_STATE);
        func(static_cast<uint32_t>(state), static_cast<int32_t>(substate));
    }

    void RequestStateChange(GameState state, GameSubState substate) {
        ChangeGameState(GameState::DATA_TEST);
        g_targetState = state;
        g_targetSubState = substate;
        g_transitionTimer = 15;
    }

    void ToggleDebugModePatches() {
        if (g_DebugModeEnabled) {
            exl::patch::CodePatcher(ADDR_PATCH_GUI_DRAW).Write<uint32_t>(0x2A0003F3);
            exl::patch::CodePatcher(ADDR_PATCH_SHOW_SPRITES_1).Write<uint32_t>(0x1400000B);
            exl::patch::CodePatcher(ADDR_PATCH_SHOW_SPRITES_2).Write<uint32_t>(0x14000003);
            exl::patch::CodePatcher(ADDR_PATCH_NOP_1).Write<uint32_t>(0xD503201F);
            exl::patch::CodePatcher(ADDR_PATCH_NOP_2).Write<uint32_t>(0xD503201F);
        } else {
            exl::patch::CodePatcher(ADDR_PATCH_GUI_DRAW).Write<uint32_t>(0x320007E0);
            exl::patch::CodePatcher(ADDR_PATCH_SHOW_SPRITES_1).Write<uint32_t>(0x36000160);
            exl::patch::CodePatcher(ADDR_PATCH_SHOW_SPRITES_2).Write<uint32_t>(0x36000060);
            exl::patch::CodePatcher(ADDR_PATCH_NOP_1).Write<uint32_t>(0x360000A0);
            exl::patch::CodePatcher(ADDR_PATCH_NOP_2).Write<uint32_t>(0x360000A0);
        }
    }

    // Read commands from Tesla menu overlay
    void CheckOverlayCommands() {
        const char* cmdPath = "ExlSD:/DMLSwitchPort/diva_state_cmd.bin";
        nn::fs::FileHandle h;
        if (R_FAILED(nn::fs::OpenFile(&h, cmdPath, nn::fs::OpenMode_Read))) return;

        int32_t cmd[2];
        if (R_SUCCEEDED(nn::fs::ReadFile(h, 0, cmd, sizeof(cmd)))) {
            nn::fs::CloseFile(h);
            nn::fs::DeleteFile(cmdPath);
            RequestStateChange(static_cast<GameState>(cmd[0]), static_cast<GameSubState>(cmd[1]));
        } else {
            nn::fs::CloseFile(h);
        }
    }

    // Read commands for emulators (Direct TXT parsing)
    void CheckEmulatorCommands() {
        const char* txtPath = "ExlSD:/DMLSwitchPort/emu_states.txt";
        nn::fs::FileHandle h;

        // Auto-generate the instruction file if it doesn't exist
        if (R_FAILED(nn::fs::OpenFile(&h, txtPath, nn::fs::OpenMode_Read))) {
            const char* defText =
                "# =========================================\n"
                "# DEBUG UI TOGGLES (EMULATOR VERSION)\n"
                "# =========================================\n"
                "# Change 0 to 1 to enable, 1 to 0 to disable.\n"
                "# Save the file to apply changes instantly.\n"
                "# -----------------------------------------\n"
                "ImGUI_Debug_Window = 0\n"
                "InputOverlay = 0\n"
                "\n"
                "# =========================================\n"
                "# DIVA SCENE SWITCHER\n"
                "# =========================================\n"
                "# Set 'Trigger' to 1 and save to jump to scene.\n"
                "# Format: StID SubID Trigger # Name\n"
                "# -----------------------------------------\n"
                "# STARTUP\n"
                "0 0 0 # 0: DATA_INITIALIZE\n"
                "0 1 0 # 1: SYSTEM_STARTUP\n\n"
                "# BASE / MENU_SWITCH\n"
                "9 2 0 # 2: LOGO\n"
                "9 3 0 # 3: TITLE\n"
                "9 4 0 # 4: CONCEAL\n"
                "9 5 0 # 5: GAME\n"
                "9 6 0 # 6: PLAYLIST\n"
                "9 7 0 # 7: CAPTURE\n"
                "9 8 0 # 8: CS_GALLERY (State 9)\n\n"
                "# DATA_TEST (DEBUG SCENES)\n"
                "3 8 0 # 8: DT_MAIN (State 3)\n"
                "3 9 0 # 9: DT_MISC\n"
                "3 10 0 # 10: DT_OBJ\n"
                "3 11 0 # 11: DT_STG\n"
                "3 12 0 # 12: DT_MOT\n"
                "3 13 0 # 13: DT_COLLISION\n"
                "3 14 0 # 14: DT_SPR\n"
                "3 15 0 # 15: DT_AET\n"
                "3 16 0 # 16: DT_AUTH3D\n"
                "3 17 0 # 17: DT_CHR\n"
                "3 18 0 # 18: DT_ITEM\n"
                "3 19 0 # 19: DT_PERF\n"
                "3 20 0 # 20: DT_PVSCRIPT\n"
                "3 21 0 # 21: DT_PRINT\n"
                "3 22 0 # 22: DT_CARD\n"
                "3 23 0 # 23: DT_OPD\n"
                "3 24 0 # 24: DT_SLIDER\n"
                "3 25 0 # 25: DT_GLITTER\n"
                "3 26 0 # 26: DT_GRAPHICS\n"
                "3 27 0 # 27: DT_COL_CARD\n"
                "3 28 0 # 28: DT_PAD\n\n"
                "# AFT TEST MODE & ERROR\n"
                "4 29 0 # 29: TEST_MODE\n"
                "5 30 0 # 30: APP_ERROR\n"
                "3 31 0 # 31: UNK_31\n\n"
                "# CS_MENU & SWITCH UI\n"
                "6 32 0 # 32: CS_MENU\n"
                "6 33 0 # 33: CS_COMMERCE\n"
                "6 34 0 # 34: CS_OPTION_MENU\n"
                "6 35 0 # 35: CS_TUTORIAL\n"
                "6 36 0 # 36: CS_CUSTOMIZE_SEL\n"
                "6 37 0 # 37: CS_TUTORIAL_37\n"
                "6 38 0 # 38: CS_GALLERY_ST38\n"
                "6 39 0 # 39: UNK_39\n"
                "6 40 0 # 40: UNK_40\n"
                "6 41 0 # 41: UNK_41\n"
                "6 42 0 # 42: MENU_SWITCH_UI\n"
                "6 43 0 # 43: UNK_43\n"
                "6 44 0 # 44: OPTION_MENU_UI\n"
                "6 45 0 # 45: UNK_45\n"
                "6 46 0 # 46: UNK_46\n";

            nn::fs::CreateFile(txtPath, strlen(defText));
            if (R_SUCCEEDED(nn::fs::OpenFile(&h, txtPath, nn::fs::OpenMode_Write))) {
                nn::fs::WriteFile(h, 0, defText, strlen(defText), nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
                nn::fs::CloseFile(h);
            }
            return;
        }

        int64_t size = 0;
        nn::fs::GetFileSize(&size, h);
        if (size <= 0 || size > 8192) {
            nn::fs::CloseFile(h);
            return;
        }

        std::string content(size, '\0');
        nn::fs::ReadFile(h, 0, content.data(), size);
        nn::fs::CloseFile(h);

        bool changed = false;
        std::string newContent;
        size_t pos = 0;

        // Track changes to avoid disrupting window focus every frame
        static int s_lastImGui = -1;
        static int s_lastOverlay = -1;

        while (pos < content.length()) {
            size_t endLine = content.find('\n', pos);
            if (endLine == std::string::npos) endLine = content.length();

            std::string line = content.substr(pos, endLine - pos);

            if (!line.empty() && line[0] != '#' && line[0] != '\r') {
                int toggleVal = 0;

                // 1. Read ImGui toggle
                if (sscanf(line.c_str(), "ImGUI_Debug_Window = %d", &toggleVal) == 1) {
                    if (s_lastImGui == -1) s_lastImGui = toggleVal;
                    else if (s_lastImGui != toggleVal) {
                        MotionImGui::g_isMenuOpen = (toggleVal != 0);
                        MotionImGui::g_imguiHasFocus = MotionImGui::g_isMenuOpen;
                        s_lastImGui = toggleVal;
                    }
                }
                // 2. Read Gamepad Overlay toggle
                else if (sscanf(line.c_str(), "InputOverlay = %d", &toggleVal) == 1) {
                    if (s_lastOverlay == -1) s_lastOverlay = toggleVal;
                    else if (s_lastOverlay != toggleVal) {
                        InputOverlay::SetVisible(toggleVal != 0);
                        s_lastOverlay = toggleVal;
                    }
                }
                // 3. Read scene change triggers
                else {
                    int st = -1, sub = -1, trigger = 0;
                    if (sscanf(line.c_str(), "%d %d %d", &st, &sub, &trigger) == 3) {
                        if (trigger == 1) {
                            RequestStateChange(static_cast<GameState>(st), static_cast<GameSubState>(sub));

                            // Reset trigger back to 0
                            char buf[64];
                            snprintf(buf, sizeof(buf), "%d %d 0", st, sub);

                            size_t commentPos = line.find('#');
                            if (commentPos != std::string::npos) {
                                line = std::string(buf) + " " + line.substr(commentPos);
                            } else {
                                line = std::string(buf);
                            }
                            changed = true;
                        }
                    }
                }
            }
            newContent += line + "\n";
            pos = endLine + 1;
        }

        // If a scene trigger was reset, overwrite the file
        if (changed) {
            nn::fs::DeleteFile(txtPath);
            nn::fs::CreateFile(txtPath, newContent.length());
            if (R_SUCCEEDED(nn::fs::OpenFile(&h, txtPath, nn::fs::OpenMode_Write))) {
                nn::fs::WriteFile(h, 0, newContent.c_str(), newContent.length(), nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
                nn::fs::CloseFile(h);
            }
        }
    }

    void ProcessDebugInputs() {
        if (MotionImGui::g_isMenuOpen && MotionImGui::g_imguiHasFocus) {
            return;
        }

        nn::hid::NpadHandheldState mergedState = nn::hid::GetMergedNpadState();
        uint64_t keysHeld = mergedState.buttons;

        static uint64_t s_prevKeys = 0;
        uint64_t keysDown = keysHeld & ~s_prevKeys;
        s_prevKeys = keysHeld;

        // Toggle debug mode (L + R + Plus)
        if ((keysHeld & nn::hid::Button::L) && (keysHeld & nn::hid::Button::R) && (keysDown & nn::hid::Button::Plus)) {
            g_DebugModeEnabled = !g_DebugModeEnabled;
            ToggleDebugModePatches();
        }

        if (!g_DebugModeEnabled) return;

        // D-Pad Hotkeys (Safe transitions)
        if ((keysHeld & nn::hid::Button::L) && (keysHeld & nn::hid::Button::R)) {
            if (keysDown & nn::hid::Button::Down)   { ChangeGameState(GameState::DATA_TEST); }
            if (keysDown & nn::hid::Button::Up)     { ChangeGameState(GameState::TEST_MODE); }
            if (keysDown & nn::hid::Button::Right)  { ChangeGameState(GameState::MENU_SWITCH); }
            if (keysDown & nn::hid::Button::Left)   { RequestStateChange(GameState::CS_MENU, static_cast<GameSubState>(32)); }
        }

        GetInputStateT GetInput = (GetInputStateT)(exl::util::GetMainModuleInfo().m_Total.m_Start + ADDR_GET_INPUT_STATE);
        DivaInputState* dis = GetInput(0);
        if (!dis) return;

        // Toggle mouse input
        static bool s_mouseEnabled = false;
        if (keysDown & nn::hid::Button::LStick) { s_mouseEnabled = !s_mouseEnabled; }

        if (keysHeld & nn::hid::Button::RStick) {
            dis->Key = 0x11; // VK_CONTROL

            if (!s_mouseEnabled) {
                struct SceneEntry { int32_t state; int32_t sub; };
                static const SceneEntry SCENES[] = {
                    {0, 0}, {0, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5}, {9, 6}, {9, 7},
                    {9, 8}, {3, 8}, {3, 9}, {3, 10}, {3, 11}, {3, 12}, {3, 13}, {3, 14}, {3, 15}, {3, 16},
                    {3, 17}, {3, 18}, {3, 19}, {3, 20}, {3, 21}, {3, 22}, {3, 23}, {3, 24},
                    {3, 25}, {3, 26}, {3, 27}, {3, 28}, {4, 29}, {5, 30}, {3, 31}, {6, 32},
                    {6, 33}, {6, 34}, {6, 35}, {6, 36}, {6, 37}, {6, 38}, {6, 39}, {6, 40},
                    {6, 41}, {6, 42}, {6, 43}, {6, 44}, {6, 45}, {6, 46}
                };
                constexpr int SCENE_COUNT = sizeof(SCENES) / sizeof(SCENES[0]);

                static int32_t s_SelectedIdx = 9;

                if (keysDown & nn::hid::Button::Down) { s_SelectedIdx = (s_SelectedIdx + 1) % SCENE_COUNT; }
                if (keysDown & nn::hid::Button::Up)   { s_SelectedIdx = (s_SelectedIdx - 1 + SCENE_COUNT) % SCENE_COUNT; }
                if (keysDown & nn::hid::Button::A) {
                    RequestStateChange(static_cast<GameState>(SCENES[s_SelectedIdx].state),
                                       static_cast<GameSubState>(SCENES[s_SelectedIdx].sub));
                }
            }
        } else {
            dis->Key = 0;
        }

        if (s_mouseEnabled) {
            bool block = (keysHeld & nn::hid::Button::L) || (keysHeld & nn::hid::Button::R);
            bool isZL = (keysHeld & nn::hid::Button::ZL) && !block;
            bool isZR = (keysHeld & nn::hid::Button::ZR) && !block;

            static uint64_t lastTick = 0;
            uint64_t currentTick = nn::os::GetSystemTick().GetInt64Value();
            float dt = 1.0f / 60.0f;
            if (lastTick != 0) {
                uint64_t freq = nn::os::GetSystemTickFrequency();
                dt = (float)(currentTick - lastTick) / (float)freq;
                if (dt <= 0.0f || dt > 0.1f) dt = 1.0f / 60.0f;
            }
            lastTick = currentTick;

            float frameComp = dt * 60.0f;

            // 1. Joystick
            float lx = (float)mergedState.analogStickL[0] / 32767.0f;
            float ly = (float)mergedState.analogStickL[1] / 32767.0f;

            float speed = (keysHeld & nn::hid::Button::Y) ? 12.0f : 5.0f;

            if (std::abs(lx) > 0.15f) dis->MouseX += (int32_t)(speed * lx * frameComp);
            if (std::abs(ly) > 0.15f) dis->MouseY -= (int32_t)(speed * ly * frameComp);

            // 2. USB Mouse
            if (nn::hid::GetMouseState) {
                nn::hid::MouseState ms = {};
                nn::hid::GetMouseState(&ms);
                dis->MouseX += ms.deltaX;
                dis->MouseY += ms.deltaY;
                if (ms.buttons & 1) isZL = true;
                if (ms.buttons & 2) isZR = true;
            }

            // 3. Touch Screen
            if (nn::hid::GetTouchScreenStates) {
                nn::hid::TouchScreenState ts = {};
                nn::hid::GetTouchScreenStates(&ts, 1);
                if (ts.count > 0) {
                    dis->MouseX = (int32_t)(ts.touches[0].x * (1920.0f / 1280.0f));
                    dis->MouseY = (int32_t)(ts.touches[0].y * (1080.0f / 720.0f));
                    isZL = true;
                }
            }

            dis->MouseX = std::clamp(dis->MouseX, 0, 1919);
            dis->MouseY = std::clamp(dis->MouseY, 0, 1079);

            static bool s_zlPressed = false;
            static bool s_zrPressed = false;

            if (isZL) {
                dis->SetBit(100, !s_zlPressed, DivaInputState::Type_Tapped);
                dis->SetBit(100, true, DivaInputState::Type_Down);
                dis->SetBit(100, false, DivaInputState::Type_Released);
                s_zlPressed = true;
            } else {
                dis->SetBit(100, false, DivaInputState::Type_Tapped);
                dis->SetBit(100, false, DivaInputState::Type_Down);
                dis->SetBit(100, s_zlPressed, DivaInputState::Type_Released);
                s_zlPressed = false;
            }

            if (isZR) {
                dis->SetBit(102, !s_zrPressed, DivaInputState::Type_Tapped);
                dis->SetBit(102, true, DivaInputState::Type_Down);
                dis->SetBit(102, false, DivaInputState::Type_Released);
                s_zrPressed = true;
            } else {
                dis->SetBit(102, false, DivaInputState::Type_Tapped);
                dis->SetBit(102, false, DivaInputState::Type_Down);
                dis->SetBit(102, s_zrPressed, DivaInputState::Type_Released);
                s_zrPressed = false;
            }
        }
    }

    struct BoundingBox { float x, y, w, h; };

    HOOK_DEFINE_TRAMPOLINE(DwScrollableGetBoundingBoxHook) {
        static BoundingBox Callback(void* this_ptr) {
            if (!g_DebugModeEnabled) return Orig(this_ptr);

            uintptr_t a1 = (uintptr_t)this_ptr;
            float v2_orig = *(float*)(a1 + 0x40);
            float v3_orig = *(float*)(a1 + 0x44);
            bool v4_flag = (*(uint8_t*)(a1 + 0x51) & 0x08) == 0;

            BoundingBox box = { 0.0f, 0.0f, v2_orig, v3_orig };

            if (!v4_flag) {
                box.x = 2.0f;
                box.y = 2.0f;
                box.w = v2_orig - 4.0f;
                box.h = v3_orig - 4.0f;
            }

            uint64_t v5_parent = *(uint64_t*)(a1 + 0xD8);
            if (v5_parent) {
                box.w -= *(float*)(v5_parent + 0x40);
            }
            return box;
        }
    };

    HOOK_DEFINE_TRAMPOLINE(EngineUpdateTickHook) {
        static void Callback(void* arg0) {
            static int fsTimer = 0;
            if (++fsTimer >= 15) {
                CheckOverlayCommands();
                CheckEmulatorCommands();
                MotionImGui::CheckToggles();
                InputOverlay::CheckToggles();
                fsTimer = 0;
            }

            Orig(arg0);

            if (g_transitionTimer > 0) {
                g_transitionTimer--;
                if (g_transitionTimer == 0 && g_targetState != GameState::MAX) {
                    ChangeGameSubState(g_targetState, g_targetSubState);
                    g_targetState = GameState::MAX;
                    g_targetSubState = GameSubState::MAX;
                }
            } else {
                ProcessDebugInputs();
            }
        }
    };

    void Init() {
        if (!Config::enableDebug) return;

        DwScrollableGetBoundingBoxHook::InstallAtOffset(ADDR_BOUNDING_BOX);
        EngineUpdateTickHook::InstallAtOffset(ADDR_ENGINE_UPDATE_TICK);

        exl::patch::CodePatcher(FIX(0x002F8370)).Write<uint32_t>(0x7144011F);
        exl::patch::CodePatcher(FIX(0x0028DB18)).Write<uint32_t>(0x7144007F);
        exl::patch::CodePatcher(FIX(0x0028DB6C)).Write<uint32_t>(0x7144007F);
        exl::patch::CodePatcher(FIX(0x0028E2F4)).Write<uint32_t>(0x7144007F);
        exl::patch::CodePatcher(FIX(0x00294D10)).Write<uint32_t>(0x7144011F);
        exl::patch::CodePatcher(FIX(0x002FEB80)).Write<uint32_t>(0x714402DF);
        exl::patch::CodePatcher(FIX(0x003680D0)).Write<uint32_t>(0xF14402BF);
        exl::patch::CodePatcher(FIX(0x00316EB0)).Write<uint32_t>(0x92800003);
        exl::patch::CodePatcher(FIX(0x00316F4C)).Write<uint32_t>(0x92800003);
        exl::patch::CodePatcher(FIX(0x002F8504)).Write<uint32_t>(0x92800003);
    }
}