#include "ui/views/dep/dep_pivot.hpp"
#include "ui/views/dep/dep_sim.hpp"       // s_phys, s_focus_id
#include "ui/key_controls/keyboard_nav.hpp" // g_kbnav, FocusZone
#include "data/loaders/dep_graph.hpp"
#include "raylib.h"
#include <deque>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

// ── Estado global ─────────────────────────────────────────────────────────────

DepPivotState g_dep_pivot;

// ── dep_pivot_rebuild ─────────────────────────────────────────────────────────
// BFS bidireccional desde pivot_id dentro del conjunto visible (s_phys).
// Los nodos upstream reciben distancia negativa, downstream positiva.

void dep_pivot_rebuild(const AppState& state) {
    g_dep_pivot.rings.clear();
    g_dep_pivot.rings_valid = false;

    if (g_dep_pivot.pivot_id.empty()) return;
    if (s_phys.find(g_dep_pivot.pivot_id) == s_phys.end()) return;

    const DepGraph& graph = get_dep_graph_for_const(state);

    // dist < 0: upstream (nodos de los que el pivote depende)
    // dist > 0: downstream (nodos que dependen del pivote)
    std::unordered_map<std::string, int> dist;
    dist[g_dep_pivot.pivot_id] = 0;

    // BFS upstream (dist negativa)
    {
        std::deque<std::string> q;
        q.push_back(g_dep_pivot.pivot_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            const DepNode* dn = graph.get(cur);
            if (!dn) continue;
            for (auto& dep : dn->depends_on) {
                if (s_phys.find(dep) == s_phys.end()) continue;
                if (dist.count(dep)) continue;
                dist[dep] = dist[cur] - 1;
                q.push_back(dep);
            }
        }
    }

    // BFS downstream (dist positiva)
    {
        std::deque<std::string> q;
        q.push_back(g_dep_pivot.pivot_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            for (auto& down : graph.get_dependents(cur)) {
                if (s_phys.find(down) == s_phys.end()) continue;
                if (dist.count(down)) continue;
                dist[down] = dist[cur] + 1;
                q.push_back(down);
            }
        }
    }

    // Nodos visibles no alcanzados: no se incluyen en ningún anillo
    // (están en s_phys pero no son vecinos del pivote en el grafo)

    // Agrupar por distancia (excluyendo el pivote mismo, dist==0)
    std::unordered_map<int, std::vector<std::string>> by_dist;
    for (auto& [id, d] : dist) {
        if (d == 0) continue;
        by_dist[d].push_back(id);
    }

    // Ordenar los slots de cada anillo alfabéticamente (estable entre sesiones)
    for (auto& [d, ids] : by_dist)
        std::sort(ids.begin(), ids.end());

    // Construir vector de rings ordenado por ring_idx
    for (auto& [d, ids] : by_dist) {
        DepPivotRing ring;
        ring.ring_idx = d;
        ring.slots    = ids;
        g_dep_pivot.rings.push_back(ring);
    }
    std::sort(g_dep_pivot.rings.begin(), g_dep_pivot.rings.end(),
        [](const DepPivotRing& a, const DepPivotRing& b) {
            return a.ring_idx < b.ring_idx;
        });

    g_dep_pivot.rings_valid = !g_dep_pivot.rings.empty();

    // Clamp cur_ring y cur_slot al nuevo contenido
    if (g_dep_pivot.rings_valid) {
        // Si cur_ring ya no existe, elegir el más cercano a +1
        bool found = false;
        for (auto& r : g_dep_pivot.rings)
            if (r.ring_idx == g_dep_pivot.cur_ring) { found = true; break; }
        if (!found) {
            // Preferir anillo +1 (downstream directo); si no existe, el primero
            g_dep_pivot.cur_ring = g_dep_pivot.rings[0].ring_idx;
            for (auto& r : g_dep_pivot.rings)
                if (r.ring_idx == 1) { g_dep_pivot.cur_ring = 1; break; }
        }
        // Clamp slot
        for (auto& r : g_dep_pivot.rings) {
            if (r.ring_idx == g_dep_pivot.cur_ring) {
                if (g_dep_pivot.cur_slot >= (int)r.slots.size())
                    g_dep_pivot.cur_slot = 0;
                break;
            }
        }
    }
}

// ── dep_pivot_selected_id ─────────────────────────────────────────────────────

