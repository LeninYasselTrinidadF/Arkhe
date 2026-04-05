#pragma once
#include "app.hpp"
#include <unordered_map>
#include <string>
#include <vector>

// ── NodePhys ──────────────────────────────────────────────────────────────────

struct NodePhys {
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    float w = 220.f;   // era 150 — más ancho para que entre el texto
    float h = 52.f;    // era 40  — más alto para dos líneas cómodas
};

// ── Constantes de simulación ──────────────────────────────────────────────────
// El layout radial (anillos BFS) da el esqueleto; el force-directed refina:
//   - Repulsión con separación mínima forzada
//   - Spring suave para aristas demasiado largas
//   - Anclaje radial (mantiene cada nodo cerca de su anillo BFS)
//   - Descruce de aristas (solo grafos pequeños, N ≤ 80)

static constexpr int   SIM_MAX = 360;        // +80 pasos extra para que el anclaje asiente
static constexpr float K_REPEL = 44000.f;
static constexpr float K_ATTRACT = 0.010f;
static constexpr float K_CENTER = 0.022f;     // ancla el foco al origen
static constexpr float K_UNCROSS = 0.38f;      // fuerza de descruce
static constexpr float K_RADIAL = 0.055f;     // anclaje radial (era 0.020 — muy débil)
static constexpr float DAMPING = 0.68f;
static constexpr float REST_LEN = 260.f;      // longitud de reposo base

// ── Parámetros del layout radial ──────────────────────────────────────────────
static constexpr float SUB_RING_GAP = 230.f;   // separación radial entre sub-anillos
static constexpr int   MAX_SUBRING_N = 9;        // máx nodos por arco de sub-anillo
static constexpr float MIN_ARC_PX = 185.f;   // separación mínima angular (px) entre nodos
// (era 164 — producía anillos demasiado juntos)

// ── Info de anillos para dibujo visual en dep_scene ──────────────────────────
struct RingVisInfo {
    int   ring_idx;       // distancia BFS con signo (−=upstream, +=downstream)
    float base_radius;    // radio del sub-anillo más interior
    int   num_subrings;   // cantidad de sub-anillos en este nivel
};

// ── Estado global ─────────────────────────────────────────────────────────────

extern std::unordered_map<std::string, NodePhys> s_phys;
extern std::string s_focus_id;
extern int         s_sim_step;
extern bool        s_ready;

extern std::vector<RingVisInfo>               s_ring_vis;      // para dibujo de círculos
extern std::unordered_map<std::string, float> s_node_target_r; // radio objetivo por nodo

// ── API ───────────────────────────────────────────────────────────────────────

void dep_sim_init(AppState& state,
    const std::string& focus_id,
    const std::unordered_map<std::string, bool>& visible_ids);

void dep_sim_step(const AppState& state);