#pragma once
// ── dep_scene.hpp ─────────────────────────────────────────────────────────────
// Dibujo de la escena 2D en dep_view:
//   - Fondo con grid
//   - Aristas (con resaltado de pivote)
//   - Nodos con selectores (azul geográfico + rojo pivote)
//   - Drag de nodos en modo posición
//
// dep_scene_draw llama internamente a BeginMode2D / EndMode2D.
// Devuelve el id del nodo clicado (vacío si ninguno).
// ─────────────────────────────────────────────────────────────────────────────
#include "app.hpp"
#include "raylib.h"
#include <string>

std::string dep_scene_draw(AppState& state, Camera2D& dep_cam,
                           Vector2 mouse, bool in_canvas, bool canvas_blocked);
