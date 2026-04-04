#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/views/dep_view.hpp"
#include "ui/views/controls/nav_sync.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "data/connect/vscode_bridge.hpp"
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

// ── Detección de zona por posición del mouse ──────────────────────────────────
// Devuelve la zona que corresponde geométricamente al punto dado.
// Se usa para que los clics actualicen la zona activa sin desactivar el sistema.

static FocusZone zone_from_mouse(Vector2 m) {
    // Toolbar: franja superior
    if (m.y < TOOLBAR_H)
        return FocusZone::Toolbar;

    // Panel de búsqueda: columna derecha superior (entre toolbar y split)
    if (m.x >= CANVAS_W() && m.y < g_split_y)
        return FocusZone::Search;

    // Info panel: toda la franja inferior
    if (m.y >= g_split_y)
        return FocusZone::Info;

    // Por defecto: canvas
    return FocusZone::Canvas;
}

// ── kbnav_handle_global ───────────────────────────────────────────────────────

bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam) {
    Vector2 mouse = GetMousePosition();
    bool consumed = false;
    Camera2D& active_cam = state.dep_view_active ? dep_cam : cam;

    // ── Clic: actualizar zona activa, nunca desactivar ────────────────────────
    // La zona cambia si el usuario hace clic, pero el sistema permanece activo.
    // Solo Tab puede cambiar la zona cuando el sistema ya está activo por teclado.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
        || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        FocusZone clicked = zone_from_mouse(mouse);
        if (!g_kbnav.active) {
            // Primera interacción: activar con la zona clickeada
            g_kbnav.activate(clicked);
        }
        else {
            // Ya activo: cambiar zona si el clic es en una zona diferente
            if (clicked != g_kbnav.zone)
                g_kbnav.goto_zone(clicked);
        }
        // No consumimos → los handlers normales de clic siguen funcionando
        return false;
    }

    // ── Tab: ciclar zona de foco ──────────────────────────────────────────────
    if (IsKeyPressed(KEY_TAB)) {
        if (!g_kbnav.active) {
            g_kbnav.activate(FocusZone::Canvas);
        }
        else {
            g_kbnav.goto_zone(zone_next(g_kbnav.zone));
        }
        consumed = true;
    }

    // ── Hotkeys de canvas: solo actúan cuando la zona activa es Canvas ───────
    // Si el foco está en Search o Info el usuario está escribiendo/navegando
    // en esos paneles, y estas teclas no deben interferir.
    bool canvas_zone = !g_kbnav.active || g_kbnav.zone == FocusZone::Canvas;

    // ── +/− zoom ──────────────────────────────────────────────────────────────
    if (canvas_zone) {
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            active_cam.zoom = Clamp(active_cam.zoom + 0.15f, 0.05f, 6.f);
            consumed = true;
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            active_cam.zoom = Clamp(active_cam.zoom - 0.15f, 0.05f, 6.f);
            consumed = true;
        }

        // ── P: modo posición ──────────────────────────────────────────────────
        if (IsKeyPressed(KEY_P)) {
            state.position_mode_active = !state.position_mode_active;
            consumed = true;
        }

        // ── M / N: cambio de modo ─────────────────────────────────────────────
        if (IsKeyPressed(KEY_M)) { state.mode = mode_cw(state.mode);  consumed = true; }
        if (IsKeyPressed(KEY_N)) { state.mode = mode_ccw(state.mode); consumed = true; }

        // ── S: conmutar Burbujas ↔ Dependencias ──────────────────────────────
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

        // ── E: abrir en VS Code ───────────────────────────────────────────────
        if (IsKeyPressed(KEY_E)) { bridge_launch_vscode(state);         consumed = true; }

        // ── F: abrir en Mathlib ───────────────────────────────────────────────
        if (IsKeyPressed(KEY_F)) { bridge_launch_vscode_mathlib(state); consumed = true; }

        // ── C: Casa — resetear cámara al origen y re-enfocar raíz del grafo ──
        if (IsKeyPressed(KEY_C)) {
            if (state.dep_view_active) {
                // En vista de dependencias: resetear cámara + volver al foco raíz
                active_cam.target = { 0.f, 0.f };
                active_cam.zoom = 1.f;
                // Re-inicializar desde la selección actual para centrar el foco
                dep_view_init_from_selection(state);
            }
            else {
                // En vista de burbujas: volver al nodo raíz del modo actual
                state.save_cam(cam);
                while (state.can_go_back()) state.pop();
                state.restore_cam(cam);
                cam.target = { 0.f, 0.f };
                cam.zoom = 1.f;
            }
            consumed = true;
        }
    }

    return consumed;
}

// ── kbnav_draw_indicator ──────────────────────────────────────────────────────
// El indicador se dibuja siempre que el sistema esté activo, sin parpadeo.
// Su posición es inline en el toolbar, a la derecha del botón de tema.
// toolbar.cpp debe llamar a kbnav_draw_indicator_inline() para obtener
// el ancho que ocupa, y luego llamar a kbnav_draw_indicator() al final del frame.
//
// Arquitectura: toolbar.cpp llama kbnav_query_indicator_width() para reservar
// espacio, y main.cpp llama kbnav_draw_indicator() para dibujarlo encima de todo.

static const char* zone_label(FocusZone z) {
    switch (z) {
    case FocusZone::Canvas:  return "Canvas";
    case FocusZone::Toolbar: return "Toolbar";
    case FocusZone::Search:  return "Busqueda";
    case FocusZone::Info:    return "Info";
    }
    return "";
}

// Posición X donde debe dibujarse el indicador en el toolbar.
// toolbar.cpp la setea después de posicionar el botón de tema.
static int s_indicator_x = 0;

void kbnav_set_indicator_x(int x) {
    s_indicator_x = x;
}

int kbnav_query_indicator_width() {
    if (!g_kbnav.active) return 0;
    // U+2328 KEYBOARD (UTF-8: E2 8C A8) + espacio + zona
    char buf[40];
    snprintf(buf, sizeof(buf), "\xE2\x8C\xA8 %s", zone_label(g_kbnav.zone));
    return MeasureTextF(buf, 11) + 20;  // padding 10 cada lado
}

void kbnav_draw_indicator() {
    if (!g_kbnav.active) return;
    const Theme& th = g_theme;

    char buf[40];
    snprintf(buf, sizeof(buf), "\xE2\x8C\xA8 %s", zone_label(g_kbnav.zone));

    const int fs = 11;
    const int bh = TOOLBAR_H - 6;
    const int by = 3;
    const int tw = MeasureTextF(buf, fs);
    const int bw = tw + 20;
    const int px = s_indicator_x;

    // Fondo con color de acento suave
    DrawRectangleRounded({ (float)px, (float)by, (float)bw, (float)bh },
        4.f / bh, 4,
        ColorAlpha(th.accent, 0.18f));
    // Borde sólido (sin parpadeo)
    DrawRectangleRoundedLinesEx({ (float)px, (float)by, (float)bw, (float)bh },
        4.f / bh, 4, 1.5f, th.accent);
    // Texto
    DrawTextF(buf, px + 10, by + (bh - fs) / 2, fs, th.accent);
}