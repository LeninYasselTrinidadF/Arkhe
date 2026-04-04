#pragma once
#include "raylib.h"

// ── search_widgets ────────────────────────────────────────────────────────────
// Primitivos de dibujo reutilizables para el panel de búsqueda.
// No tienen estado; todos los parámetros son explícitos.
// ─────────────────────────────────────────────────────────────────────────────

// Línea divisoria horizontal dentro del panel lateral
void search_draw_divider_h(int y);

// Campo de texto (solo visual, sin manejo de input)
void search_draw_field(int px, int y, int pw, int h, const char* buf);

// Botón "Buscar"; devuelve true si se hizo click
bool search_draw_button(const char* label, int x, int y, int w, int h, bool hov);

// Tarjeta de resultado (fondo + borde)
void search_draw_result_card(int px, int y, int pw, int h, bool hov, bool selected);

// Scrollbar vertical con drag y rueda.
// Devuelve el nuevo offset.
float search_draw_scrollbar(int sx, int area_top, int area_h,
    float cont_h, float offset,
    Vector2 mouse, bool in_area,
    bool& dragging, float& drag_start_y, float& drag_start_off);

// Paginador < N/M >; devuelve la página resultante.
int search_draw_pager(int px, int pw, int y,
    int page, int total_pages,
    Vector2 mouse, bool panel_active);
