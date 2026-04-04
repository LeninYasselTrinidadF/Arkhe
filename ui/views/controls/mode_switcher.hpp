#pragma once
#include "app.hpp"
#include "raylib.h"

// Flecha triangular del switcher de modo.
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);

// Barra central con nombre del modo activo + flechas izq/der para cambiarlo.
void draw_mode_switcher(AppState& state, Vector2 mouse);