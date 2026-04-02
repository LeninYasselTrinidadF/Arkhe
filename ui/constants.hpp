#pragma once
#include "raylib.h"

// ── Altura del toolbar (fuente de verdad) ─────────────────────────────────────
// toolbar.hpp declara el mismo valor. Se repite aqui para evitar dependencia
// circular: constants.hpp <- toolbar.hpp <- app.hpp <- math_node.hpp ...
static constexpr int TOOLBAR_H = 34;

// Dimensiones dinamicas
inline int SW() { return GetScreenWidth(); }
inline int SH() { return GetScreenHeight(); }

// Divisor vertical arrastrable (coordenada Y absoluta de ventana).
// Valor inicial en main.cpp: TOOLBAR_H + 640.
extern int g_split_y;

// La zona util empieza justo debajo del toolbar.
inline int UI_TOP() { return TOOLBAR_H; }
// Altura del area de burbujas (entre toolbar y divisor).
inline int TOP_H() { return g_split_y - TOOLBAR_H; }
// Altura del info panel (entre divisor y fondo).
inline int BOTTOM_H() { return SH() - g_split_y; }

inline int PANEL_W() { return 420; }
inline int CANVAS_W() { return SW() - PANEL_W(); }
inline int CX() { return CANVAS_W() / 2; }
// Centro vertical del area de burbujas en coordenadas de ventana.
inline int CCY() { return UI_TOP() + TOP_H() / 2; } // Cambiado de CY() a CCY()