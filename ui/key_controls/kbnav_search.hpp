#pragma once
#include "../../app.hpp"
#include "../data/math_node.hpp"
#include "keyboard_nav.hpp"
#include "raylib.h"

// ── KbSearchState ─────────────────────────────────────────────────────────────
// Estado interno de la zona Search para navegación por teclado.
// El ciclo Tab cambia entre las dos sub-zonas:
//   search_idx == 0  → campo "Local (fuzzy)"
//   search_idx == 1  → campo "Loogle"
//
// Dentro de cada sub-zona:
//   Up/Down  → no aplica en el campo de texto; se reserva para futura lista.
//   Enter    → ejecutar búsqueda correspondiente.
//
// El módulo expone dos funciones:
//   kbnav_search_handle  – llama UNA VEZ por frame, ANTES de draw_search_panel.
//   kbnav_search_draw    – dibuja el indicador de foco activo (llamar DESPUÉS).

// Procesa teclado para la zona Search.
// Devuelve true si alguna tecla fue consumida.
bool kbnav_search_handle(AppState& state, const MathNode* search_root);

// Dibuja el borde de foco sobre el campo/botón activo.
// px, pw, field_y_local, field_y_loogle, field_h: geometría idéntica a draw_search_panel.
// btn_x, btn_w: posición del botón "Buscar" de Loogle.
void kbnav_search_draw(int px, int pw,
                       int field_y_local,
                       int field_y_loogle,
                       int field_h,
                       int btn_x, int btn_w);
