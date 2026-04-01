#include "dep_sim.hpp"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <numeric>

// ── Estado global ─────────────────────────────────────────────────────────────

std::unordered_map<std::string, NodePhys> s_phys;
std::string s_focus_id;
int         s_sim_step = 0;
bool        s_ready = false;

// ── Helpers ───────────────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

// ── Layered (Sugiyama-style) initial placement ────────────────────────────────
//
// Asigna cada nodo a una "capa" según su distancia BFS desde el foco:
//   capa 0  = foco
//   capa -1, -2, ... = upstream  (nodos de los que depende el foco)
//   capa +1, +2, ... = downstream (nodos que dependen del foco)
//
// Los nodos se colocan en columnas verticales por capa.
// Dentro de cada capa se ordenan por heuristica barycenter para minimizar cruces.

static void layered_init(
    const std::unordered_map<std::string, bool>& visible_ids,
    const DepGraph& graph,
    const std::string& focus_id)
{
    // ── 1. Asignar capas via BFS bidireccional ────────────────────────────────
    std::unordered_map<std::string, int> layer;
    layer[focus_id] = 0;

    // BFS upstream (nodos de los que el foco depende): capas negativas
    {
        std::deque<std::string> q;
        q.push_back(focus_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            const DepNode* dn = graph.get(cur);
            if (!dn) continue;
            for (auto& dep : dn->depends_on) {
                if (!visible_ids.count(dep)) continue;
                if (!layer.count(dep)) {
                    layer[dep] = layer[cur] - 1;
                    q.push_back(dep);
                }
            }
        }
    }

    // BFS downstream (quién depende del foco): capas positivas
    {
        std::deque<std::string> q;
        q.push_back(focus_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            for (auto& down : graph.get_dependents(cur)) {
                if (!visible_ids.count(down)) continue;
                if (!layer.count(down)) {
                    layer[down] = layer[cur] + 1;
                    q.push_back(down);
                }
            }
        }
    }

    // Nodos no alcanzados por BFS: capa 0 con desplazamiento lateral pequeño
    for (auto& [id, _] : visible_ids)
        if (!layer.count(id)) layer[id] = 0;

    // ── 2. Agrupar nodos por capa ─────────────────────────────────────────────
    std::unordered_map<int, std::vector<std::string>> by_layer;
    for (auto& [id, l] : layer)
        by_layer[l].push_back(id);

    int min_layer = 0, max_layer = 0;
    for (auto& [l, _] : by_layer) {
        min_layer = std::min(min_layer, l);
        max_layer = std::max(max_layer, l);
    }

    // ── 3. Heuristica barycenter para reducir cruces (3 pasadas) ─────────────
    // Orden inicial estable
    for (auto& [l, nodes] : by_layer)
        std::sort(nodes.begin(), nodes.end());

    // Mapeo id -> posicion actual en su capa (actualizado cada pasada)
    std::unordered_map<std::string, float> cur_pos;
    auto refresh_pos = [&]() {
        cur_pos.clear();
        for (auto& [l, nodes] : by_layer)
            for (int i = 0; i < (int)nodes.size(); i++)
                cur_pos[nodes[i]] = (float)i;
        };
    refresh_pos();

    auto barycenter_pass = [&](bool forward) {
        int start = forward ? min_layer + 1 : max_layer - 1;
        int end = forward ? max_layer + 1 : min_layer - 1;
        int step = forward ? 1 : -1;

        for (int l = start; l != end; l += step) {
            auto it = by_layer.find(l);
            if (it == by_layer.end()) continue;
            auto& nodes = it->second;

            std::vector<std::pair<float, std::string>> scored;
            for (auto& id : nodes) {
                float sum = 0.f; int cnt = 0;
                // Vecinos en la capa de referencia (capa anterior en dirección)
                const DepNode* dn = graph.get(id);
                if (dn) {
                    for (auto& dep : dn->depends_on) {
                        auto p = cur_pos.find(dep);
                        if (p != cur_pos.end()) { sum += p->second; cnt++; }
                    }
                }
                for (auto& down : graph.get_dependents(id)) {
                    auto p = cur_pos.find(down);
                    if (p != cur_pos.end()) { sum += p->second; cnt++; }
                }
                float bc = (cnt > 0) ? sum / cnt : (float)scored.size() * 1000.f;
                scored.push_back({ bc, id });
            }
            std::stable_sort(scored.begin(), scored.end(),
                [](auto& a, auto& b) { return a.first < b.first; });
            for (int i = 0; i < (int)scored.size(); i++)
                nodes[i] = scored[i].second;
        }
        refresh_pos();
        };

    barycenter_pass(true);
    barycenter_pass(false);
    barycenter_pass(true);

    // ── 4. Asignar posiciones XY ──────────────────────────────────────────────
    // Capas a lo largo del eje X; nodos apilados en Y dentro de cada capa.
    const float LAYER_W = REST_LEN * 1.15f;  // separación entre capas
    const float NODE_H = 75.f;              // separación vertical entre nodos

    for (auto& [l, nodes] : by_layer) {
        int M = (int)nodes.size();
        for (int i = 0; i < M; i++) {
            auto it = s_phys.find(nodes[i]);
            if (it == s_phys.end()) continue;
            it->second.x = (float)l * LAYER_W + randf(-3.f, 3.f);
            it->second.y = (i - (M - 1) * 0.5f) * NODE_H + randf(-3.f, 3.f);
            it->second.vx = 0.f;
            it->second.vy = 0.f;
        }
    }
}

