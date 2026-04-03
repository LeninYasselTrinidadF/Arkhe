#pragma once
#include <string>
#include "../../app.hpp"
#include "raylib.h"

// ── FocusZone ─────────────────────────────────────────────────────────────────
enum class FocusZone { Canvas, Toolbar, Search, Info };

// ── KbNavState ────────────────────────────────────────────────────────────────
struct KbNavState {
    bool       active = false;
    FocusZone  zone = FocusZone::Canvas;

    // Canvas
    int         canvas_idx = 0;
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
    // Nunca desactiva; solo cambia de zona
    void deactivate() { active = false; }

    void goto_zone(FocusZone z) {
        zone = z;
        canvas_idx = toolbar_idx = search_idx = info_idx = 0;
        dep_sel_id.clear();
    }
};

extern KbNavState g_kbnav;

// ── API ───────────────────────────────────────────────────────────────────────

// Llamar una vez por frame ANTES de los draw_* (en el bucle principal).
bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam);

// ── Indicador inline en toolbar ───────────────────────────────────────────────

// Llamar desde toolbar.cpp después de posicionar el botón de tema.
// Devuelve el ancho que necesita el indicador (0 si no está activo).
int  kbnav_query_indicator_width();

// toolbar.cpp informa la X donde debe dibujarse el indicador.
void kbnav_set_indicator_x(int x);

// Dibujar el indicador; llamar desde main.cpp al final del frame (sobre todo).
// El indicador aparece en el toolbar, a la derecha del botón de tema.
// Sin parpadeo: el borde es sólido mientras haya zona activa.
void kbnav_draw_indicator();