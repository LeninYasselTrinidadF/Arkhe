#pragma once
#include "app.hpp"
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
// El force-directed actua como refinador sobre el layout jerarquico inicial,
// no como solver principal. Parametros ajustados para correcciones locales.

static constexpr int   SIM_MAX = 300;    // menos iteraciones: el layout ya es bueno
static constexpr float K_REPEL = 18000.f;
static constexpr float K_ATTRACT = 0.018f;
static constexpr float K_CENTER = 0.04f;
static constexpr float DAMPING = 0.75f;  // damping mas agresivo → converge rapido
static constexpr float REST_LEN = 260.f;

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