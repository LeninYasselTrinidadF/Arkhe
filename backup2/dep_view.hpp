#pragma once
#include "../app.hpp"
#include "raylib.h"

// Inicializa el layout force-directed centrado en `focus_id`.
// Llamar cuando dep_view_active pasa a true o cuando el usuario
// hace clic en un nodo para re-enfocar.
void dep_view_init(AppState& state, const std::string& focus_id);

// Dibuja la vista de dependencias (reemplaza draw_bubble_view cuando
// state.dep_view_active == true).
// dep_cam es una Camera2D independiente de la del bubble_view.
void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse);
