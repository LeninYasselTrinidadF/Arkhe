#pragma once
#include "app.hpp"
#include "raylib.h"

// Inicializa el layout force-directed centrado en `focus_id`.
// Si focus_id está vacío, usa el primer nodo del grafo.
// Llamar cuando dep_view_active pasa a true o cuando el usuario
// hace clic en un nodo para re-enfocar.
void dep_view_init(AppState& state, const std::string& focus_id);

// Igual que dep_view_init pero intenta resolver focus_id desde el nodo
// actualmente seleccionado en la jerarquía MSC (usa find_best).
// Devuelve true si encontró un nodo válido en el grafo.
bool dep_view_init_from_selection(AppState& state);

// Dibuja la vista de dependencias (reemplaza draw_bubble_view cuando
// state.dep_view_active == true).
// dep_cam es una Camera2D independiente de la del bubble_view.
void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse);

// Allow external toggling of position mode (when active, node clicks are for moving)
void dep_view_set_position_mode(AppState& state, bool on);
