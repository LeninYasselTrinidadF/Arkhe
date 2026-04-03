#include "canvas_buttons.hpp"
#include "canvas_btn.hpp"
#include "nav_sync.hpp"
#include "../dep_view.hpp"
#include "../../data/position_state.hpp"
#include "../core/theme.hpp"
#include "../core/font_manager.hpp"
#include "../constants.hpp"
#include "raylib.h"

// ── draw_canvas_buttons (modo Burbujas) ───────────────────────────────────────

void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked) {
    const Theme& th = g_theme;
    const float  BX = 14.f;
    float by = (float)(UI_TOP() + 10), bw, bh;

    // [Casa]
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]",
        true, false, &bw, &bh)
        && state.nav_stack.size() > 1) {
        auto root = state.nav_stack[0];
        state.nav_stack.clear();
        state.nav_stack.push_back(root);
        state.selected_code.clear();
        state.selected_label.clear();
        state.resource_scroll = 0.f;
    }
    by += bh + btn_gap();

    // Burbujas ↔ Dependencias
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    bool dep_loaded = !use_graph.empty();
    bool dep_active = state.dep_view_active;
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
        dep_active ? "Burbujas" : "Dependencias",
        dep_active || dep_loaded, dep_active, &bw, &bh)) {
        if (dep_active) {
            nav_to_dep_node(state, state.dep_focus_id);
            state.dep_view_active = false;
        }
        else {
            dep_view_init_from_selection(state);
            state.dep_view_active = true;
        }
    }
    if (!dep_loaded) {
        int hsz = 10, tw = MeasureTextF("(sin deps.json)", hsz);
        DrawTextF("(sin deps.json)",
            (int)(BX + bw / 2 - tw / 2), (int)(by + bh + 2),
            hsz, ColorAlpha(th.text_dim, 0.4f));
        by += (float)g_fonts.scale(hsz) + 4.f;
    }
    by += bh + btn_gap();

    // Posicion
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Posicion",
        true, state.position_mode_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active) position_state_save(state);
    }
    by += bh + btn_gap();

    // < Atrás
    if (!dep_active && state.can_go_back())
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "< Atras",
            true, false, &bw, &bh))
            state.pop();
}

// ── draw_dep_canvas_buttons (modo Dependencias) ───────────────────────────────

void draw_dep_canvas_buttons(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool canvas_blocked)
{
    const Theme& th = g_theme;
    const float  BX = 14.f;
    float by = (float)(UI_TOP() + 10), bw, bh;

    // [Casa]
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]",
        true, false, &bw, &bh)) {
        dep_cam.target = { 0.f, 0.f };
        dep_cam.zoom = 1.f;
        const DepGraph& g = get_dep_graph_for_const(state);
        if (!g.empty()) dep_view_init(state, g.nodes().begin()->second.id);
    }
    by += bh + btn_gap();

    // Burbujas (volver) — navega el árbol al nodo enfocado en el grafo
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Burbujas",
        true, false, &bw, &bh)) {
        nav_to_dep_node(state, state.dep_focus_id);
        state.dep_view_active = false;
    }
    by += bh + btn_gap();

    // Posicion
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Posicion",
        true, state.position_mode_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active) position_state_save(state);
    }
    by += bh + btn_gap();

    // < Atrás
    if (!state.dep_focus_id.empty()) {
        auto parents = get_dep_graph_for_const(state).get_dependents(state.dep_focus_id);
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "< Atras",
            !parents.empty(), false, &bw, &bh))
            dep_view_init(state, parents[0]);
    }
}