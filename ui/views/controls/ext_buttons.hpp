#pragma once
#include "app.hpp"
#include "raylib.h"

// Botones de conexión con herramientas externas.
// Compartidos entre bubble_view y dep_view.
void draw_vscode_button(AppState& state, Vector2 mouse);
void draw_mathlib_button(AppState& state, Vector2 mouse);