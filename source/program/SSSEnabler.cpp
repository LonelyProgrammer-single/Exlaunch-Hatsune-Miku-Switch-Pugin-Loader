#include "lib.hpp"
#include "patches.hpp" // FIX()

// old
#define ADDR_SSS_CALL_CHECK FIX(0x004F3604)
// new
#define ADDR_SSS_BIT_GETTER FIX(0x0020B950)
#define ADDR_SSS_BIT_SETTER FIX(0x0020B970)


// beautiful graphics cstm menu ft style (aft/ft comeback)
#define ADDR_NPR_CSTMMENU_KILL FIX(0x003D4674)
#define ADDR_FXAA_CSTMMENU_KILL FIX(0x003D4678)

// useless
#define ADDR_BRANCH_KILL_1 FIX(0x004F3634)
#define ADDR_BRANCH_KILL_2 FIX(0x004F4C88)

#define ADDR_ADP_GETTER FIX(0x0020B4E0)
#define ADDR_ADP_SETTER FIX(0x0020B500)
#define ADDR_ADP_HARDCODE 0x002028A8

// useless
#define ADDR_SSS_BLUR_IF_1 FIX(0x004F36E4)
#define ADDR_SSS_BLUR_IF_2 FIX(0x004F36EC)
#define ADDR_SSS_BLUR_IF_3 FIX(0x004F36F4)

// useless
#define ADDR_SSS_UPD_ZEROED FIX(0x004F376C)
#define ADDR_SSS_UPD_IF FIX(0x004F379C)


// shadow maps
#define ADDR_SHADOW_W1 FIX(0x005E5EEC) // was 0x200 (512)
#define ADDR_SHADOW_H1 FIX(0x005E5EF0) // was 0x200

#define ADDR_SHADOW_W2 FIX(0x005E5F10) // was 0x200
#define ADDR_SHADOW_H2 FIX(0x005E5F14) // was 0x200

#define ADDR_SHADOW_W3 FIX(0x005E5F34) // was 0x400 (1024)
#define ADDR_SHADOW_H3 FIX(0x005E5F38) // was 0x400

#define ADDR_SHADOW_W4 FIX(0x005E5F58) // was 0x400
#define ADDR_SHADOW_H4 FIX(0x005E5F5C) // was 0x400

#define ADDR_SHADOW_W5 FIX(0x005E5F7C) // was 0x100 (256)
#define ADDR_SHADOW_H5 FIX(0x005E5F80) // was 0x100

#define ADDR_SHADOW_W6 FIX(0x005E5FA0) // was 0x100
#define ADDR_SHADOW_H6 FIX(0x005E5FA4) // was 0x100

#define ADDR_SHADOW_W7 FIX(0x005E5FC4) // was 0x200
#define ADDR_SHADOW_H7 FIX(0x005E5FC8) // was 0x200

// dunno
#define ADDR_SS_SSS_AFT_BASED_FLAG 0xB2E94498


/*HOOK_DEFINE_TRAMPOLINE(Sss_Flag_Hook){
    static bool Callback(uint32_t index){
        if (index == 1) {
            return true;
        }
        return Orig(index);
    }
};*/


/*HOOK_DEFINE_INLINE(TwoInsOne) {
    static void Callback(exl::hook::nx64::InlineCtx* ctx) {
        uintptr_t target_addr = ctx->X[19] + ctx->X[8];
        *reinterpret_cast<uint64_t*>(target_addr) = 2;
    }
};

constexpr uint32_t ARM64_NOP = 0xD503201F;

template <typename HookType>
inline void ReplaceWithNopAndHook(uintptr_t addr) {
    exl::patch::CodePatcher(addr).Write<uint32_t>(ARM64_NOP);
    HookType::InstallAtOffset(addr);
}

uintptr_t setters[] = { 0x0020b800, 0x0020b870, 0x0020b8c0, 0x0020b910, 0x0020b960, 0x0020b990, 0x0020c160, 0x0020c430, 0x0020c460, 0x0020c5b0, 0x0020c600, 0x0020c630, 0x0020c660, 0x0020c870, 0x0020c8c0, 0x0020c910, 0x0020c960, 0x0020ca50, 0x0020ca80, 0x0020cad0, 0x0020cb00, 0x0020cb30, 0x0020cb80, 0x0020cc20, 0x0020c7b0, 0x002146d0 };
*/

