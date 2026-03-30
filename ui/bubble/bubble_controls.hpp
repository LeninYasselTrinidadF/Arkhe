#pragma once
#include "../app.hpp"
#include "raylib.h"

// ── Controles del canvas de burbujas ─────────────────────────────────────────

// Flecha triangular del switcher de modo.
// Devuelve true si se hizo click.
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);

// Barra central con nombre del modo + flechas.
// Cambia state.mode (eje de CONTENIDO: MSC2020 / Mathlib / Estándar).
// No toca dep_view_active.
void draw_mode_switcher(AppState& state, Vector2 mouse);

// Botones + / - de zoom y etiqueta de porcentaje.
void draw_zoom_buttons(Camera2D& cam, Vector2 mouse);

// Botones flotantes del canvas:
//   [Casa]        — sube a la raíz del árbol actual
//   Dependencias  — toggle que alterna draw_bubble_view ↔ draw_dep_view
//   < Atrás       — retrocede un nivel (solo en modo Burbujas)
//
// canvas_blocked: true cuando un panel flotante bloquea el canvas.
void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked);
