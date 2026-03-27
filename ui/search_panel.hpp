#pragma once
#include "../app.hpp"
#include "../data/math_node.hpp"
#include "constants.hpp"
#include "raylib.h"

// Dibuja el panel superior derecho: búsqueda local fuzzy + Loogle
void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse);
