#pragma once
// ── bubble_draw.hpp ───────────────────────────────────────────────────────────
// Primitivos de dibujo de burbujas (color, arcos, radio, textura).
// El sistema de etiquetas vive en bubble_label.hpp/cpp.
// ─────────────────────────────────────────────────────────────────────────────

#include "../app.hpp"
#include "bubble_stats.hpp"
#include "bubble_label.hpp"   // re-exportado para que los consumers no rompan
#include "raylib.h"
#include <string>

// Aplica oscuridad si el nodo no está conectado al crossref_map (factor 0.40).
Color bubble_color(Color base, const BubbleStats& st);

// Arco de progreso alrededor de un círculo. Empieza en -90° en sentido horario.
void draw_progress_arc(float cx, float cy, float r,
                       float progress, float thick, Color col);

// Radio entre [min_r, max_r] proporcional al peso respecto al máximo (sqrt).
float proportional_radius(int weight, int max_weight, float min_r, float max_r);

// Color del arco de progreso según modo y fracción completada.
Color arc_color(ViewMode mode, float progress);

// Dibuja un círculo (o nine-patch si hay skin) y superpone el sprite si existe.
void draw_bubble(AppState& state, float cx, float cy, float radius,
                 Color col, const std::string& tex_key);
