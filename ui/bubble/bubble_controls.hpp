#pragma once
#include "../app.hpp"
#include "raylib.h"

// ── Controles compartidos del canvas ─────────────────────────────────────────
// Usados tanto por bubble_view como por dep_view para mantener
// consistencia visual y de interacción entre ambas vistas.

// Flecha triangular del switcher de modo.
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse);

// Barra central con nombre del modo + flechas (eje de CONTENIDO).
void draw_mode_switcher(AppState& state, Vector2 mouse);

// Botones +/- de zoom y etiqueta de porcentaje.
// Funciona con cualquier Camera2D (burbuja o dependencias).
void draw_zoom_buttons(Camera2D& cam, Vector2 mouse);


// Botones de conexión con VS Code (solo en modo burbuja, eje de CONTROLES).
void draw_vscode_button(AppState& state, Vector2 mouse);

void draw_mathlib_button(AppState& state, Vector2 mouse);


// Botones del canvas en modo BURBUJAS:
//   [Casa] / Dependencias (toggle) / Posicion / < Atrás
void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked);

// Botones del canvas en modo DEPENDENCIAS:
//   [Casa] / Burbujas (volver al árbol) / Posicion / < Atrás (nodo padre en grafo)
// dep_cam se pasa para resetear cámara al pulsar [Casa].
void draw_dep_canvas_buttons(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool canvas_blocked);