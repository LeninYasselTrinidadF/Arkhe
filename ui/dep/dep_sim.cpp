#include "dep_sim.hpp"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

// ── Estado global ─────────────────────────────────────────────────────────────

std::unordered_map<std::string, NodePhys> s_phys;
std::string s_focus_id;
int         s_sim_step = 0;
bool        s_ready    = false;

// ── Helpers internos ──────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

// ── dep_sim_init ──────────────────────────────────────────────────────────────

void dep_sim_init(AppState& state,
                  const std::string& focus_id,
                  const std::unordered_map<std::string, bool>& visible_ids)
{
    s_phys.clear();
    s_sim_step = 0;
    s_ready    = false;
    s_focus_id = focus_id;

    // Posiciones iniciales: foco al centro, resto en anillos
    int   n_others   = (int)visible_ids.size() - 1;
    float angle_step = (n_others > 0) ? (2.f * 3.14159265f / n_others) : 0.f;
    float base_dist  = REST_LEN * 0.85f;
    int   i          = 0;

    for (auto& [id, _] : visible_ids) {
        NodePhys p{};
        if (id == focus_id) {
            p.x = p.y = 0.f;
        } else {
            float a = angle_step * i + randf(-0.15f, 0.15f);
            float d = base_dist + randf(-30.f, 30.f);
            p.x = cosf(a) * d;
            p.y = sinf(a) * d;
            i++;
        }
        s_phys[id] = p;
    }

    // Aplicar posiciones temporales guardadas para este modo
    std::string prefix =
        state.mode == ViewMode::MSC2020  ? "MSC:" :
        state.mode == ViewMode::Mathlib  ? "ML:"  : "STD:";

    for (auto& [k, v] : state.temp_positions) {
        if (k.rfind(prefix, 0) == 0) {
            std::string id = k.substr(prefix.size());
            auto it = s_phys.find(id);
            if (it != s_phys.end()) {
                it->second.x = v.x;
                it->second.y = v.y;
            }
        }
    }

    s_ready = true;
}

// ── dep_sim_step ──────────────────────────────────────────────────────────────

void dep_sim_step(const AppState& state) {
    if (s_phys.size() < 2) { s_sim_step = SIM_MAX; return; }

    std::vector<std::string> ids;
    ids.reserve(s_phys.size());
    for (auto& [id, _] : s_phys) ids.push_back(id);

    // ── Repulsión ─────────────────────────────────────────────────────────────
    for (int i = 0; i < (int)ids.size(); i++) {
        auto& a = s_phys[ids[i]];
        for (int j = i + 1; j < (int)ids.size(); j++) {
            auto& b  = s_phys[ids[j]];
            float dx = b.x - a.x, dy = b.y - a.y;
            float d2 = dx * dx + dy * dy + 1.f;
            float d  = sqrtf(d2);
            float f  = K_REPEL / d2;
            float fx = f * dx / d, fy = f * dy / d;
            b.vx += fx; b.vy += fy;
            a.vx -= fx; a.vy -= fy;
        }
    }

    // ── Spring (longitud de reposo) ───────────────────────────────────────────
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;
            float dx      = ib->second.x - ia->second.x;
            float dy      = ib->second.y - ia->second.y;
            float d       = sqrtf(dx * dx + dy * dy) + 0.001f;
            float stretch = (d - REST_LEN) / d;
            float fx = K_ATTRACT * stretch * dx;
            float fy = K_ATTRACT * stretch * dy;
            ia->second.vx += fx;
            ia->second.vy += fy;
            ib->second.vx -= fx;
            ib->second.vy -= fy;
        }
    }

    // ── Centrípeto sobre el foco ──────────────────────────────────────────────
    auto fi = s_phys.find(s_focus_id);
    if (fi != s_phys.end()) {
        fi->second.vx -= fi->second.x * K_CENTER;
        fi->second.vy -= fi->second.y * K_CENTER;
    }

    // ── Integración + clamp ───────────────────────────────────────────────────
    const float MAX_V = 80.f;
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
