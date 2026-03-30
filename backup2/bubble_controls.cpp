#include "bubble_controls.hpp"
#include "dep_view.hpp"
#include "core/overlay.hpp"
#include "core/theme.hpp"
#include "core/font_manager.hpp"
#include "core/nine_patch.hpp"
#include "core/skin.hpp"
#include "constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>

// ── Helpers de modo ───────────────────────────────────────────────────────────

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
    Rectangle r = {
        (float)(cx-HW-4), (float)(cy-HH-4),
        (float)(HW*2+8),  (float)(HH*2+8)
    };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)
        DrawTriangle({(float)(cx+HW),(float)(cy-HH)},
                     {(float)(cx+HW),(float)(cy+HH)},
                     {(float)(cx-HW),(float)cy}, col);
    else
        DrawTriangle({(float)(cx-HW),(float)(cy+HH)},
                     {(float)(cx-HW),(float)(cy-HH)},
                     {(float)(cx+HW),(float)cy}, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_mode_switcher ────────────────────────────────────────────────────────

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th   = g_theme;
    const char* mname = mode_name(state.mode);
    int nw    = MeasureTextF(mname, 26);
    int bar_w = nw + 130;
    int bar_x = CX() - bar_w/2;
    int bar_y = UI_TOP() + 10;
    int bar_h = 40;

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y,
                           (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    DrawLine(bar_x+50, bar_y+6, bar_x+50, bar_y+bar_h-6, th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x+bar_w-50, bar_y+6, bar_x+bar_w-50, bar_y+bar_h-6,
             th_alpha(th.ctrl_text_dim));
    DrawTextF(mname, CX()-nw/2, bar_y+8, 26, th.ctrl_text);

    if (draw_arrow(bar_x+25,       bar_y+bar_h/2, true,  mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x+bar_w-25, bar_y+bar_h/2, false, mouse)) state.mode = mode_next(state.mode);
}

// ── draw_zoom_buttons ─────────────────────────────────────────────────────────

void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int bx = 14, by = g_split_y - 86, bw = 36, bh = 36;
    Rectangle rp = {(float)bx, (float)by,      (float)bw, (float)bh};
    Rectangle rm = {(float)bx, (float)(by+42), (float)bw, (float)bh};
    bool hp = CheckCollisionPointRec(mouse, rp);
    bool hm = CheckCollisionPointRec(mouse, rm);

    auto draw_btn = [&](Rectangle r, bool hov) {
        if (g_skin.button.valid())
            g_skin.button.draw(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        else {
            DrawRectangleRec(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
            DrawRectangleLinesEx(r, 1, th.ctrl_border);
        }
    };
    draw_btn(rp, hp); draw_btn(rm, hm);
    DrawTextF("+", bx+9,  by+7,  27, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", bx+11, by+49, 27, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);
    }
    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureTextF(zbuf, 17);
    DrawTextF(zbuf, bx+bw/2-tw/2, by+bh*2+10, 17, th_alpha(th.ctrl_text_dim));
}

// ── draw_canvas_buttons ───────────────────────────────────────────────────────

void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked) {
    const Theme& th = g_theme;

    // Helper: dibuja un botón de canvas, devuelve true si se hizo click
    auto canvas_btn = [&](float bx, float by, float bw, float bh,
                          const char* label, bool enabled) -> bool
    {
        bool hov = enabled
                && CheckCollisionPointRec(mouse, {bx, by, bw, bh})
                && !canvas_blocked;

        Color bg   = hov    ? th.bg_button_hover
                   : enabled? th_alpha(th.bg_button)
                            : Color{th.bg_button.r, th.bg_button.g, th.bg_button.b, 80};
        Color text = hov    ? th.ctrl_text
                   : enabled? th_alpha(th.ctrl_text_dim)
                            : Color{th.ctrl_text_dim.r, th.ctrl_text_dim.g, th.ctrl_text_dim.b, 80};

        if (g_skin.button.valid())
            g_skin.button.draw(bx, by, bw, bh, bg);
        else {
            DrawRectangleRec({bx, by, bw, bh}, bg);
            DrawRectangleLinesEx({bx, by, bw, bh}, 1, enabled ? th.ctrl_border
                                                               : th_alpha(th.border));
        }
        DrawTextF(label, (int)bx+8, (int)by+7, 19, text);
        return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    };

    // ── [Casa] ────────────────────────────────────────────────────────────────
    if (canvas_btn(14, (float)(UI_TOP()+10), 90, 32, "[Casa]", true)) {
        if (state.nav_stack.size() > 1) {
            auto root_node = state.nav_stack[0];
            state.nav_stack.clear();
            state.nav_stack.push_back(root_node);
            state.selected_code.clear();
            state.selected_label.clear();
            state.resource_scroll = 0.f;
        }
    }

    // ── Dependencias ─────────────────────────────────────────────────────────
    // Siempre visible; activo cuando hay algo que mostrar
    // (nodo seleccionado o nodo actual del stack)
    bool dep_enabled = !state.selected_code.empty()
                    || state.current() != nullptr;

    if (canvas_btn(14, (float)(UI_TOP()+52), 130, 32, "Contexto", dep_enabled)) {
        dep_view_init(state, state.selected_code);
        state.dep_view_active = true;
    }

    // ── < Atrás ───────────────────────────────────────────────────────────────
    if (state.can_go_back()) {
        if (canvas_btn(14, (float)(UI_TOP()+94), 100, 32, "< Atras", true))
            state.pop();
    }
}
