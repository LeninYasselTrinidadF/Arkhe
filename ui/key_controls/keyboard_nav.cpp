#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/key_controls/kbnav_views/kbnav_views.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "raylib.h"

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

static FocusZone zone_from_mouse(Vector2 m) {
    if (m.y < TOOLBAR_H)
        return FocusZone::Toolbar;
    if (m.x >= CANVAS_W() && m.y < g_split_y)
        return FocusZone::Search;
    if (m.y >= g_split_y)
        return FocusZone::Info;
    return FocusZone::Canvas;
}

// ── kbnav_handle_global ───────────────────────────────────────────────────────
// Orquesta zonas de foco y delega hotkeys al módulo correspondiente.
// Orden de responsabilidades:
//   1. Clics  → actualizar zona activa (no consumen evento)
//   2. Tab    → ciclar zona
//   3. Canvas → delegar a kbnav_views_handle
// Las zonas Search e Info se manejan en sus propios módulos
// (kbnav_search_handle, kbnav_info_handle), llamados desde main.cpp.

bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam) {
    bool consumed = false;

    // ── Clic: actualizar zona activa ──────────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
        || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
    {
        FocusZone clicked = zone_from_mouse(GetMousePosition());
        if (!g_kbnav.active)
            g_kbnav.activate(clicked);
        else if (clicked != g_kbnav.zone)
            g_kbnav.goto_zone(clicked);
        // No consumas: los handlers de clic normales deben seguir funcionando.
        return false;
    }

    // ── Paneles modales del toolbar: bloquean todas las hotkeys de vista ─────
    const bool modal_open = state.toolbar.ubicaciones_open
        || state.toolbar.docs_open
        || state.toolbar.editor_open
        || state.toolbar.config_open;

    if (modal_open)
        return consumed;   // los paneles manejan su propio input; no pasar nada más

    // ── Tab: ciclar zona ──────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_TAB)) {
        if (!g_kbnav.active)
            g_kbnav.activate(FocusZone::Canvas);
        else
            g_kbnav.goto_zone(zone_next(g_kbnav.zone));
        consumed = true;
    }

    // ── Canvas: hotkeys de vistas ─────────────────────────────────────────────
    if (kbnav_views_handle(state, cam, dep_cam))
        consumed = true;

    return consumed;
}

// ── Indicador de zona activa (dibujado en el toolbar) ─────────────────────────

static const char* zone_label(FocusZone z) {
    switch (z) {
    case FocusZone::Canvas:  return "Canvas";
    case FocusZone::Toolbar: return "Toolbar";
    case FocusZone::Search:  return "Busqueda";
    case FocusZone::Info:    return "Info";
    }
    return "";
}

static int s_indicator_x = 0;

void kbnav_set_indicator_x(int x) { s_indicator_x = x; }

int kbnav_query_indicator_width() {
    if (!g_kbnav.active) return 0;
    char buf[40];
    snprintf(buf, sizeof(buf), "\xE2\x8C\xA8 %s", zone_label(g_kbnav.zone));
    return MeasureTextF(buf, 11) + 20;
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

    DrawRectangleRounded({ (float)px, (float)by, (float)bw, (float)bh },
        4.f / bh, 4, ColorAlpha(th.accent, 0.18f));
    DrawRectangleRoundedLinesEx({ (float)px, (float)by, (float)bw, (float)bh },
        4.f / bh, 4, 1.5f, th.accent);
    DrawTextF(buf, px + 10, by + (bh - fs) / 2, fs, th.accent);
}