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
        DrawTriangle({ (float)(cx + HW),(float)(cy - HH) },
            { (float)(cx + HW),(float)(cy + HH) },
            { (float)(cx - HW),(float)cy }, col);
    else
        DrawTriangle({ (float)(cx - HW),(float)(cy + HH) },
            { (float)(cx - HW),(float)(cy - HH) },
            { (float)(cx + HW),(float)cy }, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_mode_switcher ────────────────────────────────────────────────────────

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int lbl_sz = 20;                            // tamaño lógico del label (se escala)
    int nw = MeasureTextF(mname, lbl_sz);
    int arrow_w = g_fonts.scale(50);             // espacio para cada flecha (escala)
    int bar_w = nw + arrow_w * 2 + g_fonts.scale(16);
    int bar_x = CX() - bar_w / 2;
    int lbl_h = MeasureTextF("Ag", lbl_sz);
    int bar_h = lbl_h + g_fonts.scale(16);    // padding vertical escalado
    int bar_y = UI_TOP() + g_fonts.scale(8);

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y,
            (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    // Separadores verticales
    DrawLine(bar_x + arrow_w, bar_y + g_fonts.scale(5),
        bar_x + arrow_w, bar_y + bar_h - g_fonts.scale(5), th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - arrow_w, bar_y + g_fonts.scale(5),
        bar_x + bar_w - arrow_w, bar_y + bar_h - g_fonts.scale(5), th_alpha(th.ctrl_text_dim));

    // Label centrado
    int text_y = bar_y + (bar_h - lbl_h) / 2;
    DrawTextF(mname, CX() - nw / 2, text_y, lbl_sz, th.ctrl_text);

    if (draw_arrow(bar_x + arrow_w / 2, bar_y + bar_h / 2, true, mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - arrow_w / 2, bar_y + bar_h / 2, false, mouse)) state.mode = mode_next(state.mode);
}

// ── draw_zoom_buttons ─────────────────────────────────────────────────────────

void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    // Tamaño del botón escala con la fuente: se basa en el texto "+" escalado
    int btn_sz = MeasureTextF("+", 20) + g_fonts.scale(14);  // cuadrado
    int bx = 14;
    int by = g_split_y - btn_sz * 2 - g_fonts.scale(20);
    Rectangle rp = { (float)bx, (float)by,              (float)btn_sz, (float)btn_sz };
    Rectangle rm = { (float)bx, (float)(by + btn_sz + g_fonts.scale(6)), (float)btn_sz, (float)btn_sz };
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

    // Centrar "+" y "-" dentro de cada botón
    int sym_sz = 20;
    int sym_w = MeasureTextF("+", sym_sz);
    int sym_h = MeasureTextF("A", sym_sz);
    DrawTextF("+", (int)rp.x + (btn_sz - sym_w) / 2, (int)rp.y + (btn_sz - sym_h) / 2,
        sym_sz, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", (int)rm.x + (btn_sz - MeasureTextF("-", sym_sz)) / 2,
        (int)rm.y + (btn_sz - sym_h) / 2,
        sym_sz, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 6.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 6.0f);
    }
    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int lbl_sz = 11;
    int tw = MeasureTextF(zbuf, lbl_sz);
    DrawTextF(zbuf, bx + btn_sz / 2 - tw / 2,
        (int)rm.y + btn_sz + g_fonts.scale(4), lbl_sz, th_alpha(th.ctrl_text_dim));
}

// ── Helper interno: botón de canvas con tamaño dinámico ──────────────────────
// El ancho y alto se calculan desde el texto escalado + padding proporcional.
// No hay valores hardcodeados: todo crece con g_fonts.base_size.

static constexpr int BTN_FONT = 14;   // tamaño lógico del texto (se escala)
static constexpr int BTN_PAD_X = 10;   // half-padding horizontal lógico
static constexpr int BTN_PAD_Y = 7;   // half-padding vertical   lógico

// Calcula bw y bh para un label dado con la escala actual de fuente.
static void btn_dims(const char* label, float& bw, float& bh) {
    int tw = MeasureTextF(label, BTN_FONT);
    int font_h = MeasureTextF("Ag", BTN_FONT);          // altura real escalada
    int padx = g_fonts.scale(BTN_PAD_X);
    int pady = g_fonts.scale(BTN_PAD_Y);
    bw = (float)(tw + padx * 2);
    bh = (float)(font_h + pady * 2);
}

