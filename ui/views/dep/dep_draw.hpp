#pragma once
// ── dep_draw.hpp ──────────────────────────────────────────────────────────────
#include "app.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "raylib.h"
#include "ui/aesthetic/theme.hpp"
#include <string>

// ── NodeRole ──────────────────────────────────────────────────────────────────

enum class NodeRole { Focus, Upstream, Downstream, Other };

// Clasifica `id` respecto al nodo foco actual (s_focus_id).
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

// ── Selectores del modo pivote ────────────────────────────────────────────────

// Dibuja el anillo rojo pulsante alrededor del nodo pivote.
// Llamar dentro de BeginMode2D.
void dep_draw_pivot_node(const NodePhys& p, const Theme& th);

// Dibuja el anillo azul-rojo pulsante alrededor del nodo azul seleccionado
// dentro del modo pivote (cursor de navegación del modo pivote).
// Llamar dentro de BeginMode2D.
void dep_draw_pivot_selector(const NodePhys& p, const Theme& th);

// ── Utilidades ────────────────────────────────────────────────────────────────

// Trunca una cadena a max_chars con ellipsis UTF-8 si excede.
std::string dep_safe_trunc(const std::string& s, int max_chars);
