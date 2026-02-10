#pragma once
#include "lib.hpp"
#define FIX(addr)           ((addr) - 0x100)
#define ADDR_LOOP_PATCH_1   FIX(0x4C1444)
#define ADDR_LOOP_PATCH_2   FIX(0x4C1374)
#define ADDR_NOP_1     FIX(0x5D1CDC)
#define ADDR_NOP_2    FIX(0x5D1CE0)
#define ADDR_UNLOCK_PATCH   FIX(0x0CA400)
#define OFFSET_SATURATION_1   FIX(0x5D1B64)
#define OFFSET_SATURATION_2   FIX(0x5D29B0)

inline void ApplyCustomPatches() {
    exl::patch::CodePatcher(ADDR_LOOP_PATCH_1).Write<uint32_t>(0xF1400EBF);
    exl::patch::CodePatcher(ADDR_LOOP_PATCH_2).Write<uint32_t>(0xF1400EBF);
    exl::patch::CodePatcher(ADDR_NOP_1).Write<uint32_t>(0xD503201F);
    exl::patch::CodePatcher(ADDR_NOP_2).Write<uint32_t>(0xD503201F);
    exl::patch::CodePatcher(OFFSET_SATURATION_1).Write<uint32_t>(0x710F951F);
    exl::patch::CodePatcher(OFFSET_SATURATION_2).Write<uint32_t>(0x710F903F);
    exl::patch::CodePatcher(ADDR_UNLOCK_PATCH).Write<uint32_t>(0x52800020);
}

