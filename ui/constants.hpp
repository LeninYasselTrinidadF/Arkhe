#pragma once
#include "raylib.h"

// Dimensiones dinamicas
inline int SW() { return GetScreenWidth(); }
inline int SH() { return GetScreenHeight(); }

// Divisor vertical arrastrable — valor por defecto 680px desde arriba
// Se modifica desde main.cpp via g_split_y
extern int g_split_y;

inline int TOP_H() { return g_split_y; }
inline int BOTTOM_H() { return SH() - g_split_y; }
inline int PANEL_W() { return 420; }
inline int CANVAS_W() { return SW() - PANEL_W(); }
inline int CX() { return CANVAS_W() / 2; }
inline int CY() { return TOP_H() / 2; }