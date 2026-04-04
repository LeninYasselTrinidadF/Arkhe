#include "ui/views/controls/mode_switcher.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/nine_patch.hpp"
#include "ui/aesthetic/skin.hpp"
#include "ui/constants.hpp"
#include "raylib.h"

// ── Helpers internos ──────────────────────────────────────────────────────────

static const char* mode_name(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return "MSC2020";
    case ViewMode::Mathlib:  return "Mathlib";
    case ViewMode::Standard: return "Estandar";
    }
    return "";
}

static ViewMode mode_prev(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Standard;
    case ViewMode::Mathlib:  return ViewMode::MSC2020;
    case ViewMode::Standard: return ViewMode::Mathlib;
    }
    return ViewMode::MSC2020;
}

static ViewMode mode_next(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Mathlib;
    case ViewMode::Mathlib:  return ViewMode::Standard;
    case ViewMode::Standard: return ViewMode::MSC2020;
    }
    return ViewMode::MSC2020;
}

// ── draw_arrow ────────────────────────────────────────────────────────────────

bool draw_arrow(int cx, int cy, bool left, Vector2 mouse) {
    if (overlay::blocks_mouse(mouse)) return false;
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx - HW - 4), (float)(cy - HH - 4),
                    (float)(HW * 2 + 8),  (float)(HH * 2 + 8) };
    bool  hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)
        DrawTriangle({ (float)(cx + HW), (float)(cy - HH) },
            { (float)(cx + HW), (float)(cy + HH) },
            { (float)(cx - HW), (float)cy }, col);
    else
        DrawTriangle({ (float)(cx - HW), (float)(cy + HH) },
            { (float)(cx - HW), (float)(cy - HH) },
            { (float)(cx + HW), (float)cy }, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_mode_switcher ────────────────────────────────────────────────────────

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int lbl_sz = 20;
    int nw = MeasureTextF(mname, lbl_sz);
    int arrow_w = g_fonts.scale(50);
    int bar_w = nw + arrow_w * 2 + g_fonts.scale(16);
    int bar_x = CX() - bar_w / 2;
    int lbl_h = MeasureTextF("Ag", lbl_sz);
    int bar_h = lbl_h + g_fonts.scale(16);
    int bar_y = UI_TOP() + g_fonts.scale(8);

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y,
            (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    int sep_top = bar_y + g_fonts.scale(5);
    int sep_bot = bar_y + bar_h - g_fonts.scale(5);
    DrawLine(bar_x + arrow_w, sep_top, bar_x + arrow_w, sep_bot, th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - arrow_w, sep_top, bar_x + bar_w - arrow_w, sep_bot, th_alpha(th.ctrl_text_dim));

    DrawTextF(mname, CX() - nw / 2, bar_y + (bar_h - lbl_h) / 2, lbl_sz, th.ctrl_text);
    if (draw_arrow(bar_x + arrow_w / 2, bar_y + bar_h / 2, true, mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - arrow_w / 2, bar_y + bar_h / 2, false, mouse)) state.mode = mode_next(state.mode);
}