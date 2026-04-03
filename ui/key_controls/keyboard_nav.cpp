#include "keyboard_nav.hpp"
#include "../dep_view.hpp"
#include "../controls/nav_sync.hpp"
#include "../core/theme.hpp"
#include "../core/font_manager.hpp"
#include "../constants.hpp"
#include "../../data/vscode_bridge.hpp"
#include "raylib.h"
#include "raymath.h"

KbNavState g_kbnav;

// ── Helpers internos ──────────────────────────────────────────────────────────

static FocusZone zone_next(FocusZone z) {
    switch (z) {
    case FocusZone::Canvas:  return FocusZone::Toolbar;
    case FocusZone::Toolbar: return FocusZone::Search;
    case FocusZone::Search:  return FocusZone::Info;
    case FocusZone::Info:    return FocusZone::Canvas;
    }
    return FocusZone::Canvas;
}

static ViewMode mode_cw(ViewMode m) {   // M → derecha
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Mathlib;
    case ViewMode::Mathlib:  return ViewMode::Standard;
    case ViewMode::Standard: return ViewMode::MSC2020;
    }
    return ViewMode::MSC2020;
}

static ViewMode mode_ccw(ViewMode m) {  // N → izquierda
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Standard;
    case ViewMode::Mathlib:  return ViewMode::MSC2020;
    case ViewMode::Standard: return ViewMode::Mathlib;
    }
    return ViewMode::MSC2020;
}

// ── kbnav_handle_global ───────────────────────────────────────────────────────

bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam) {
    // Desactivar con cualquier clic de ratón
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
     || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
     || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        g_kbnav.deactivate();
        return false;
    }

    bool consumed = false;
    Camera2D& active_cam = state.dep_view_active ? dep_cam : cam;

    // ── Tab: ciclar zona de foco ──────────────────────────────────────────────
    if (IsKeyPressed(KEY_TAB)) {
        if (!g_kbnav.active) {
            g_kbnav.activate(FocusZone::Canvas);
        } else {
            g_kbnav.goto_zone(zone_next(g_kbnav.zone));
        }
        consumed = true;
    }

    // Activar en primera flecha/Enter sin consumir la tecla
    // (la zona Canvas la procesará también en su propio handler)
    if (!g_kbnav.active &&
        (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
         IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_DOWN)  ||
         IsKeyPressed(KEY_ENTER)|| IsKeyPressed(KEY_KP_ENTER))) {
        g_kbnav.activate(FocusZone::Canvas);
        // No consumed → el handler de canvas procesará la tecla este mismo frame
    }

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

    // ── M / N: cambio de modo ─────────────────────────────────────────────────
    if (IsKeyPressed(KEY_M)) { state.mode = mode_cw(state.mode);  consumed = true; }
    if (IsKeyPressed(KEY_N)) { state.mode = mode_ccw(state.mode); consumed = true; }

    // ── S: conmutar Burbujas ↔ Dependencias ──────────────────────────────────
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
                g_kbnav.dep_sel_id.clear();  // se inicializa en draw_dep_view
            }
        }
        consumed = true;
    }

    // ── E: abrir en VS Code ───────────────────────────────────────────────────
    if (IsKeyPressed(KEY_E)) { bridge_launch_vscode(state);         consumed = true; }

    // ── F: abrir en Mathlib ───────────────────────────────────────────────────
    if (IsKeyPressed(KEY_F)) { bridge_launch_vscode_mathlib(state); consumed = true; }

    return consumed;
}

// ── kbnav_draw_indicator ──────────────────────────────────────────────────────

static const char* zone_label(FocusZone z) {
    switch (z) {
    case FocusZone::Canvas:  return "Canvas";
    case FocusZone::Toolbar: return "Toolbar";
    case FocusZone::Search:  return "Busqueda";
    case FocusZone::Info:    return "Info";
    }
    return "";
}

void kbnav_draw_indicator() {
    if (!g_kbnav.active) return;
    const Theme& th = g_theme;

    char buf[40];
    // U+2328 KEYBOARD = UTF-8 E2 8C A8
    snprintf(buf, sizeof(buf), "\xE2\x8C\xA8 %s", zone_label(g_kbnav.zone));

    int fs = 11;
    int tw = MeasureTextF(buf, fs);
    int px = 8, py = g_split_y - 22;
    int bw = tw + 12, bh = 17;

    DrawRectangleRec({ (float)px, (float)py, (float)bw, (float)bh },
                     ColorAlpha(th.bg_app, 0.92f));
    DrawRectangleLinesEx({ (float)px, (float)py, (float)bw, (float)bh },
                         1.f, th.accent);
    DrawTextF(buf, px + 6, py + 3, fs, th.accent);
}
