#pragma once
#include "app.hpp"
#include "data/math_node.hpp"
#include "data/search/loogle.hpp"   // SearchHit, fuzzy_search
#include "ui/search_panel/search_layout.hpp"
#include "raylib.h"
#include <string>
#include <vector>

// ── LocalSearchList ───────────────────────────────────────────────────────────
// Gestiona la sección superior del panel de búsqueda:
//   · Campo de texto con foco (mouse / kbnav)
//   · Botón "Buscar" → carga completa de resultados
//   · Lista paginada con scrollbar
//   · Todo el estado vive en la instancia (sin estáticos)
// ─────────────────────────────────────────────────────────────────────────────

class LocalSearchList {
public:
    // y_start : primera fila disponible (bajo el divisor de cabecera)
    // y_bottom: límite inferior de la sección (= half_y)
    void draw(AppState& state, const MathNode* search_root,
              const SearchLayout& L, bool panel_active,
              Vector2 mouse, int y_start, int y_bottom);

private:
    float                  scroll      = 0.f;
    int                    page        = 0;
    bool                   full_loaded = false;
    std::string            last_query;
    std::vector<SearchHit> full_hits;
    std::vector<SearchHit> page_hits;
    bool                   focus       = true;

    // Drag state para la scrollbar
    bool  sb_drag = false;
    float sb_dy   = 0.f;
    float sb_doff = 0.f;
};
