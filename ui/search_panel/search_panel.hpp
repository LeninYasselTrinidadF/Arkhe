#pragma once
#include "app.hpp"
#include "data/math_node.hpp"
#include "ui/search_panel/local_search_list.hpp"
#include "ui/search_panel/loogle_list.hpp"
#include "raylib.h"

// ── SearchPanel ───────────────────────────────────────────────────────────────
// Panel lateral derecho fijo (no modal).
// Dibuja fondo, cabecera y divisor central; delega el contenido a
// LocalSearchList (mitad superior) y LoogleList (mitad inferior).
// ─────────────────────────────────────────────────────────────────────────────

class SearchPanel {
public:
    void draw(AppState& state, const MathNode* search_root, Vector2 mouse);

private:
    LocalSearchList local_list;
    LoogleList      loogle_list;
};

extern SearchPanel search_panel;
