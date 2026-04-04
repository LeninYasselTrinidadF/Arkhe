#pragma once
#include "raylib.h"

// Botones +/- de zoom con etiqueta de porcentaje.
// Funciona con cualquier Camera2D (burbuja o dependencias).
void draw_zoom_buttons(Camera2D& cam, Vector2 mouse);