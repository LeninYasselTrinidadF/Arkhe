#pragma once
#include <string>
#include "../../app.hpp"
#include "raylib.h"

// ── FocusZone ─────────────────────────────────────────────────────────────────
// Zonas de foco para navegación por teclado (Tab las cicla).
enum class FocusZone { Canvas, Toolbar, Search, Info };

// ── KbNavState ────────────────────────────────────────────────────────────────
struct KbNavState {
    bool       active     = false;           // activado en primer Tab / flecha
    FocusZone  zone       = FocusZone::Canvas;

    // ── Canvas ────────────────────────────────────────────────────────────────
    // Bubble view: 0 = burbuja central, 1..N = hijo i-ésimo
    int         canvas_idx  = 0;
    // Dep view: ID del nodo seleccionado
    std::string dep_sel_id;

    // ── Toolbar ───────────────────────────────────────────────────────────────
    // 0..3 = tabs (Ubicaciones/Docs/Editor/Config), 4 = botón tema
    int  toolbar_idx  = 0;

    // ── Search ────────────────────────────────────────────────────────────────
    // 0 = búsqueda local / fuzzy, 1 = loogle
    int  search_idx   = 0;

    // ── Info ──────────────────────────────────────────────────────────────────
    int  info_idx     = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    bool in(FocusZone z) const { return active && zone == z; }

    void activate(FocusZone z = FocusZone::Canvas) {
        active = true; zone = z;
    }
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
// Gestiona: Tab, +/−, P, M, N, S, E, F, y desactivación por ratón.
// Devuelve true si alguna tecla fue consumida.
bool kbnav_handle_global(AppState& state, Camera2D& cam, Camera2D& dep_cam);

// Dibuja un pequeño indicador "[KB] Zona" en la esquina del canvas.
// Llamar al final del frame (encima de todo lo demás).
void kbnav_draw_indicator();
