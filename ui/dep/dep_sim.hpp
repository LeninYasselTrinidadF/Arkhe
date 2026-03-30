#pragma once
#include "../../app.hpp"
#include <unordered_map>
#include <string>

// ── NodePhys ──────────────────────────────────────────────────────────────────
// Estado físico de un nodo en el layout force-directed.

struct NodePhys {
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    float w = 150.f;
    float h = 40.f;
};

// ── Constantes de simulación ──────────────────────────────────────────────────

static constexpr int   SIM_MAX   = 500;
static constexpr float K_REPEL   = 22000.f;
static constexpr float K_ATTRACT = 0.025f;
static constexpr float K_CENTER  = 0.06f;
static constexpr float DAMPING   = 0.80f;
static constexpr float REST_LEN  = 260.f;

// ── Estado global del simulador ───────────────────────────────────────────────

extern std::unordered_map<std::string, NodePhys> s_phys;
extern std::string s_focus_id;
extern int         s_sim_step;
extern bool        s_ready;

// ── API ───────────────────────────────────────────────────────────────────────

// Inicializa posiciones de nodos y resetea la simulación.
// Llamar desde dep_view_init después de colectar los nodos visibles.
void dep_sim_init(AppState& state,
                  const std::string& focus_id,
                  const std::unordered_map<std::string, bool>& visible_ids);

// Avanza un paso de la simulación (repulsión + spring + centrípeto + integración).
void dep_sim_step(const AppState& state);
