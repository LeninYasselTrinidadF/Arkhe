#pragma once
#include "../app.hpp"
#include "raylib.h"

// ── Controles del canvas de burbujas ─────────────────────────────────────────

// Flecha triangular del switcher de modo.
// Devuelve true si se hizo click.
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);

// Barra central con nombre del modo + flechas de cambio.
void draw_mode_switcher(AppState& state, Vector2 mouse);

// Botones + / - de zoom y etiqueta de porcentaje.
void draw_zoom_buttons(Camera2D& cam, Vector2 mouse);

// Botones flotantes del canvas: Casa, Dependencias, Atrás.
// canvas_blocked: true cuando un panel flotante bloquea el canvas.
void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked);
