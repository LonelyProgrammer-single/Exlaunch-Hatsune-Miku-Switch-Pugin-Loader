#include "InputOverlay.hpp"
#include "imgui/imgui_nvn.h"
#include <hid.hpp>
#include <nn/fs.hpp>
#include "cmath"

namespace InputOverlay {

static bool g_showOverlay = false;

void CheckToggles() {
    nn::fs::FileHandle h;
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, "ExlSD:/DMLSwitchPort/overlay_toggle.bin", 1))) {
        nn::fs::CloseFile(h);
        if (R_SUCCEEDED(nn::fs::DeleteFile("ExlSD:/DMLSwitchPort/overlay_toggle.bin"))) {
            g_showOverlay = !g_showOverlay;
        }
    }
}

bool IsVisible() { return g_showOverlay; }
void SetVisible(bool state) { g_showOverlay = state; }

void Draw() {
    if (!g_showOverlay) return;

    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoBackground |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoInputs;

    float SCALE = 1.2f;
    auto s = [&](float v) { return v * SCALE; };

    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.5f, screenSize.y), ImGuiCond_Always, ImVec2(0.5f, 1.0f));

    if (ImGui::Begin("GamepadOverlay", nullptr, overlayFlags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->PushClipRectFullScreen();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        nn::hid::NpadHandheldState npad = nn::hid::GetMergedNpadState();
        uint64_t btns = npad.buttons;

        // COLORS
        ImU32 alphaGlo  = 225;
        ImU32 alphaBtn  = 220;
        ImU32 alphaLogo = 150;

        ImU32 colLeft    = IM_COL32(0, 195, 227, alphaGlo);
        ImU32 colRight   = IM_COL32(255, 50, 60, alphaGlo);
        ImU32 colMiddle  = IM_COL32(35, 35, 35, alphaGlo);
        ImU32 colBtnBase = IM_COL32(15, 15, 15, alphaBtn);
        ImU32 colBtnPrs  = IM_COL32(200, 200, 200, alphaBtn);
        ImU32 colLogo    = IM_COL32(210, 210, 210, alphaLogo);

        ImU32 colGreen = IM_COL32(20, 230, 150, alphaBtn);
        ImU32 colRed   = IM_COL32(205, 95, 95, alphaBtn);
        ImU32 colBlue  = IM_COL32(140, 180, 255, alphaBtn);
        ImU32 colPink  = IM_COL32(240, 100, 200, alphaBtn);

        // BODY COORDINATES
        float jcW     = s(44.0f);
        float centerW = s(58.0f);
        float gripW   = centerW / 2.0f;
        float cx      = pos.x + s(140.0f);
        float leftX   = cx - gripW - jcW;
        float rightX  = cx + gripW;
        float jcRad   = s(25.0f);
        float topY    = pos.y + s(45.0f);
        float bodyBottomY = topY + s(190.0f);

        float leftTopGroupY     = topY + s(24.0f);
        float leftBottomGroupY  = leftTopGroupY + s(36.0f);
        float rightTopGroupY    = leftTopGroupY + s(8.0f);
        float rightBottomGroupY = leftBottomGroupY + s(8.0f);

        ImGui::Dummy(ImVec2(s(280.0f), (leftBottomGroupY + s(15.0f)) - pos.y));

        // 1. ZL/ZR TRIGGERS
        float zc = s(2.5f);
        draw->PathClear();
        draw->PathArcTo(ImVec2(leftX + s(7.0f), topY - s(13.0f)), zc, IM_PI, IM_PI * 1.5f, 5);
        draw->PathArcTo(ImVec2(cx - gripW - s(13.0f), topY - s(10.0f)), zc, IM_PI * 1.5f, IM_PI * 2.0f, 5);
        draw->PathArcTo(ImVec2(cx - gripW - s(13.0f), topY - s(7.0f)), zc, 0.0f, IM_PI * 0.5f, 5);
        draw->PathArcTo(ImVec2(leftX + s(5.0f), topY - s(5.0f)), zc, IM_PI * 0.5f, IM_PI, 5);
        draw->PathFillConvex((btns & nn::hid::Button::ZL) ? colBtnPrs : colBtnBase);

        draw->PathClear();
        draw->PathArcTo(ImVec2(cx + gripW + s(13.0f), topY - s(10.0f)), zc, IM_PI, IM_PI * 1.5f, 5);
        draw->PathArcTo(ImVec2(rightX + jcW - s(7.0f), topY - s(13.0f)), zc, IM_PI * 1.5f, IM_PI * 2.0f, 5);
        draw->PathArcTo(ImVec2(rightX + jcW - s(5.0f), topY - s(5.0f)), zc, 0.0f, IM_PI * 0.5f, 5);
        draw->PathArcTo(ImVec2(cx + gripW + s(13.0f), topY - s(7.0f)), zc, IM_PI * 0.5f, IM_PI, 5);
        draw->PathFillConvex((btns & nn::hid::Button::ZR) ? colBtnPrs : colBtnBase);

        // 2. L/R BUMPERS
        float bThick = s(3.5f);
        float flushOff = bThick / 2.0f;
        draw->PathClear();
        draw->PathArcTo(ImVec2(leftX + jcRad, topY + jcRad), jcRad + flushOff, IM_PI * 1.2f, IM_PI * 1.5f, 15);
        draw->PathLineTo(ImVec2(cx - gripW - s(12.0f), topY - flushOff));
        draw->PathStroke((btns & nn::hid::Button::L) ? colBtnPrs : colBtnBase, 0, bThick);

        draw->PathClear();
        draw->PathLineTo(ImVec2(cx + gripW + s(12.0f), topY - flushOff));
        draw->PathArcTo(ImVec2(rightX + jcW - jcRad, topY + jcRad), jcRad + flushOff, IM_PI * 1.5f, IM_PI * 1.8f, 15);
        draw->PathStroke((btns & nn::hid::Button::R) ? colBtnPrs : colBtnBase, 0, bThick);

        // 3. CONTROLLER BODY
        draw->AddRectFilled(ImVec2(cx - gripW, topY), ImVec2(cx + gripW, bodyBottomY), colMiddle);
        draw->AddRectFilled(ImVec2(leftX, topY), ImVec2(cx - gripW, bodyBottomY), colLeft, jcRad, ImDrawFlags_RoundCornersTopLeft);
        draw->AddRectFilled(ImVec2(rightX, topY), ImVec2(rightX + jcW, bodyBottomY), colRight, jcRad, ImDrawFlags_RoundCornersTopRight);
        draw->AddLine(ImVec2(cx - gripW, topY), ImVec2(cx - gripW, bodyBottomY), IM_COL32(0,0,0, 130), s(2.0f));
        draw->AddLine(ImVec2(rightX, topY), ImVec2(rightX, bodyBottomY), IM_COL32(0,0,0, 130), s(2.0f));

        // =========================================================
        // 4. SWITCH LOGO (KEEP LEFT AS IS, SHRINK RIGHT)
        // =========================================================
        float ly = topY + s(38.0f);
        float lw = s(11.0f);
        float lh = s(12.0f);
        float gapLogo = s(3.0f);
        float th = s(2.2f);
        float logoRad = s(4.0f);

        float logoLeftX = cx - gapLogo/2.0f - lw;

        // LEFT PART (Remains as it was)
        draw->AddRect(
            ImVec2(logoLeftX, ly - lh),
            ImVec2(logoLeftX + lw, ly + lh),
            colLogo, logoRad, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft, th
        );

        // RIGHT PART
        float rX1 = cx + gapLogo/2.0f - th/2.0f;
        float rY1 = ly - lh - th/2.0f;
        float rW  = lw + th;
        float rH  = lh * 2.0f + th;

        // Here is the offset that "eats" extra pixels from all sides of the right half
        float shrink = s(0.6f);

        draw->AddRectFilled(
            ImVec2(rX1 + shrink, rY1 + shrink),
            ImVec2(rX1 + rW - shrink, rY1 + rH - shrink),
            colLogo, logoRad + th/2.0f, ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomRight
        );

        // DOTS (Exactly 2.0f)
        float dotRad = s(2.0f);
        float dotLeftX = logoLeftX + lw / 2.0f;
        float dotRightX = rX1 + rW / 2.0f;
        float dotOffsetY = lh / 2.0f - s(0.5f);

        // Left dot (top)
        draw->AddCircleFilled(ImVec2(dotLeftX, ly - dotOffsetY), dotRad, colLogo);

        // Right dot (bottom, black)
        ImU32 colHole = IM_COL32(0, 0, 0, alphaGlo);
        draw->AddCircleFilled(ImVec2(dotRightX, ly + dotOffsetY), dotRad, colHole);

        // =========================================================
        // 5. INDICATORS
        // =========================================================
        ImU32 ledOn = IM_COL32(50, 255, 50, alphaGlo);
        ImU32 ledOff = IM_COL32(20, 50, 20, 130);
        float indRad = s(1.2f), indGap = s(3.5f), startIndY = ly - (indGap * 1.5f);
        for(int i=0; i<4; i++) {
            draw->AddCircleFilled(ImVec2(cx - gripW + s(6.0f), startIndY + i*indGap), indRad, (i == 0) ? ledOn : ledOff);
            draw->AddCircleFilled(ImVec2(cx + gripW - s(6.0f), startIndY + i*indGap), indRad, (i == 0) ? ledOn : ledOff);
        }

        // 6. TEXT
        auto DrawBlockChar = [&](char c, float x, float y, float w, float hc, float t) {
            float m = w * 0.35f;
            draw->PathClear();
            switch(c) {
                case 'N': draw->AddLine(ImVec2(x,y+hc),ImVec2(x,y),colLogo,t); draw->AddLine(ImVec2(x,y),ImVec2(x+w,y+hc),colLogo,t); draw->AddLine(ImVec2(x+w,y+hc),ImVec2(x+w,y),colLogo,t); break;
                case 'I': draw->AddLine(ImVec2(x+w/2,y),ImVec2(x+w/2,y+hc),colLogo,t); break;
                case 'T': draw->AddLine(ImVec2(x,y),ImVec2(x+w,y),colLogo,t); draw->AddLine(ImVec2(x+w/2,y),ImVec2(x+w/2,y+hc),colLogo,t); break;
                case 'E': draw->AddLine(ImVec2(x+w,y),ImVec2(x,y),colLogo,t); draw->AddLine(ImVec2(x,y),ImVec2(x,y+hc),colLogo,t); draw->AddLine(ImVec2(x,y+hc),ImVec2(x+w,y+hc),colLogo,t); draw->AddLine(ImVec2(x,y+hc/2),ImVec2(x+w*0.8f,y+hc/2),colLogo,t); break;
                case 'H': draw->AddLine(ImVec2(x,y),ImVec2(x,y+hc),colLogo,t); draw->AddLine(ImVec2(x+w,y),ImVec2(x+w,y+hc),colLogo,t); draw->AddLine(ImVec2(x,y+hc/2),ImVec2(x+w,y+hc/2),colLogo,t); break;
                case 'O': case 'D':
                    if (c == 'O') { draw->AddLine(ImVec2(x+m,y), ImVec2(x,y+m), colLogo, t); draw->AddLine(ImVec2(x,y+m), ImVec2(x,y+hc-m), colLogo, t); draw->AddLine(ImVec2(x,y+hc-m), ImVec2(x+m,y+hc), colLogo, t); }
                    else { draw->AddLine(ImVec2(x,y), ImVec2(x,y+hc), colLogo, t); draw->AddLine(ImVec2(x,y+hc), ImVec2(x+m,y+hc), colLogo, t); draw->AddLine(ImVec2(x,y), ImVec2(x+m,y), colLogo, t); }
                    draw->AddLine(ImVec2(x+m,y), ImVec2(x+w-m,y), colLogo, t); draw->AddLine(ImVec2(x+w-m,y), ImVec2(x+w,y+m), colLogo, t);
                    draw->AddLine(ImVec2(x+w,y+m), ImVec2(x+w,y+hc-m), colLogo, t); draw->AddLine(ImVec2(x+w,y+hc-m), ImVec2(x+w-m,y+hc), colLogo, t);
                    draw->AddLine(ImVec2(x+w-m,y+hc), ImVec2(x+m,y+hc), colLogo, t); break;
                case 'S':
                    draw->AddLine(ImVec2(x+w,y+m/2), ImVec2(x+w-m,y), colLogo, t); draw->AddLine(ImVec2(x+w-m,y), ImVec2(x+m,y), colLogo, t);
                    draw->AddLine(ImVec2(x+m,y), ImVec2(x,y+m/2), colLogo, t); draw->AddLine(ImVec2(x,y+m/2), ImVec2(x,y+hc/2-m/2), colLogo, t);
                    draw->AddLine(ImVec2(x,y+hc/2-m/2), ImVec2(x+m,y+hc/2), colLogo, t); draw->AddLine(ImVec2(x+m,y+hc/2), ImVec2(x+w-m,y+hc/2), colLogo, t);
                    draw->AddLine(ImVec2(x+w-m,y+hc/2), ImVec2(x+w,y+hc/2+m/2), colLogo, t); draw->AddLine(ImVec2(x+w,y+hc/2+m/2), ImVec2(x+w,y+hc-m/2), colLogo, t);
                    draw->AddLine(ImVec2(x+w,y+hc-m/2), ImVec2(x+w-m,y+hc), colLogo, t); draw->AddLine(ImVec2(x+w-m,y+hc), ImVec2(x+m,y+hc), colLogo, t);
                    draw->AddLine(ImVec2(x+m,y+hc), ImVec2(x,y+hc-m/2), colLogo, t); break;
                case 'W': draw->AddLine(ImVec2(x,y),ImVec2(x+w*0.25f,y+hc),colLogo,t); draw->AddLine(ImVec2(x+w*0.25f,y+hc),ImVec2(x+w*0.5f,y+hc*0.5f),colLogo,t); draw->AddLine(ImVec2(x+w*0.5f,y+hc*0.5f),ImVec2(x+w*0.75f,y+hc),colLogo,t); draw->AddLine(ImVec2(x+w*0.75f,y+hc),ImVec2(x+w,y),colLogo,t); break;
                case 'C': draw->AddLine(ImVec2(x+w,y+m), ImVec2(x+w-m,y), colLogo, t); draw->AddLine(ImVec2(x+w-m,y), ImVec2(x+m,y), colLogo, t); draw->AddLine(ImVec2(x+m,y), ImVec2(x,y+m), colLogo, t); draw->AddLine(ImVec2(x,y+m), ImVec2(x,y+hc-m), colLogo, t); draw->AddLine(ImVec2(x,y+hc-m), ImVec2(x+m,y+hc), colLogo, t); draw->AddLine(ImVec2(x+m,y+hc), ImVec2(x+w-m,y+hc), colLogo, t); draw->AddLine(ImVec2(x+w-m,y+hc), ImVec2(x+w,y+hc-m), colLogo, t); break;
            }
        };
        auto DrawWord = [&](const char* word, float startX, float endX, float y_pos, float charHeight, float t) {
            int len = 0; float totalCharW = 0;
            for (const char* p = word; *p; ++p) { len++; totalCharW += (*p == 'I') ? charHeight * 0.25f : ((*p == 'W') ? charHeight * 0.95f : charHeight * 0.65f); }
            float space = (endX - startX - totalCharW) / (len - 1), currX = startX;
            for (const char* p = word; *p; ++p) {
                float w = (*p == 'I') ? charHeight * 0.25f : ((*p == 'W') ? charHeight * 0.95f : charHeight * 0.65f);
                DrawBlockChar(*p, currX, y_pos, w, charHeight, t);
                currX += w + space;
            }
        };
        DrawWord("NINTENDO", cx - s(20.0f), cx + s(20.0f), ly + s(17.0f), s(3.5f), s(1.0f));
        DrawWord("SWITCH", cx - s(20.0f), cx + s(20.0f), ly + s(23.0f), s(7.0f), s(1.8f));

        // 7. BUTTONS AND STICKS
        auto DrawSwitchDPad = [&](ImVec2 c, float offset, int dir, uint64_t mask) {
            bool pressed = (btns & mask) != 0;
            ImVec2 btnC = (dir==0)?ImVec2(c.x,c.y-offset):(dir==1)?ImVec2(c.x,c.y+offset):(dir==2)?ImVec2(c.x-offset,c.y):ImVec2(c.x+offset,c.y);
            draw->AddCircleFilled(btnC, s(6.0f), pressed ? colBtnPrs : colBtnBase);
            ImU32 arrCol = pressed ? IM_COL32(0,0,0,alphaBtn) : IM_COL32(180,180,180,alphaBtn);
            float R = s(2.6f), r = R / 2.0f, w = R * 0.866f;
            if (dir==0) draw->AddTriangleFilled(ImVec2(btnC.x,btnC.y-R), ImVec2(btnC.x-w,btnC.y+r), ImVec2(btnC.x+w,btnC.y+r), arrCol);
            if (dir==1) draw->AddTriangleFilled(ImVec2(btnC.x,btnC.y+R), ImVec2(btnC.x-w,btnC.y-r), ImVec2(btnC.x+w,btnC.y-r), arrCol);
            if (dir==2) draw->AddTriangleFilled(ImVec2(btnC.x-R,btnC.y), ImVec2(btnC.x+r,btnC.y-w), ImVec2(btnC.x+r,btnC.y+w), arrCol);
            if (dir==3) draw->AddTriangleFilled(ImVec2(btnC.x+R,btnC.y), ImVec2(btnC.x-r,btnC.y-w), ImVec2(btnC.x-r,btnC.y+w), arrCol);
        };
        float lcx = leftX + (jcW/2.0f), rcx = rightX + (jcW/2.0f);
        DrawSwitchDPad(ImVec2(lcx, leftBottomGroupY), s(11.5f), 0, nn::hid::Button::Up);
        DrawSwitchDPad(ImVec2(lcx, leftBottomGroupY), s(11.5f), 1, nn::hid::Button::Down);
        DrawSwitchDPad(ImVec2(lcx, leftBottomGroupY), s(11.5f), 2, nn::hid::Button::Left);
        DrawSwitchDPad(ImVec2(lcx, leftBottomGroupY), s(11.5f), 3, nn::hid::Button::Right);

        auto DrawPSBtn = [&](ImVec2 c, uint64_t mask, int shape, ImU32 shapeCol) {
            bool pressed = (btns & mask) != 0;
            draw->AddCircleFilled(c, s(6.5f), pressed ? colBtnPrs : colBtnBase);
            draw->AddCircle(c, s(6.5f), IM_COL32(50,50,50,alphaBtn), 0, s(1.0f));
            float sz = s(3.0f), t = pressed ? s(2.0f) : s(1.5f);
            if(shape==0){draw->AddLine(ImVec2(c.x-sz,c.y-sz),ImVec2(c.x+sz,c.y+sz),shapeCol,t);draw->AddLine(ImVec2(c.x-sz,c.y+sz),ImVec2(c.x+sz,c.y-sz),shapeCol,t);}
            if(shape==1)draw->AddCircle(c, sz+s(0.5f), shapeCol, 0, t);
            if(shape==2)draw->AddRect(ImVec2(c.x-sz,c.y-sz), ImVec2(c.x+sz,c.y+sz), shapeCol, 0, 0, t);
            if(shape==3){float R=s(3.2f),r=R/2,w=R*0.866f;draw->AddTriangle(ImVec2(c.x,c.y-R),ImVec2(c.x-w,c.y+r),ImVec2(c.x+w,c.y+r),shapeCol,t);}
        };
        DrawPSBtn(ImVec2(rcx, rightTopGroupY - s(12.5f)), nn::hid::Button::X, 3, colGreen);
        DrawPSBtn(ImVec2(rcx, rightTopGroupY + s(12.5f)), nn::hid::Button::B, 0, colBlue);
        DrawPSBtn(ImVec2(rcx - s(12.5f), rightTopGroupY), nn::hid::Button::Y, 2, colPink);
        DrawPSBtn(ImVec2(rcx + s(12.5f), rightTopGroupY), nn::hid::Button::A, 1, colRed);

        auto DrawStick = [&](float sx, float sy, int32_t stickX, int32_t stickY, uint64_t clickMask) {
            float nx = (float)stickX/32768.0f, ny = -(float)stickY/32768.0f;
            float d = std::sqrt(nx*nx+ny*ny); if(d>1.0f){nx/=d;ny/=d;}
            ImVec2 head(sx+nx*s(4.0f), sy+ny*s(4.0f));
            draw->AddCircleFilled(ImVec2(sx,sy), s(11.0f), IM_COL32(10,10,10,150));
            draw->AddCircleFilled(head, s(7.5f), (btns&clickMask)?colBtnPrs:colBtnBase);
            draw->AddCircle(head, s(7.5f), IM_COL32(60,60,60,alphaBtn), 0, s(1.2f));
        };
        DrawStick(lcx, leftTopGroupY, npad.analogStickL[0], npad.analogStickL[1], nn::hid::Button::LStick);
        DrawStick(rcx, rightBottomGroupY, npad.analogStickR[0], npad.analogStickR[1], nn::hid::Button::RStick);

        // =========================================================
        // 8. +/- BUTTONS (MONOLITHIC PLUS WITHOUT SQUARE)
        // =========================================================
        float pmCenterY = topY + s(7.0f);
        float pmHalfLen = s(4.5f);
        float pmHalfThick = s(1.25f);

        float minusCX = (cx - gripW) - s(9.5f);
        float plusCX  = rightX + s(8.5f);

        // MINUS button
        draw->AddRectFilled(
            ImVec2(minusCX - pmHalfLen, pmCenterY - pmHalfThick),
            ImVec2(minusCX + pmHalfLen, pmCenterY + pmHalfThick),
            (btns & nn::hid::Button::Minus) ? colBtnPrs : colBtnBase,
            s(1.0f)
        );

        // PLUS button
        ImU32 pCol = (btns & nn::hid::Button::Plus) ? colBtnPrs : colBtnBase;
        float pR = s(0.8f);

        // Disable anti-aliasing so the blocks merge into a monolith without seams or squares
        ImDrawListFlags backup_flags = draw->Flags;
        draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;

        // Center horizontal bar (left and right rays)
        draw->AddRectFilled(
            ImVec2(plusCX - pmHalfLen, pmCenterY - pmHalfThick),
            ImVec2(plusCX + pmHalfLen, pmCenterY + pmHalfThick),
            pCol, pR, ImDrawFlags_RoundCornersLeft | ImDrawFlags_RoundCornersRight
        );

        // Top ray
        draw->AddRectFilled(
            ImVec2(plusCX - pmHalfThick, pmCenterY - pmHalfLen),
            ImVec2(plusCX + pmHalfThick, pmCenterY - pmHalfThick),
            pCol, pR, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight
        );

        // Bottom ray
        draw->AddRectFilled(
            ImVec2(plusCX - pmHalfThick, pmCenterY + pmHalfThick),
            ImVec2(plusCX + pmHalfThick, pmCenterY + pmHalfLen),
            pCol, pR, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight
        );

        // Restore anti-aliasing
        draw->Flags = backup_flags;

        draw->PopClipRect();
    }
    ImGui::End();
}

}
