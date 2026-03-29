#pragma once
#include "constants.hpp"
#include "../app.hpp"
#include "raylib.h"

// Dibuja el toolbar estilo VSCode en y=0.
// Devuelve true si se cambiaron rutas y hay que recargar assets.
bool draw_toolbar(AppState& state, Vector2 mouse);

// Paneles flotantes (wrappers para uso externo si se necesitan)
void draw_docs_panel(AppState& state, Vector2 mouse);
void draw_entry_editor(AppState& state, Vector2 mouse);
void draw_ubicaciones_panel(AppState& state, Vector2 mouse);
void draw_config_panel(AppState& state, Vector2 mouse);
