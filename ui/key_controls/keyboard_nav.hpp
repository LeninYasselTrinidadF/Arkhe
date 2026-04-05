#pragma once
#include <string>
#include "app.hpp"
#include "raylib.h"

// Sub-módulos de zonas — incluir por separado según necesidad:
//   kbnav_views.hpp   — hotkeys canvas/dep_view
//   kbnav_search.hpp  — hotkeys zona Search
//   kbnav_info.hpp    — hotkeys zona Info

// ── FocusZone ─────────────────────────────────────────────────────────────────
enum class FocusZone { Canvas, Toolbar, Search, Info };

// ── KbNavState ────────────────────────────────────────────────────────────────
struct KbNavState {
    bool       active = false;
    FocusZone  zone = FocusZone::Canvas;

    // ── Canvas (vista burbujas) ───────────────────────────────────────────────
    // ring_idx = 0  → nodo central (padre actual)
    // ring_idx = 1  → anillo más interior
    // ring_idx = 2+ → anillos exteriores
    // slot_idx      → posición dentro del anillo (0-based, sentido horario)
    int  ring_idx = 0;
    int  slot_idx = 0;

    // Canvas (vista dependencias)
    std::string dep_sel_id;

    // Toolbar: 0..3 = tabs, 4 = botón tema
    int  toolbar_idx = 0;

    // Search: 0 = local/fuzzy, 1 = loogle
    int  search_idx = 0;

    // Info
    int  info_idx = 0;

    bool in(FocusZone z) const { return active && zone == z; }

    void activate(FocusZone z = FocusZone::Canvas) {
        active = true; zone = z;
    }
    void deactivate() { active = false; }

    void goto_zone(FocusZone z) {
        zone = z;
        ring_idx = 0;
        slot_idx = 0;
        toolbar_idx = search_idx = info_idx = 0;
        dep_sel_id.clear();
    }
};

extern KbNavState g_kbnav;

// ── API ───────────────────────────────────────────────────────────────────────

// Llamar una vez por frame ANTES de los draw_* (en el bucle principal).
bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam);

// ── Indicador inline en toolbar ───────────────────────────────────────────────

int  kbnav_query_indicator_width();
void kbnav_set_indicator_x(int x);
void kbnav_draw_indicator();