// ── dep_sim_init ──────────────────────────────────────────────────────────────

void dep_sim_init(AppState& state,
    const std::string& focus_id,
    const std::unordered_map<std::string, bool>& visible_ids)
{
    s_phys.clear();
    s_sim_step = 0;
    s_ready = false;
    s_focus_id = focus_id;

    for (auto& [id, _] : visible_ids)
        s_phys[id] = NodePhys{};

    const DepGraph& graph = get_dep_graph_for_const(state);
    layered_init(visible_ids, graph, focus_id);

    // Override con posiciones guardadas manualmente por el usuario
    std::string prefix =
        state.mode == ViewMode::MSC2020 ? "MSC:" :
        state.mode == ViewMode::Mathlib ? "ML:" : "STD:";

    for (auto& [k, v] : state.temp_positions) {
        if (k.rfind(prefix, 0) == 0) {
            std::string id = k.substr(prefix.size());
            auto it = s_phys.find(id);
            if (it != s_phys.end()) {
                it->second.x = v.x;
                it->second.y = v.y;
                it->second.vx = 0.f;
                it->second.vy = 0.f;
            }
        }
    }

    s_ready = true;
}

// ── dep_sim_step ──────────────────────────────────────────────────────────────
// El force-directed actua como refinador local sobre el layout jerarquico:
// solo corrige superposiciones y tensiones residuales, no resuelve el layout
// desde cero. Parametros mas suaves que el solver original.

void dep_sim_step(const AppState& state) {
    if (s_phys.size() < 2) { s_sim_step = SIM_MAX; return; }

    std::vector<std::string> ids;
    ids.reserve(s_phys.size());
    for (auto& [id, _] : s_phys) ids.push_back(id);

    // Cooling: fuerza decrece con el tiempo para estabilizar antes
    float t = (float)s_sim_step / SIM_MAX;
    float cooling = 1.0f - t * 0.7f;   // 1.0 → 0.3 a lo largo de la sim

    // ── Repulsion entre nodos cercanos ────────────────────────────────────────
    const float REPEL_CUTOFF2 = (REST_LEN * 1.6f) * (REST_LEN * 1.6f);
    for (int i = 0; i < (int)ids.size(); i++) {
        auto& a = s_phys[ids[i]];
        for (int j = i + 1; j < (int)ids.size(); j++) {
            auto& b = s_phys[ids[j]];
            float dx = b.x - a.x, dy = b.y - a.y;
            float d2 = dx * dx + dy * dy + 1.f;
            if (d2 > REPEL_CUTOFF2) continue;
            float d = sqrtf(d2);
            float f = (K_REPEL * cooling) / d2;
            float fx = f * dx / d, fy = f * dy / d;
            b.vx += fx; b.vy += fy;
            a.vx -= fx; a.vy -= fy;
        }
    }

    // ── Spring: solo actua sobre aristas muy largas (no comprime el layout) ───
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    const float SPRING_THRESHOLD = REST_LEN * 1.4f;
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;
            float dx = ib->second.x - ia->second.x;
            float dy = ib->second.y - ia->second.y;
            float d = sqrtf(dx * dx + dy * dy) + 0.001f;
            if (d < SPRING_THRESHOLD) continue;
            float stretch = (d - REST_LEN) / d;
            float f = K_ATTRACT * cooling * stretch;
            float fx = f * dx, fy = f * dy;
            ia->second.vx += fx; ia->second.vy += fy;
            ib->second.vx -= fx; ib->second.vy -= fy;
        }
    }

    // ── Atraccion debil del foco al origen ────────────────────────────────────
    auto fi = s_phys.find(s_focus_id);
    if (fi != s_phys.end()) {
        fi->second.vx -= fi->second.x * K_CENTER * cooling;
        fi->second.vy -= fi->second.y * K_CENTER * cooling;
    }

    // ── Integracion ───────────────────────────────────────────────────────────
    const float MAX_V = 30.f * cooling + 5.f;
    for (auto& [id, p] : s_phys) {
        p.vx *= DAMPING;
        p.vy *= DAMPING;
        float spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
        if (spd > MAX_V) { p.vx = p.vx / spd * MAX_V; p.vy = p.vy / spd * MAX_V; }
        p.x += p.vx;
        p.y += p.vy;
    }

    s_sim_step++;
}