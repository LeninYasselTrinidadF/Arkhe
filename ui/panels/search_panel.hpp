#pragma once
#include "../../app.hpp"
#include "../../data/math_node.hpp"
#include "../../search/loogle.hpp"   // SearchHit, loogle_search_async
#include "../constants.hpp"
#include "raylib.h"
#include <string>
#include <vector>

// ── SearchPanel ───────────────────────────────────────────────────────────────
// Panel lateral derecho fijo (no modal).  Gestiona búsqueda fuzzy local y
// Loogle remoto.  Todo el estado de scroll/página/foco vive en la instancia;
// no hay estáticos globales.
// ─────────────────────────────────────────────────────────────────────────────

class SearchPanel {
public:
    void draw(AppState& state, const MathNode* search_root, Vector2 mouse);

private:
    // ── Estado búsqueda local ─────────────────────────────────────────────
    float                  local_scroll      = 0.f;
    int                    local_page        = 0;
    bool                   local_full_loaded = false;
    std::string            local_last_query;
    std::vector<SearchHit> local_full_hits;
    std::vector<SearchHit> local_page_hits;

    // ── Estado búsqueda Loogle ────────────────────────────────────────────
    float loogle_scroll = 0.f;
    int   loogle_page   = 0;

    // ── Foco de campos de texto ───────────────────────────────────────────
    bool local_focus  = true;
    bool loogle_focus = false;

    // ── Estado drag scrollbar local ───────────────────────────────────────
    bool  sb_drag    = false;
    float sb_dy      = 0.f;
    float sb_doff    = 0.f;

    // ── Estado drag scrollbar loogle ──────────────────────────────────────
    bool  ls_drag    = false;
    float ls_dy      = 0.f;
    float ls_doff    = 0.f;
};

// Global instance used by main.cpp and other callers
extern SearchPanel search_panel;
