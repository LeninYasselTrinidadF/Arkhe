#pragma once
#include "../app.hpp"
#include "../data/math_node.hpp"
#include "raylib.h"

// Dibuja el preview del sprite del nodo (esquina superior derecha del bloque).
void draw_sprite_preview(AppState& state,
                         MathNode* node,
                         int x, int y, int size);

// Dibuja el bloque "RECURSOS" con cards reales o placeholders.
// Devuelve la Y final.
int draw_resources_block(AppState& state,
                         MathNode* sel,
                         int col, int y,
                         int card_w, int card_h,
                         Vector2 mouse);
