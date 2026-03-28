#include "font_panel.hpp"
#include "font_manager.hpp"
#include "theme.hpp"
#include "constants.hpp"
#include "raylib.h"
#include <cstring>
#include <algorithm>

void FontPanel::draw(Vector2 mouse) {
    if (!state.toolbar.font_panel_open) return;

    if (draw_window_frame(PX, PY, PW, PH,
            "APARIENCIA / FUENTE", { 180, 140, 255, 255 },
            { 80, 50, 160, 220 }, mouse))
    {
        state.toolbar.font_panel_open = false;
        return;
    }

    const Theme& th = g_theme;
    const int lx = PX + 16;
    const int fw = PW - 32;
    int y = PY + 40;

    // ── Ruta de fuente .ttf ───────────────────────────────────────────────────
    DrawTextF("Fuente (.ttf)", lx, y, 10, th_alpha(th.text_dim));
    y += 14;
    draw_text_field(font_path_buf, 512,
                    lx, y, fw - 80, 22, 11,
                    active_field, 0, mouse);

    // Botón "Cargar"
    Rectangle load_r = { (float)(lx + fw - 76), (float)y, 72.0f, 22.0f };
    bool load_hov = CheckCollisionPointRec(mouse, load_r);
    DrawRectangleRec(load_r, load_hov
        ? Color{80,50,180,255} : Color{50,30,120,255});
    DrawRectangleLinesEx(load_r, 1.0f, {120,80,255,220});
    DrawTextF("Cargar", (int)load_r.x + 10, (int)load_r.y + 5, 11, WHITE);
    if (load_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_fonts.unload();
        g_fonts.load(font_path_buf);
    }
    y += 30;

    // ── Slider de tamaño base ─────────────────────────────────────────────────
    DrawTextF("Tamaño base de fuente", lx, y, 10, th_alpha(th.text_dim));
    y += 14;

    // Rango: 8 a 28
    constexpr float F_MIN = 8.0f, F_MAX = 28.0f;
    float& sz = g_fonts.base_size;

    int track_w = fw - 60;
    Rectangle track = { (float)lx, (float)(y + 8), (float)track_w, 4.0f };
    DrawRectangleRec(track, th_alpha(th.bg_button));

    float t      = (sz - F_MIN) / (F_MAX - F_MIN);
    int   fill_w = (int)(t * track_w);
    DrawRectangle(lx, y + 8, fill_w, 4, th.accent);

    // Knob
    int kx = lx + fill_w;
    DrawCircle(kx, y + 10, 7.0f, th.accent);
    DrawCircleLines(kx, y + 10, 7.0f, th.accent_hover);

    // Drag del slider
    Rectangle drag_zone = { (float)lx - 8, (float)(y + 2), (float)(track_w + 16), 18.0f };
    if (CheckCollisionPointRec(mouse, drag_zone) &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        float new_t = std::clamp((mouse.x - lx) / (float)track_w, 0.0f, 1.0f);        sz = F_MIN + new_t * (F_MAX - F_MIN);
        // Snap a enteros
        sz = (float)(int)(sz + 0.5f);
    }

    // Valor numérico a la derecha
    char sz_str[16]; snprintf(sz_str, sizeof(sz_str), "%.0fpx", sz);
    DrawTextF(sz_str, lx + track_w + 8, y + 4, 12, th.text_primary);
    y += 28;

    // ── Preview de texto ──────────────────────────────────────────────────────
    DrawLine(lx, y, lx + fw, y, th_alpha(th.border));
    y += 8;
    DrawTextF("Texto de prueba: AaBbCc 0123 +-=", lx, y, 13, th.text_primary);
    y += 20;
    DrawTextF("areas matematicas · referencias · Mathlib4", lx, y, 10, th_alpha(th.text_secondary));
}

void draw_font_panel(AppState& state, Vector2 mouse) {
    static FontPanel panel(state);
    panel.draw(mouse);
}