std::string dep_pivot_selected_id() {
    if (!g_dep_pivot.active || !g_dep_pivot.rings_valid) return "";
    for (auto& r : g_dep_pivot.rings) {
        if (r.ring_idx == g_dep_pivot.cur_ring) {
            if (g_dep_pivot.cur_slot < (int)r.slots.size())
                return r.slots[g_dep_pivot.cur_slot];
            return "";
        }
    }
    return "";
}

// ── dep_pivot_handle_keys ─────────────────────────────────────────────────────

bool dep_pivot_handle_keys(bool& out_repivot) {
    out_repivot = false;
    if (!g_dep_pivot.active || !g_dep_pivot.rings_valid) return false;

    bool consumed = false;

    // Helper: índice del ring actual en el vector
    auto ring_vec_idx = [&]() -> int {
        for (int i = 0; i < (int)g_dep_pivot.rings.size(); i++)
            if (g_dep_pivot.rings[i].ring_idx == g_dep_pivot.cur_ring) return i;
        return -1;
    };

    auto slots_of_ring = [&](int ring_idx) -> int {
        for (auto& r : g_dep_pivot.rings)
            if (r.ring_idx == ring_idx) return (int)r.slots.size();
        return 0;
    };

    // ── Izquierda → anillo más negativo (upstream más lejano) ─────────────────
    if (IsKeyPressed(KEY_LEFT)) {
        int vi = ring_vec_idx();
        if (vi > 0) {
            g_dep_pivot.cur_ring = g_dep_pivot.rings[vi - 1].ring_idx;
            int s = slots_of_ring(g_dep_pivot.cur_ring);
            if (g_dep_pivot.cur_slot >= s) g_dep_pivot.cur_slot = 0;
        }
        consumed = true;
    }

    // ── Derecha → anillo más positivo (downstream más lejano) ─────────────────
    if (IsKeyPressed(KEY_RIGHT)) {
        int vi = ring_vec_idx();
        if (vi >= 0 && vi < (int)g_dep_pivot.rings.size() - 1) {
            g_dep_pivot.cur_ring = g_dep_pivot.rings[vi + 1].ring_idx;
            int s = slots_of_ring(g_dep_pivot.cur_ring);
            if (g_dep_pivot.cur_slot >= s) g_dep_pivot.cur_slot = 0;
        }
        consumed = true;
    }

    // ── Arriba → slot anterior (anti-horario) ─────────────────────────────────
    if (IsKeyPressed(KEY_UP)) {
        int s = slots_of_ring(g_dep_pivot.cur_ring);
        if (s > 0)
            g_dep_pivot.cur_slot = (g_dep_pivot.cur_slot - 1 + s) % s;
        consumed = true;
    }

    // ── Abajo → slot siguiente (horario) ──────────────────────────────────────
    if (IsKeyPressed(KEY_DOWN)) {
        int s = slots_of_ring(g_dep_pivot.cur_ring);
        if (s > 0)
            g_dep_pivot.cur_slot = (g_dep_pivot.cur_slot + 1) % s;
        consumed = true;
    }

    // ── Enter → re-pivotar al nodo azul seleccionado ──────────────────────────
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        std::string sel = dep_pivot_selected_id();
        if (!sel.empty()) {
            // El nuevo pivote es el nodo azul seleccionado
            g_dep_pivot.pivot_id   = sel;
            g_dep_pivot.cur_ring   = 1;   // intentar arrancar en downstream directo
            g_dep_pivot.cur_slot   = 0;
            g_dep_pivot.rings_valid = false;
            out_repivot = true;
        }
        consumed = true;
    }

    return consumed;
}

// ── dep_pivot_activate ────────────────────────────────────────────────────────

void dep_pivot_activate(const AppState& state) {
    if (g_dep_pivot.active) return;   // ya activo, no re-inicializar
    g_dep_pivot.active = true;

    // El pivote rojo arranca en el nodo azul actualmente seleccionado
    std::string start = g_kbnav.dep_sel_id;
    if (start.empty() || s_phys.find(start) == s_phys.end())
        start = s_focus_id;
    g_dep_pivot.pivot_id    = start;
    g_dep_pivot.cur_ring    = 1;
    g_dep_pivot.cur_slot    = 0;
    g_dep_pivot.rings_valid = false;

    dep_pivot_rebuild(state);
}

// ── dep_pivot_deactivate ──────────────────────────────────────────────────────

void dep_pivot_deactivate() {
    g_dep_pivot.active      = false;
    g_dep_pivot.rings_valid = false;
}
