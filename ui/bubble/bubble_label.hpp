#pragma once
// ── bubble_label.hpp ──────────────────────────────────────────────────────────
// Sistema completo de cálculo y abreviatura de etiquetas.
// Extraído de bubble_draw para que esa unidad quede centrada en primitivos
// de dibujo puro (círculos, arcos, texturas).
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <unordered_map>

// ── Resultado de cálculo multilínea ──────────────────────────────────────────

struct BubbleLabelLines {
    std::vector<std::string> lines;
    bool needs_abbrev = false;   // true → no cabe, usar build_child_abbrev_map
};

// ── Display helpers ───────────────────────────────────────────────────────────

/// Inserta espacios en transiciones minúscula→Mayúscula y dígito→Mayúscula.
/// Solo para display (modo Mathlib); no modifica datos del nodo.
/// "LinearAlgebra" → "Linear Algebra", "smul_mk" → "smul_mk"
std::string split_camel(const std::string& s);

/// Número de palabras en s (split por whitespace).
int word_count(const std::string& s);

// ── Ajuste de radio ───────────────────────────────────────────────────────────

/// Radio mínimo para que el label entre en multilínea, hasta max_r.
/// Usar antes de make_label_lines para etiquetas cortas (≤3 palabras).
float fit_radius_for_label(const std::string& label, float base_r,
                           float max_r, int font_size);

// ── Cálculo de líneas ─────────────────────────────────────────────────────────

/// Calcula las líneas de texto para un label dado el radio y font_size.
/// 1-3 palabras → word-wrap hasta 3 líneas.
/// 4-6 palabras → word-wrap + "..." si no caben todas.
/// 7+ / sin solución → needs_abbrev = true.
BubbleLabelLines make_label_lines(const std::string& label, float radius,
                                  int font_size);

// ── Abreviaturas ──────────────────────────────────────────────────────────────

/// Construye label→abreviatura única para un conjunto de labels.
/// Resuelve colisiones añadiendo letras extra a la primera palabra significativa.
std::unordered_map<std::string, std::string>
build_child_abbrev_map(const std::vector<std::string>& labels);

/// Versión legacy de una sola línea. Preferir make_label_lines.
std::string short_label(const std::string& label, float radius);
