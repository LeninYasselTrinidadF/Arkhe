#pragma once
#include "app.hpp"
#include "ui/search_panel/search_layout.hpp"
#include "raylib.h"

// ── LoogleList ────────────────────────────────────────────────────────────────
// Gestiona la sección inferior del panel de búsqueda:
//   · Campo de texto con foco + botón "Buscar"
//   · Disparo de loogle_search_async
//   · Estado de carga / error
//   · Lista paginada con scrollbar
//   · Todo el estado vive en la instancia (sin estáticos)
// ─────────────────────────────────────────────────────────────────────────────

class LoogleList {
public:
    // y_start : primera fila disponible (justo bajo el divisor de mitad)
    // y_bottom: límite inferior de la sección (= panel_top + TOP_H())
    void draw(AppState& state, const SearchLayout& L,
              bool panel_active, Vector2 mouse,
              int y_start, int y_bottom);

private:
    float loogle_scroll = 0.f;
    int   loogle_page   = 0;
    bool  focus         = false;

    // Drag state para la scrollbar
    bool  sb_drag = false;
    float sb_dy   = 0.f;
    float sb_doff = 0.f;
};