namespace SssEnabler {

    void Init() {
        // old way // exl::patch::CodePatcher(ADDR_SSS_CALL_CHECK).Write<uint32_t>(0x52800020);


        // [ - Instead we can patch getter and setter - ]

        // getter of 15th bit in npr flags used for SSS pass
        //mov w0, #0x1
        exl::patch::CodePatcher(ADDR_SSS_BIT_GETTER).Write<uint32_t>(0x52800020);
        // ret
        exl::patch::CodePatcher(ADDR_SSS_BIT_GETTER + 4).Write<uint32_t>(0xD65F03C0);

        // Kill setter with ret
        exl::patch::CodePatcher(ADDR_SSS_BIT_SETTER).Write<uint32_t>(0xD65F03C0);


        // beautiful graphics cstm menu ft style (aft/ft comeback)
        exl::patch::CodePatcher(ADDR_NPR_CSTMMENU_KILL).Write<uint32_t>(0xF901BE7F);
        exl::patch::CodePatcher(ADDR_FXAA_CSTMMENU_KILL).Write<uint32_t>(0xF901C27F);

        //exl::patch::CodePatcher(ADDR_BRANCH_KILL_1).Write<uint32_t>(0xD503201F);
        //exl::patch::CodePatcher(ADDR_BRANCH_KILL_2).Write<uint32_t>(0xD503201F);

        // removes adp completely

        exl::patch::CodePatcher(ADDR_ADP_GETTER).Write<uint32_t>(0x52800000);
        exl::patch::CodePatcher(ADDR_ADP_GETTER + 4).Write<uint32_t>(0xD65F03C0);

        exl::patch::CodePatcher(ADDR_ADP_SETTER).Write<uint32_t>(0xD65F03C0);

        exl::patch::CodePatcher(ADDR_ADP_HARDCODE).Write<uint32_t>(0xD2800009);

       // SHADOWS
        exl::patch::CodePatcher(ADDR_SHADOW_W1).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H1).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W2).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H2).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W3).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H3).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W4).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H4).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W5).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H5).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W6).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H6).Write<uint32_t>(0x52808002);

        exl::patch::CodePatcher(ADDR_SHADOW_W7).Write<uint32_t>(0x52808001);
        exl::patch::CodePatcher(ADDR_SHADOW_H7).Write<uint32_t>(0x52808002);

        //exl::patch::CodePatcher(ADDR_SS_SSS_AFT_BASED_FLAG + 1).Write<uint8_t>(0x01);
        //exl::patch::CodePatcher(ADDR_SS_SSS_AFT_BASED_FLAG + 0xA).Write<uint8_t>(0x00);

        // (512 -> 1024) IDEK
        exl::patch::CodePatcher(0x00331C14).Write<uint32_t>(0x52808002); // mov w2, #0x400
        exl::patch::CodePatcher(0x00331C18).Write<uint32_t>(0x52808003); // mov w3, #0x400

        // (512 -> 1024) IDEK
        exl::patch::CodePatcher(0x0066DE38).Write<uint32_t>(0x52808001); // mov w1, #0x400
        exl::patch::CodePatcher(0x0066DE3C).Write<uint32_t>(0x52808018); // mov w24, #0x400


        // (512x256 -> 1024x512) IDEK
        exl::patch::CodePatcher(0x004A169C).Write<uint32_t>(0x52808001); // mov w1, #0x400
        exl::patch::CodePatcher(0x004A16A0).Write<uint32_t>(0x52804002); // mov w2, #0x200

        // refract
        exl::patch::CodePatcher(0x00B28CA8).Write<uint32_t>(1024); // refract w
        exl::patch::CodePatcher(0x00B28CAC).Write<uint32_t>(512); // refract h

        // reflect
        exl::patch::CodePatcher(0x00B28CBC).Write<uint32_t>(1024); // reflect w
        exl::patch::CodePatcher(0x00B28CC0).Write<uint32_t>(512); // reflect h

        //DISABLING OF SELF SHADOW
        exl::patch::CodePatcher(0x00450CF4).Write<uint32_t>(0x3902013F);

        // --- [ Atttempts of fixing Self-Shadow ] (half of this breaks shit so was just my countless hopes lols) ---

        /*// 1. Patch of setter (FUN_004f4280)
        // Change "mov w9, #0x1" to "mov w9, #0x0"
        exl::patch::CodePatcher(0x004f4284).Write<uint32_t>(0x52800009);

        // 2. Getter patch (FUN_004f42a0)
        exl::patch::CodePatcher(0x004f42a0).Write<uint32_t>(0x52800000); // mov w0, #0
        exl::patch::CodePatcher(0x004f42a4).Write<uint32_t>(0xD65F03C0); // ret*/

        /* // idk:/ (Downsample buffers perhaps)
        // 640x360 -> 960x540
        exl::patch::CodePatcher(0x00626198).Write<uint32_t>(0x52807801); // mov w1, #0x3C0
        exl::patch::CodePatcher(0x0062619C).Write<uint32_t>(0x52804382); // mov w2, #0x21C
        // 320x180 -> 480x270
        exl::patch::CodePatcher(0x006261BC).Write<uint32_t>(0x52803C01); // mov w1, #0x1E0
        exl::patch::CodePatcher(0x006261C0).Write<uint32_t>(0x528021C2); // mov w2, #0x10E
        exl::patch::CodePatcher(0x006261E0).Write<uint32_t>(0x52803C01); // mov w1, #0x1E0
        exl::patch::CodePatcher(0x006261E4).Write<uint32_t>(0x528021C2); // mov w2, #0x10E
        exl::patch::CodePatcher(0x00626204).Write<uint32_t>(0x52803C01); // mov w1, #0x1E0
        exl::patch::CodePatcher(0x00626208).Write<uint32_t>(0x528021C2); // mov w2, #0x10E

        //
        //Textures (960x540 -> 1920x1080)
        exl::patch::CodePatcher(0x00395220).Write<uint32_t>(0x52814013); // mov w19, #0xA00
        exl::patch::CodePatcher(0x00395224).Write<uint32_t>(0x52805014); // mov w20, #0x280
        exl::patch::CodePatcher(0x00395270).Write<uint32_t>(0x52808702); // mov w2, #0x438
        exl::patch::CodePatcher(0x00395274).Write<uint32_t>(0x5280F001); // mov w1, #0x780*/

        /*// Refl/buff again (i dunno really lol) (512x256 -> 1024x512)
        exl::patch::CodePatcher(0x004FFF34).Write<uint32_t>(0x52808019); // mov w25, #0x400
        exl::patch::CodePatcher(0x004FFF38).Write<uint32_t>(0x5280401A); // mov w26, #0x200*/

        //exl::patch::CodePatcher(0x005E5DC8).Write<uint32_t>(0x528005E5);

        //exl::patch::CodePatcher(ADDR_SSS_BLUR_IF_1).Write<uint32_t>(0xD503201F);
        //exl::patch::CodePatcher(ADDR_SSS_BLUR_IF_2).Write<uint32_t>(0xD503201F);
        //exl::patch::CodePatcher(ADDR_SSS_BLUR_IF_3).Write<uint32_t>(0xD503201F);

        //exl::patch::CodePatcher(ADDR_SSS_UPD_ZEROED).Write<uint32_t>(0xD503201F);
        //exl::patch::CodePatcher(ADDR_SSS_UPD_IF).Write<uint32_t>(0xD503201F);

        //exl::patch::CodePatcher(0x004f351c).Write<uint32_t>(0xD503201F); // nop second if
        //exl::patch::CodePatcher(FIX(0x006267D0)).Write<uint32_t>(0x39000A9F); // force 0

        /*ReplaceWithNopAndHook<TwoInsOne>(0x002028cc);

        for (auto addr : setters) {
            exl::patch::CodePatcher(addr).Write<uint32_t>(0xD65F03C0);
        }*/

        // --- [ Pathces of buffers and shadow formats and flags ] --- (again ts breaks shadows bruv)
        // Goal: set flags (0, 6, 0x1b, 0) for all calls FUN_004f5f10

        /*// 1.  +0x48 (005e5df4 - 005e5e00)
        // Orig: w3=3, w4=6, w6=1, w5=0
        exl::patch::CodePatcher(0x005E5DF4).Write<uint32_t>(0x2A1F03E3); // mov w3, wzr (was orr w3, wzr, #0x3)
        // 0x005E5DF8: w4 == 6 (orr w4, wzr, #0x6) - skip
        exl::patch::CodePatcher(0x005E5DFC).Write<uint32_t>(0x2A1F03E6); // mov w6, wzr (was mov w6, #0x1)
        exl::patch::CodePatcher(0x005E5E00).Write<uint32_t>(0x52800365); // mov w5, #0x1b (was mov w5, wzr)

        // 2.  +0x88 (005e5e18 - 005e5e24)
        // Orig: w3=3, w4=6, w6=1, w5=0
        exl::patch::CodePatcher(0x005E5E18).Write<uint32_t>(0x2A1F03E3); // mov w3, wzr (was orr w3, wzr, #0x3)
        // 0x005E5E1C: w4 == 6 - skip
        exl::patch::CodePatcher(0x005E5E20).Write<uint32_t>(0x2A1F03E6); // mov w6, wzr (was mov w6, #0x1)
        exl::patch::CodePatcher(0x005E5E24).Write<uint32_t>(0x52800365); // mov w5, #0x1b (was mov w5, wzr)*/

        /*// 3.  +0xC8 (005e5e3c - 005e5e48)
        // Orig: w4=0xc, w3=0, w5=0, w6=0
        exl::patch::CodePatcher(0x005E5E3C).Write<uint32_t>(0x528000C4); // mov w4, #0x6 (was orr w4, wzr, #0xc)
        // 0x005E5E40: w3 == 0 - skip
        exl::patch::CodePatcher(0x005E5E44).Write<uint32_t>(0x52800365); // mov w5, #0x1b (was mov w5, wzr)
        // 0x005E5E48: w6 == 0 - skip

        // 4.  +0x108 (005e5e60 - 005e5e6c)
        exl::patch::CodePatcher(0x005E5E60).Write<uint32_t>(0x528000C4); // mov w4, #0x6
        exl::patch::CodePatcher(0x005E5E68).Write<uint32_t>(0x52800365); // mov w5, #0x1b

        // 5.  +0x148 (005e5e84 - 005e5e90)
        exl::patch::CodePatcher(0x005E5E84).Write<uint32_t>(0x528000C4); // mov w4, #0x6
        exl::patch::CodePatcher(0x005E5E8C).Write<uint32_t>(0x52800365); // mov w5, #0x1b

        // 6.  +0x188 (005e5ea8 - 005e5eb4)
        exl::patch::CodePatcher(0x005E5EA8).Write<uint32_t>(0x528000C4); // mov w4, #0x6
        exl::patch::CodePatcher(0x005E5EB0).Write<uint32_t>(0x52800365); // mov w5, #0x1b

        // 7.  +0x1C8 (005e5ecc - 005e5ed8)
        // Orig: w4=6, w3=0, w5=0, w6=0. only w5 needs a change.
        // 0x005E5ECC: w4 == 6 (orr w4, wzr, #0x6) - skip
        exl::patch::CodePatcher(0x005E5ED4).Write<uint32_t>(0x52800365); // mov w5, #0x1b (was mov w5, wzr)*/


    }
}
