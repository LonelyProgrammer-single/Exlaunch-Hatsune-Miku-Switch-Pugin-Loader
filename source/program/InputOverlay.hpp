#pragma once
#include <cstdint>

namespace InputOverlay {
    void CheckToggles();
    bool IsVisible();
    void SetVisible(bool state);
    void Draw();
}