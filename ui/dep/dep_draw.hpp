#pragma once
#include "../../app.hpp"
#include "dep_sim.hpp"
#include "raylib.h"
#include "../core/theme.hpp"
#include <string>

// ── NodeRole ──────────────────────────────────────────────────────────────────

enum class NodeRole { Focus, Upstream, Downstream, Other };

// Clasifica `id` respecto al nodo foco actual.
NodeRole dep_get_role(const AppState& state, const std::string& id);

// ── Primitivos de dibujo ──────────────────────────────────────────────────────

// Flecha bezier cúbica entre dos rectángulos (borde a borde).
void dep_draw_edge(Vector2 from, Vector2 to,
                   float w_from, float h_from,
                   float w_to,   float h_to,
                   Color col);

// Nodo con sombra, fondo redondeado, label y código.
void dep_draw_node(const std::string& id, const NodePhys& p,
                   NodeRole role, bool hov,
                   const Theme& th, const AppState& state);

// ── Utilidades ────────────────────────────────────────────────────────────────

// Trunca una cadena a max_chars con ellipsis UTF-8 si excede.
std::string dep_safe_trunc(const std::string& s, int max_chars);
