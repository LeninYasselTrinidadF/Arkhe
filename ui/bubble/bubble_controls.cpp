#include "bubble_controls.hpp"
#include "dep_view.hpp"
#include "../data/position_state.hpp"
#include "core/overlay.hpp"
#include "core/theme.hpp"
#include "core/font_manager.hpp"
#include "core/nine_patch.hpp"
#include "core/skin.hpp"
#include "constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
// DISEÑO DE MODOS
//
// El switcher horizontal (flechas ◄ ►) cambia el CONTENIDO:
//   MSC2020  ←→  Mathlib  ←→  Estándar
//
// El botón toggle (canvas izquierdo) cambia la VISUALIZACIÓN del contenido:
//   Burbujas  ↕  Dependencias   (MSC2020 y Mathlib)
//   Burbujas  ↕  Lista          (Estándar — placeholder, mismo mecanismo)
//
// dep_view_active == false → draw_bubble_view
// dep_view_active == true  → draw_dep_view
//
// Ambos ejes son ortogonales: cambiar de MSC2020 a Mathlib no resetea
// dep_view_active; el usuario permanece en la misma visualización.
// ─────────────────────────────────────────────────────────────────────────────

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
        (float)(cx - HW - 4), (float)(cy - HH - 4),
        (float)(HW * 2 + 8),  (float)(HH * 2 + 8)
    };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)
        DrawTriangle({(float)(cx + HW), (float)(cy - HH)},
                     {(float)(cx + HW), (float)(cy + HH)},
                     {(float)(cx - HW), (float)cy}, col);
    else
        DrawTriangle({(float)(cx - HW), (float)(cy + HH)},
                     {(float)(cx - HW), (float)(cy - HH)},
                     {(float)(cx + HW), (float)cy}, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_mode_switcher ────────────────────────────────────────────────────────
// Cambia el eje de CONTENIDO solamente. El toggle de visualización
// (Burbujas ↔ Dependencias) vive en draw_canvas_buttons.

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th   = g_theme;
    const char* mname = mode_name(state.mode);
    int nw    = MeasureTextF(mname, 26);
    int bar_w = nw + 130;
    int bar_x = CX() - bar_w / 2;
    int bar_y = UI_TOP() + 10;
    int bar_h = 40;

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y,
                           (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    DrawLine(bar_x + 50, bar_y + 6, bar_x + 50, bar_y + bar_h - 6,
             th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - 50, bar_y + 6, bar_x + bar_w - 50, bar_y + bar_h - 6,
             th_alpha(th.ctrl_text_dim));
    DrawTextF(mname, CX() - nw / 2, bar_y + 8, 26, th.ctrl_text);

    if (draw_arrow(bar_x + 25,         bar_y + bar_h / 2, true,  mouse))
        state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - 25, bar_y + bar_h / 2, false, mouse))
        state.mode = mode_next(state.mode);
}

// ── draw_zoom_buttons ─────────────────────────────────────────────────────────

void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int bx = 14, by = g_split_y - 86, bw = 36, bh = 36;
    Rectangle rp = {(float)bx, (float)by,        (float)bw, (float)bh};
    Rectangle rm = {(float)bx, (float)(by + 42), (float)bw, (float)bh};
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
    DrawTextF("+", bx + 9,  by + 7,  27, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", bx + 11, by + 49, 27, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);
    }
    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureTextF(zbuf, 17);
    DrawTextF(zbuf, bx + bw / 2 - tw / 2, by + bh * 2 + 10, 17,
              th_alpha(th.ctrl_text_dim));
}

// ── draw_canvas_buttons ───────────────────────────────────────────────────────

