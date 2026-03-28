#pragma once
#include "../app.hpp"
#include "../data/math_node.hpp"
#include "raylib.h"
#include <vector>
#include <string>

// ── Tipos ─────────────────────────────────────────────────────────────────────

struct MathlibHit {
    std::string module;
    std::string match_kind;   // "MSC" o "STD"
    std::string matched_code;
};

// ── Búsquedas ─────────────────────────────────────────────────────────────────

// Devuelve módulos Mathlib que referencian `code` en el crossref_map.
// use_msc=true → busca en ref.msc; false → en ref.standard.
std::vector<MathlibHit>
find_mathlib_hits(const std::unordered_map<std::string, CrossRef>& cmap,
                  const std::string& code,
                  bool use_msc);

// Rellena los vectores de refs MSC y Standard activos para el nodo sel.
void collect_active_codes(AppState& state,
                          MathNode* sel,
                          std::vector<std::string>& out_msc,
                          std::vector<std::string>& out_std);

// Devuelve el código más representativo del contexto actual.
std::string get_context_code(AppState& state, MathNode* sel);

// ── Dibujo ────────────────────────────────────────────────────────────────────

// Bloque "Módulos Mathlib relacionados (via MSC/Estándar)".
// Devuelve la Y final.
int draw_inverse_block(AppState& state,
                       MathNode* sel,
                       const std::vector<MathlibHit>& hits,
                       int col, int y, int card_w,
                       Vector2 mouse);

// Bloque "Referencias cruzadas" (cards MSC + STD).
// Devuelve la Y final.
int draw_crossrefs_block(AppState& state,
                         const std::vector<std::string>& msc_refs,
                         const std::vector<std::string>& std_refs,
                         int col, int y,
                         int card_w, int card_h, int panel_w,
                         Vector2 mouse);