// Dibuja el botón y devuelve true si fue clickeado.
static bool canvas_btn_impl(const Theme& th, Vector2 mouse, bool canvas_blocked,
    float bx, float by,
    const char* label, bool enabled, bool active = false,
    float* out_bw = nullptr, float* out_bh = nullptr)
{
    float bw, bh;
    btn_dims(label, bw, bh);
    if (out_bw) *out_bw = bw;
    if (out_bh) *out_bh = bh;

    bool hov = enabled
        && CheckCollisionPointRec(mouse, { bx, by, bw, bh })
        && !canvas_blocked;

    Color bg = active ? th.accent
        : hov ? th.bg_button_hover
        : enabled ? th_alpha(th.bg_button)
        : Color{ th.bg_button.r, th.bg_button.g, th.bg_button.b, 80 };
    Color text_col = active ? th.bg_app
        : hov ? th.ctrl_text
        : enabled ? th_alpha(th.ctrl_text_dim)
        : Color{ th.ctrl_text_dim.r, th.ctrl_text_dim.g, th.ctrl_text_dim.b, 80 };
    Color border = active ? th.accent
        : enabled ? th.ctrl_border
        : th_alpha(th.border);

    if (g_skin.button.valid())
        g_skin.button.draw(bx, by, bw, bh, bg);
    else {
        DrawRectangleRec({ bx, by, bw, bh }, bg);
        DrawRectangleLinesEx({ bx, by, bw, bh }, active ? 2.f : 1.f, border);
    }

    // Texto centrado verticalmente
    int font_h = MeasureTextF("Ag", BTN_FONT);
    int pady = g_fonts.scale(BTN_PAD_Y);
    int padx = g_fonts.scale(BTN_PAD_X);
    DrawTextF(label, (int)bx + padx, (int)by + pady, BTN_FONT, text_col);

    return hov && enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Gap entre botones: también escala con la fuente
static float btn_gap() { return (float)g_fonts.scale(6); }

// ── draw_canvas_buttons (modo Burbujas) ───────────────────────────────────────

void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked) {
    const Theme& th = g_theme;
    const float  BX = 14.f;
    float by = (float)(UI_TOP() + 10);
    float bw, bh;

    // ── [Casa] ────────────────────────────────────────────────────────────────
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]", true,
        false, &bw, &bh)) {
        if (state.nav_stack.size() > 1) {
            auto root_node = state.nav_stack[0];
            state.nav_stack.clear();
            state.nav_stack.push_back(root_node);
            state.selected_code.clear();
            state.selected_label.clear();
            state.resource_scroll = 0.f;
        }
    }
    by += bh + btn_gap();

    // ── Toggle Burbujas ↔ Dependencias ────────────────────────────────────────
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    bool dep_loaded = !use_graph.empty();
    bool dep_active = state.dep_view_active;
    const char* toggle_lbl = dep_active ? "Burbujas" : "Dependencias";
    bool enabled = dep_active || dep_loaded;

    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
        toggle_lbl, enabled, dep_active, &bw, &bh)) {
        if (dep_active) {
            state.dep_view_active = false;
        }
        else {
            dep_view_init_from_selection(state);
            state.dep_view_active = true;
        }
    }
    if (!dep_loaded) {
        int hint_sz = 10;
        int tw = MeasureTextF("(sin deps.json)", hint_sz);
        DrawTextF("(sin deps.json)",
            (int)(BX + bw / 2 - tw / 2), (int)(by + bh + 2),
            hint_sz, ColorAlpha(th.text_dim, 0.4f));
        by += (float)g_fonts.scale(hint_sz) + 4.f;
    }
    by += bh + btn_gap();

    // ── Posición ──────────────────────────────────────────────────────────────
    bool pos_active = state.position_mode_active;
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
        "Posicion", true, pos_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active)
            position_state_save(state);
    }
    by += bh + btn_gap();

    // ── < Atrás ───────────────────────────────────────────────────────────────
    if (!dep_active && state.can_go_back()) {
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
            "< Atras", true, false, &bw, &bh))
            state.pop();
    }
}

// ── draw_dep_canvas_buttons (modo Dependencias) ───────────────────────────────

void draw_dep_canvas_buttons(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool canvas_blocked)
{
    const Theme& th = g_theme;
    const float  BX = 14.f;
    float by = (float)(UI_TOP() + 10);
    float bw, bh;

    // ── [Casa] — resetea cámara y vuelve al nodo raíz del grafo ──────────────
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]", true,
        false, &bw, &bh)) {
        dep_cam.target = { 0.f, 0.f };
        dep_cam.zoom = 1.f;
        const DepGraph& g = get_dep_graph_for_const(state);
        if (!g.empty())
            dep_view_init(state, g.nodes().begin()->second.id);
    }
    by += bh + btn_gap();

    // ── Burbujas — vuelve a la vista de árbol ─────────────────────────────────
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Burbujas", true,
        false, &bw, &bh))
        state.dep_view_active = false;
    by += bh + btn_gap();

    // ── Posición ──────────────────────────────────────────────────────────────
    bool pos_active = state.position_mode_active;
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
        "Posicion", true, pos_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active)
            position_state_save(state);
    }
    by += bh + btn_gap();

    // ── < Atrás — sube al padre en el grafo de deps ───────────────────────────
    if (!state.dep_focus_id.empty()) {
        const DepGraph& g = get_dep_graph_for_const(state);
        auto parents = g.get_dependents(state.dep_focus_id);
        bool has_parent = !parents.empty();
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
            "< Atras", has_parent, false, &bw, &bh))
            dep_view_init(state, parents[0]);
    }
}