void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked) {
    const Theme& th = g_theme;

    // helper: botón estándar, opcionalmente en estado "activo" (toggle encendido)
    auto canvas_btn = [&](float bx, float by, float bw, float bh,
                          const char* label, bool enabled,
                          bool active = false) -> bool
    {
        bool hov = enabled
                && CheckCollisionPointRec(mouse, {bx, by, bw, bh})
                && !canvas_blocked;

        Color bg     = active  ? th.accent
                     : hov     ? th.bg_button_hover
                     : enabled ? th_alpha(th.bg_button)
                               : Color{th.bg_button.r, th.bg_button.g,
                                       th.bg_button.b, 80};
        Color text   = active  ? th.bg_app
                     : hov     ? th.ctrl_text
                     : enabled ? th_alpha(th.ctrl_text_dim)
                               : Color{th.ctrl_text_dim.r, th.ctrl_text_dim.g,
                                       th.ctrl_text_dim.b, 80};
        Color border = active  ? th.accent
                     : enabled ? th.ctrl_border
                               : th_alpha(th.border);

        if (g_skin.button.valid())
            g_skin.button.draw(bx, by, bw, bh, bg);
        else {
            DrawRectangleRec({bx, by, bw, bh}, bg);
            DrawRectangleLinesEx({bx, by, bw, bh}, active ? 2.f : 1.f, border);
        }
        DrawTextF(label, (int)bx + 8, (int)by + 7, 19, text);
        return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    };

    const float BX   = 14.f;
    const float BH   = 32.f;
    const float GAP  = 8.f;
    float by = (float)(UI_TOP() + 10);

    // ── [Casa] ────────────────────────────────────────────────────────────────
    if (canvas_btn(BX, by, 90.f, BH, "[Casa]", true)) {
        if (state.nav_stack.size() > 1) {
            auto root_node = state.nav_stack[0];
            state.nav_stack.clear();
            state.nav_stack.push_back(root_node);
            state.selected_code.clear();
            state.selected_label.clear();
            state.resource_scroll = 0.f;
        }
    }
    by += BH + GAP;

    // ── Toggle Burbujas ↔ Dependencias ────────────────────────────────────────
    // Ambas etiquetas tienen ancho fijo para que el botón no salte de tamaño.
    // El botón siempre aparece; se deshabilita solo si el grafo no está cargado
    // Y el usuario intenta activar Dependencias (no tiene sentido sin datos).
    // Use dep_graph corresponding to current mode
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    bool dep_loaded = !use_graph.empty();
    bool dep_active = state.dep_view_active;

    // Etiqueta fija de ancho: ponemos el nombre del modo DESTINO
    const char* toggle_lbl = dep_active ? "Burbujas" : "Dependencias";
    float       toggle_w   = 138.f;
    bool        enabled    = dep_active || dep_loaded; // burbujas siempre OK

    if (canvas_btn(BX, by, toggle_w, BH, toggle_lbl, enabled, dep_active)) {
        if (dep_active) {
            state.dep_view_active = false;
        } else {
            // Initialize dependencies for the current mode's graph
            dep_view_init_from_selection(state);
            state.dep_view_active = true;
        }
    }

    // Aviso discreto debajo del botón si no hay deps.json
    if (!dep_loaded) {
        int tw = MeasureTextF("(sin deps.json)", 11);
        DrawTextF("(sin deps.json)",
                  (int)(BX + toggle_w / 2 - tw / 2),
                  (int)(by + BH + 2),
                  11, ColorAlpha(th.text_dim, 0.4f));
        by += 14.f; // empuja los botones de abajo
    }
    by += BH + GAP;

    // ── Botón POSICIÓN: alterna modo de posicionamiento (bloquea clicks en nodos)
    bool pos_active = state.position_mode_active;
    if (canvas_btn(BX, by, 100.f, BH, "Posicion", true, pos_active)) {
        state.position_mode_active = !state.position_mode_active;
        // Guardar/recargar posiciones temporales si se desactiva
        if (!state.position_mode_active) {
            // save
            position_state_save(state);
        } else {
            // load (ensure positions map exists)
            // already loaded on startup
        }
    }
    by += BH + GAP;

    // ── < Atrás — solo visible en modo Burbujas con profundidad > raíz ───────
    if (!dep_active && state.can_go_back()) {
        if (canvas_btn(BX, by, 100.f, BH, "< Atras", true))
            state.pop();
    }
}
