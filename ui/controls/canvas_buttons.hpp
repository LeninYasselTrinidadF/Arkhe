#pragma once
#include "../../app.hpp"
#include "raylib.h"

// Botones del canvas en modo BURBUJAS:
//   [Casa] / Dependencias (toggle) / Posicion / < Atrás
void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked);

// Botones del canvas en modo DEPENDENCIAS:
//   [Casa] / Burbujas (volver) / Posicion / < Atrás (nodo padre en grafo)
// dep_cam se pasa para resetear la cámara al pulsar [Casa].
void draw_dep_canvas_buttons(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool canvas_blocked);