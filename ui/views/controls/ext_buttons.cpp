#include "ui/views/controls/ext_buttons.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "data/connect/vscode_bridge.hpp"
#include "raylib.h"

// ── Helper interno ────────────────────────────────────────────────────────────

static void draw_ext_btn(Vector2 mouse, int bx, int by,
    const char* lbl, void(*on_click)(const AppState&),
    const AppState& state)
{
    const Theme& th = g_theme;
    constexpr int bw = 54, bh = 22;
    Rectangle r = { (float)bx, (float)by, bw, bh };
    bool hov = CheckCollisionPointRec(mouse, r);
    DrawRectangleRec(r, hov ? th.bg_button_hover : th_alpha(th.bg_button));
    DrawRectangleLinesEx(r, 1.f, th_alpha(th.ctrl_border));
    int tw = MeasureTextF(lbl, 11);
    DrawTextF(lbl, bx + (bw - tw) / 2, by + (bh - 11) / 2, 11, th.ctrl_text);
    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        on_click(state);
}

// ── API pública ───────────────────────────────────────────────────────────────

void draw_vscode_button(AppState& state, Vector2 mouse) {
    draw_ext_btn(mouse, 10, g_split_y - 32, "\xE2\x86\x92 VS",
        bridge_launch_vscode, state);
}

void draw_mathlib_button(AppState& state, Vector2 mouse) {
    draw_ext_btn(mouse, 10 + 54 + 6, g_split_y - 32, "ML",
        bridge_launch_vscode_mathlib, state);
}