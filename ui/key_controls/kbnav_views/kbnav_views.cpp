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
    // Solo actua cuando la zona activa es Canvas (o el sistema kbnav no esta activo).
    // Si la zona es Search, Info o Toolbar, las teclas no deben interferir con las vistas.
    if (g_kbnav.active && g_kbnav.zone != FocusZone::Canvas)
        return false;

    Camera2D& active_cam = state.dep_view_active ? dep_cam : cam;
    bool consumed = false;

    // ── +/- zoom ──────────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        active_cam.zoom = Clamp(active_cam.zoom + 0.15f, 0.05f, 6.f);
        consumed = true;
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        active_cam.zoom = Clamp(active_cam.zoom - 0.15f, 0.05f, 6.f);
        consumed = true;
    }

    // ── P: modo posicion ──────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_P)) {
        state.position_mode_active = !state.position_mode_active;
        consumed = true;
    }

    // ── M / N: ciclar modo de vista ───────────────────────────────────────────
    if (IsKeyPressed(KEY_M)) { state.mode = mode_cw(state.mode);  consumed = true; }
    if (IsKeyPressed(KEY_N)) { state.mode = mode_ccw(state.mode); consumed = true; }

    // ── C: casa ───────────────────────────────────────────────────────────────
    // dep_view → solo recentra la camara (no hay nodo raiz en deps)
    // burbujas → volver al nodo raiz del nav_stack
    if (IsKeyPressed(KEY_C)) {
        if (state.dep_view_active) {
            active_cam.target = { 0.f, 0.f };
            active_cam.zoom = 1.f;
        }
        else {
            state.save_cam(cam);
            while (state.can_go_back()) state.pop();
            state.restore_cam(cam);
            cam.target = { 0.f, 0.f };
            cam.zoom = 1.f;
        }
        consumed = true;
    }

    // ── S: conmutar burbuja <-> dependencias ──────────────────────────────────
    if (IsKeyPressed(KEY_S)) {
        if (state.dep_view_active) {
            nav_to_dep_node(state, state.dep_focus_id);
            state.dep_view_active = false;
            g_kbnav.dep_sel_id.clear();
        }
        else {
            const DepGraph& g = get_dep_graph_for_const(state);
            if (!g.empty()) {
                dep_view_init_from_selection(state);
                state.dep_view_active = true;
                g_kbnav.dep_sel_id.clear();
            }
        }
        consumed = true;
    }

    // ── D: atras ──────────────────────────────────────────────────────────────
    // dep_view → retrocede en el historial de navegacion del grafo de deps
    // burbujas → sube un nivel en el nav_stack (equivale al boton < Atras)
    if (IsKeyPressed(KEY_D)) {
        if (state.dep_view_active) {
            dep_view_back(state, dep_cam);
        }
        else {
            if (state.can_go_back()) {
                state.save_cam(cam);
                state.pop();
                state.restore_cam(cam);
            }
        }
        consumed = true;
    }

    // ── E: editor de entradas ─────────────────────────────────────────────────
    if (IsKeyPressed(KEY_E)) {
        state.toolbar.editor_open = !state.toolbar.editor_open;
        consumed = true;
    }

    // ── F: abrir en VS Code    G: abrir en Mathlib ────────────────────────────
    if (IsKeyPressed(KEY_F)) { bridge_launch_vscode(state);         consumed = true; }
    if (IsKeyPressed(KEY_G)) { bridge_launch_vscode_mathlib(state); consumed = true; }

    return consumed;
}