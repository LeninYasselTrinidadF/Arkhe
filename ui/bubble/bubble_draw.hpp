#pragma once
#include "../app.hpp"
#include "bubble_stats.hpp"
#include "raylib.h"
#include <string>

// ── Primitivos de dibujo de burbujas ─────────────────────────────────────────
// Usados tanto por bubble_view como (en el futuro) por dep_view u otras vistas.

// Aplica oscuridad si el nodo no está conectado al crossref_map (factor 0.40).
Color bubble_color(Color base, const BubbleStats& st);

// Arco de progreso alrededor de un círculo de radio `r`.
// Empieza en -90° (top) en sentido horario. No dibuja nada si progress <= 0.001.
void draw_progress_arc(float cx, float cy, float r,
                       float progress, float thick, Color col);

// Radio entre [min_r, max_r] proporcional al peso respecto al máximo.
// Usa sqrt para suavizar extremos.
float proportional_radius(int weight, int max_weight, float min_r, float max_r);

// Trunca `label` para que quepa visualmente dentro de `radius` px.
std::string short_label(const std::string& label, float radius);

// Color del arco de progreso según modo y fracción completada.
Color arc_color(ViewMode mode, float progress);

// Dibuja un círculo (o nine-patch si hay skin) y superpone el sprite si existe.
void draw_bubble(AppState& state, float cx, float cy, float radius,
                 Color col, const std::string& tex_key);
