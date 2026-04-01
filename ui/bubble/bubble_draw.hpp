#pragma once
#include "../app.hpp"
#include "bubble_stats.hpp"
#include "raylib.h"
#include <string>
#include <vector>
#include <unordered_map>

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

// ── Sistema de etiquetas inteligente ─────────────────────────────────────────

// Resultado del cálculo de líneas para una burbuja.
struct BubbleLabelLines {
    std::vector<std::string> lines;   // 1–3 líneas listas para dibujar
    bool needs_abbrev = false;         // true → no cabe, usar abreviatura
};

// Calcula las líneas de texto para un label dado el radio y font_size.
// Reglas:
//   1-3 palabras → word-wrap en hasta 3 líneas (radio ya expandido por caller)
//   4-6 palabras → word-wrap + "..." si las últimas palabras no caben
//   7+ palabras o sin solución → needs_abbrev = true
BubbleLabelLines make_label_lines(const std::string& label, float radius,
    int font_size);

// Calcula el radio mínimo para que el label entre en multilínea, hasta max_r.
// Usar antes de make_label_lines para etiquetas cortas (1-3 palabras).
// max_r = base_r * 2.0 para 2-3 palabras; base_r * 3.0 para 1 palabra.
float fit_radius_for_label(const std::string& label, float base_r,
    float max_r, int font_size);

// Construye un mapa label→abreviatura única para un conjunto de labels.
// Resuelve colisiones añadiendo letras extra a la primera palabra significativa.
std::unordered_map<std::string, std::string>
build_child_abbrev_map(const std::vector<std::string>& labels);

// Versión legacy (una sola línea con truncado). Usar make_label_lines en su lugar.
std::string short_label(const std::string& label, float radius);

// Color del arco de progreso según modo y fracción completada.
Color arc_color(ViewMode mode, float progress);

// Dibuja un círculo (o nine-patch si hay skin) y superpone el sprite si existe.
void draw_bubble(AppState& state, float cx, float cy, float radius,
    Color col, const std::string& tex_key);