#include "ui/key_controls/kbnav_views/kbnav_views.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/views/dep_view.hpp"
#include "ui/views/controls/nav_sync.hpp"
#include "data/connect/vscode_bridge.hpp"
#include "raylib.h"
#include "raymath.h"

// ── Helpers internos ──────────────────────────────────────────────────────────

static ViewMode mode_cw(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Mathlib;
    case ViewMode::Mathlib:  return ViewMode::Standard;
    case ViewMode::Standard: return ViewMode::MSC2020;
    }
    return ViewMode::MSC2020;
}

static ViewMode mode_ccw(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Standard;
    case ViewMode::Mathlib:  return ViewMode::MSC2020;
    case ViewMode::Standard: return ViewMode::Mathlib;
    }
    return ViewMode::MSC2020;
}

// ── kbnav_views_handle ────────────────────────────────────────────────────────

bool kbnav_views_handle(AppState& state, Camera2D& cam, Camera2D& dep_cam) {
    // Solo actúa cuando la zona activa es Canvas (o el sistema kbnav no está activo).
    // Si la zona es Search, Info o Toolbar, las teclas no deben interferir con las vistas.
    if (g_kbnav.active && g_kbnav.zone != FocusZone::Canvas)
        return false;

    Camera2D& active_cam = state.dep_view_active ? dep_cam : cam;
    bool consumed = false;

    // ── +/− zoom ──────────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        active_cam.zoom = Clamp(active_cam.zoom + 0.15f, 0.05f, 6.f);
        consumed = true;
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        active_cam.zoom = Clamp(active_cam.zoom - 0.15f, 0.05f, 6.f);
        consumed = true;
    }

    // ── P: modo posición ──────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_P)) {
        state.position_mode_active = !state.position_mode_active;
        consumed = true;
    }

    // ── M / N: ciclar modo de vista ───────────────────────────────────────────
    if (IsKeyPressed(KEY_M)) { state.mode = mode_cw(state.mode);  consumed = true; }
    if (IsKeyPressed(KEY_N)) { state.mode = mode_ccw(state.mode); consumed = true; }

    // ── S: conmutar burbuja ↔ dependencias ────────────────────────────────────
    if (IsKeyPressed(KEY_S)) {
        if (state.dep_view_active) {
            nav_to_dep_node(state, state.dep_focus_id);
            state.dep_view_active = false;
            g_kbnav.dep_sel_id.clear();
        } else {
            const DepGraph& g = get_dep_graph_for_const(state);
            if (!g.empty()) {
                dep_view_init_from_selection(state);
                state.dep_view_active = true;
                g_kbnav.dep_sel_id.clear();
            }
        }
        consumed = true;
    }

    // ── E: abrir en VS Code ───────────────────────────────────────────────────
    if (IsKeyPressed(KEY_E)) { bridge_launch_vscode(state);         consumed = true; }

    // ── F: abrir en Mathlib ───────────────────────────────────────────────────
    if (IsKeyPressed(KEY_F)) { bridge_launch_vscode_mathlib(state); consumed = true; }

    // ── C: casa — resetear cámara y volver al nodo raíz ──────────────────────
    if (IsKeyPressed(KEY_C)) {
        if (state.dep_view_active) {
            active_cam.target = { 0.f, 0.f };
            active_cam.zoom   = 1.f;
            dep_view_init_from_selection(state);
        } else {
            state.save_cam(cam);
            while (state.can_go_back()) state.pop();
            state.restore_cam(cam);
            cam.target = { 0.f, 0.f };
            cam.zoom   = 1.f;
        }
        consumed = true;
    }

    return consumed;
}
