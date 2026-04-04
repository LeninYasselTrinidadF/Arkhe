#pragma once
#include "app.hpp"
#include "ui/constants.hpp"
#include "raylib.h"

// Vista principal de burbujas (pan/zoom interno)
void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse);

// Switcher de modo (barra central con flechas) — llamado desde main.cpp
void draw_mode_switcher(AppState& state, Vector2 mouse);

// Flecha triangular usada por el switcher
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);
