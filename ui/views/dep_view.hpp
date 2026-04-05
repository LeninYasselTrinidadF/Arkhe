#pragma once
#include "app.hpp"
#include "raylib.h"

// Inicializa el layout force-directed centrado en `focus_id`.
// Empuja el foco anterior al historial de navegación antes de cambiar.
void dep_view_init(AppState& state, const std::string& focus_id);

// Igual que dep_view_init pero resuelve focus_id desde el nodo seleccionado
// en la jerarquía MSC. Limpia el historial (punto de entrada externo).
// Devuelve true si encontró un nodo válido en el grafo.
bool dep_view_init_from_selection(AppState& state);

// Retrocede al nodo anterior en el historial de navegación.
// Resetea dep_cam a origen/zoom=1. Devuelve true si había historia.
// Llamar desde el botón "< Atrás" o desde dep_input_handle (Backspace).
bool dep_view_back(AppState& state, Camera2D& dep_cam);

// Devuelve true si hay al menos un paso atrás disponible.
// Útil para deshabilitar/atenuar visualmente el botón "< Atrás".
bool dep_view_has_history();

// Dibuja la vista de dependencias.
void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse);

// Activa/desactiva el modo posición (drag de nodos).
void dep_view_set_position_mode(AppState& state, bool on);