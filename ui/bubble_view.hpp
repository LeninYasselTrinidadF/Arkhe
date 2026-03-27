#pragma once
#include "../app.hpp"
#include "constants.hpp"
#include "raylib.h"

// draw_bubble_view maneja internamente pan y zoom
void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse);
void draw_mode_switcher(AppState& state, Vector2 mouse);
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);
