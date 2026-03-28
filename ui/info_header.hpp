#pragma once
#include "../app.hpp"
#include "raylib.h"

// Dibuja la cabecera del panel inferior:
//   - breadcrumb de navegación
//   - título del nodo seleccionado
//   - chips de código / nivel / modo
//
// top  : coordenada Y donde empieza el panel inferior (g_split_y)
// w    : ancho total de la ventana (SW())
void draw_info_header(AppState& state, int top, int w);
