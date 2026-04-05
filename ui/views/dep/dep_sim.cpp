#include "ui/views/dep/dep_sim.hpp"
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

std::vector<RingVisInfo>               s_ring_vis;
std::unordered_map<std::string, float> s_node_target_r;

static float s_rest_len = REST_LEN;

// Constante local para evitar dependencia de M_PI (no siempre definido en MSVC)
static constexpr float PI_F = 3.14159265358979323846f;

// ── Helpers ───────────────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

struct Seg { float x0, y0, x1, y1; std::string id_a, id_b; };

static Seg make_seg(const std::string& a, const std::string& b) {
    const NodePhys& pa = s_phys.at(a);
    const NodePhys& pb = s_phys.at(b);
    return { pa.x, pa.y, pb.x, pb.y, a, b };
}

static bool seg_intersect(const Seg& s1, const Seg& s2, float& t, float& u) {
    if (s1.id_a == s2.id_a || s1.id_a == s2.id_b ||
        s1.id_b == s2.id_a || s1.id_b == s2.id_b) return false;
    float dx1 = s1.x1 - s1.x0, dy1 = s1.y1 - s1.y0;
    float dx2 = s2.x1 - s2.x0, dy2 = s2.y1 - s2.y0;
    float denom = dx1 * dy2 - dy1 * dx2;
    if (fabsf(denom) < 1e-6f) return false;
    float ex = s2.x0 - s1.x0, ey = s2.y0 - s1.y0;
    t = (ex * dy2 - ey * dx2) / denom;
    u = (ex * dy1 - ey * dx1) / denom;
    return t > 0.01f && t < 0.99f && u > 0.01f && u < 0.99f;
}

// ── radial_init ───────────────────────────────────────────────────────────────
// Layout en anillos concéntricos BFS con sub-anillos y barycenter angular.
//
//  Upstream  (d < 0): nodos de los que depende el foco → hemisferio izquierdo
//  Downstream (d > 0): nodos que dependen del foco   → hemisferio derecho
//
//  Sub-anillos: cuando un nivel BFS tiene más de MAX_SUBRING_N nodos, los
//  excedentes se colocan en un arco exterior a radio += SUB_RING_GAP, etc.
//
//  Barycenter angular iterativo: se procesa de menor a mayor |d|, por lo que
//  al ordenar el anillo abs_d sus vecinos en anillos internos ya están
//  colocados y se puede usar su ángulo como guía para reducir cruces.

static void radial_init(
    const std::unordered_map<std::string, bool>& visible_ids,
    const DepGraph& graph,
    const std::string& focus_id)
{
    // ── 1. BFS con distancia de anillo con signo ───────────────────────────────
    std::unordered_map<std::string, int> ring_dist;
    ring_dist[focus_id] = 0;

    {   // upstream: nodos de los que depende el foco (distancia negativa)
        std::deque<std::string> q;
        q.push_back(focus_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            const DepNode* dn = graph.get(cur);
            if (!dn) continue;
            for (auto& dep : dn->depends_on) {
                if (!visible_ids.count(dep) || ring_dist.count(dep)) continue;
                ring_dist[dep] = ring_dist[cur] - 1;
                q.push_back(dep);
            }
        }
    }
    {   // downstream: nodos que dependen del foco (distancia positiva)
        std::deque<std::string> q;
        q.push_back(focus_id);
        while (!q.empty()) {
            auto cur = q.front(); q.pop_front();
            for (auto& down : graph.get_dependents(cur)) {
                if (!visible_ids.count(down) || ring_dist.count(down)) continue;
                ring_dist[down] = ring_dist[cur] + 1;
                q.push_back(down);
            }
        }
    }
    // Nodos no alcanzados por BFS → anillo 0 junto al foco
    for (auto& [id, _] : visible_ids)
        if (!ring_dist.count(id)) ring_dist[id] = 0;

    // ── 2. Agrupar por anillo ─────────────────────────────────────────────────
    std::unordered_map<int, std::vector<std::string>> by_ring;
    for (auto& [id, d] : ring_dist)
        by_ring[d].push_back(id);
    for (auto& [d, ids] : by_ring)
        std::sort(ids.begin(), ids.end());   // orden alfabético inicial estable

    int max_abs = 0;
    for (auto& [d, _] : by_ring) max_abs = std::max(max_abs, abs(d));

    // ── 3. Calcular radios por anillo ─────────────────────────────────────────
    // Upstream y downstream acumulan sus radios por separado para no pisarse.
    //
    // FIX: el radio mínimo se calcula a partir del arco real disponible
    // (2 * ANG_HALF), no de PI completo. Antes: inner_n * MIN_ARC_PX / PI_F
    // → subestimaba el radio necesario, apilando nodos.

    // ANG_HALF definido aquí para que la fórmula de min_r lo use.
    const float ANG_HALF = PI_F * 0.48f;  // 86.4° cada lado; arco total ≈ 172.8°
    // era 0.46 (82.8°) — da más margen angular

    s_ring_vis.clear();
    s_node_target_r.clear();

    const float MIN_RING_STEP = s_rest_len * 1.45f;

    std::unordered_map<int, float> ring_base_r;
    ring_base_r[0] = 0.f;

    float up_outer = 0.f;   // radio exterior acumulado upstream
    float down_outer = 0.f;   // radio exterior acumulado downstream

    for (int abs_d = 1; abs_d <= max_abs; abs_d++) {
        for (int sign : { 1, -1 }) {
            int d = abs_d * sign;
            auto it = by_ring.find(d);
            if (it == by_ring.end()) continue;

            int N = (int)it->second.size();
            int nsub = (N + MAX_SUBRING_N - 1) / MAX_SUBRING_N;
            int inner_n = std::min(N, MAX_SUBRING_N);

            // Radio mínimo para que los `inner_n` nodos del sub-anillo interior
            // quepan con separación mínima MIN_ARC_PX sobre el arco 2*ANG_HALF.
            float min_r = (inner_n * MIN_ARC_PX) / (2.f * ANG_HALF);

            float& accum = (sign > 0) ? down_outer : up_outer;
            float base = std::max(accum + MIN_RING_STEP, min_r);
            ring_base_r[d] = base;
            accum = base + (nsub - 1) * SUB_RING_GAP;

            s_ring_vis.push_back({ d, base, nsub });
        }
    }

    // ── 4. Colocar foco en el origen ──────────────────────────────────────────
    {
        auto& p = s_phys[focus_id];
        p.x = p.y = 0.f;
        p.vx = p.vy = 0.f;
    }
    s_node_target_r[focus_id] = 0.f;

    // ── 5. Colocar nodos de cada anillo (orden creciente de |d|) ─────────────
    // Procesando de adentro hacia afuera, al ordenar el anillo abs_d sus vecinos
    // internos ya están colocados → barycenter angular válido.

    for (int abs_d = 1; abs_d <= max_abs; abs_d++) {
        for (int sign : { 1, -1 }) {
            int d = abs_d * sign;
            auto it = by_ring.find(d);
            if (it == by_ring.end()) continue;

            auto& ids = it->second;
            int N = (int)ids.size();
            float base_r = ring_base_r.count(d) ? ring_base_r[d] : s_rest_len;

            // Ángulo central del hemisferio asignado
            float ang_center = (d < 0) ? PI_F : 0.f;

            // ── Barycenter angular sort ───────────────────────────────────────
            // Ordena los nodos del anillo por el ángulo promedio de sus
            // vecinos ya colocados (anillos con |dist| estrictamente menor).
            if (N > 1) {
                std::vector<std::pair<float, std::string>> scored;
                scored.reserve(N);
                for (auto& id : ids) {
                    float sum_a = 0.f;
                    int   cnt = 0;

                    auto accum_nb = [&](const std::string& nb_id) {
                        auto ph = s_phys.find(nb_id);
                        if (ph == s_phys.end()) return;
                        auto rd = ring_dist.find(nb_id);
                        if (rd == ring_dist.end()) return;
                        if (abs(rd->second) >= abs_d) return;  // solo anillos internos
                        sum_a += atan2f(ph->second.y, ph->second.x);
                        cnt++;
                        };

                    const DepNode* dn = graph.get(id);
                    if (dn) for (auto& dep : dn->depends_on) accum_nb(dep);
                    for (auto& down : graph.get_dependents(id))  accum_nb(down);

                    // Fallback: si no hay vecinos internos, mantener orden actual
                    float bc = cnt > 0
                        ? sum_a / cnt
                        : ang_center + (float)scored.size() * 0.001f;
                    scored.push_back({ bc, id });
                }
                std::stable_sort(scored.begin(), scored.end(),
                    [](auto& a, auto& b) { return a.first < b.first; });
                for (int i = 0; i < N; i++) ids[i] = scored[i].second;
            }

            // ── Asignar posiciones cartesianas ────────────────────────────────
            for (int i = 0; i < N; i++) {
                int sub = i / MAX_SUBRING_N;
                int in_sub = i % MAX_SUBRING_N;
                int n_sub = std::min(MAX_SUBRING_N, N - sub * MAX_SUBRING_N);

                float r = base_r + sub * SUB_RING_GAP;
                s_node_target_r[ids[i]] = r;

                float angle;
                if (n_sub == 1)
                    angle = ang_center;
                else
                    angle = ang_center - ANG_HALF
                    + 2.f * ANG_HALF * (float)in_sub / (float)(n_sub - 1);

                auto ph = s_phys.find(ids[i]);
                if (ph == s_phys.end()) continue;
                ph->second.x = r * cosf(angle) + randf(-4.f, 4.f);
                ph->second.y = r * sinf(angle) + randf(-4.f, 4.f);
                ph->second.vx = ph->second.vy = 0.f;
            }
        }
    }
}

// ── dep_sim_init ──────────────────────────────────────────────────────────────

void dep_sim_init(AppState& state,
    const std::string& focus_id,
    const std::unordered_map<std::string, bool>& visible_ids)
{
    s_phys.clear();
    s_ring_vis.clear();
    s_node_target_r.clear();
    s_sim_step = 0;
    s_ready = false;
    s_focus_id = focus_id;

    for (auto& [id, _] : visible_ids)
        s_phys[id] = NodePhys{};

    // REST_LEN ligeramente adaptativo (el layout radial ya escala los radios)
    int N = (int)visible_ids.size();
    s_rest_len = REST_LEN * (1.f + 0.018f * std::max(0, N - 10));
    s_rest_len = std::min(s_rest_len, REST_LEN * 2.2f);

    const DepGraph& graph = get_dep_graph_for_const(state);

    // ── Anchos dinámicos: cada nodo es tan ancho como su texto necesite ───────
    // Estimación: font 15 ≈ 8.5 px/char, font 11 ≈ 7.0 px/char.
    // Ancho = max(label_px, code_px) + padding. Clampado [150, 360].
    {
        constexpr float CHAR_W_LABEL = 8.5f;
        constexpr float CHAR_W_CODE = 7.0f;
        constexpr float PAD_X = 28.f;
        constexpr float MIN_W = 150.f;
        constexpr float MAX_W = 360.f;
        constexpr float NODE_H = 52.f;

        for (auto& [id, phys] : s_phys) {
            const DepNode* dn = graph.get(id);
            const std::string& label = dn ? dn->label : id;
            float wlabel = (float)label.size() * CHAR_W_LABEL + PAD_X;
            float wcode = (float)id.size() * CHAR_W_CODE + PAD_X;
            phys.w = std::clamp(std::max(wlabel, wcode), MIN_W, MAX_W);
            phys.h = NODE_H;
        }
    }

    radial_init(visible_ids, graph, focus_id);

    // Override con posiciones manuales guardadas por el usuario
    std::string prefix =
        state.mode == ViewMode::MSC2020 ? "MSC:" :
        state.mode == ViewMode::Mathlib ? "ML:" : "STD:";

    for (auto& [k, v] : state.temp_positions) {
        if (k.rfind(prefix, 0) != 0) continue;
        std::string id = k.substr(prefix.size());
        auto it = s_phys.find(id);
        if (it != s_phys.end()) {
            it->second.x = v.x;
            it->second.y = v.y;
            it->second.vx = it->second.vy = 0.f;
            // No anclar nodos con posición guardada manualmente
            s_node_target_r.erase(id);
        }
    }

    // Para grafos grandes el layout radial ya es suficiente.
    // La simulación no mejora el resultado: la repulsión O(N²) destruye la
    // estructura de anillos más rápido de lo que el anclaje puede restaurarla.
    if (N > 60) s_sim_step = SIM_MAX;

    s_ready = true;
}

// ── dep_sim_step ──────────────────────────────────────────────────────────────

void dep_sim_step(const AppState& state) {
    if (s_phys.size() < 2) { s_sim_step = SIM_MAX; return; }

    std::vector<std::string> ids;
    ids.reserve(s_phys.size());
    for (auto& [id, _] : s_phys) ids.push_back(id);
    const int N = (int)ids.size();

    float t_norm = (float)s_sim_step / SIM_MAX;
    float cooling = 1.0f - t_norm * 0.80f;   // 1.0 → 0.20

    // ── 1. Repulsión nodo-nodo ─────────────────────────────────────────────────
    const float REPEL_CUTOFF2 = (s_rest_len * 2.0f) * (s_rest_len * 2.0f);

    for (int i = 0; i < N; i++) {
        auto& a = s_phys[ids[i]];
        for (int j = i + 1; j < N; j++) {
            auto& b = s_phys[ids[j]];
            float dx = b.x - a.x, dy = b.y - a.y;
            float d2 = dx * dx + dy * dy + 1.f;

            // Separación mínima dinámica: semisuma de anchos reales + gap 20px
            float min_sep = (a.w + b.w) * 0.5f + 20.f;

            if (d2 < min_sep * min_sep) {
                // Separación forzada: solapamiento
                float d = sqrtf(d2);
                float ovl = (min_sep - d) / d;
                float fx = dx * ovl * 0.5f;
                float fy = dy * ovl * 0.5f;
                b.vx += fx * 1.9f; b.vy += fy * 1.9f;
                a.vx -= fx * 1.9f; a.vy -= fy * 1.9f;
            }
            else if (d2 < REPEL_CUTOFF2) {
                float d = sqrtf(d2);
                float f = (K_REPEL * cooling) / d2;
                float nx = dx / d, ny = dy / d;
                b.vx += f * nx; b.vy += f * ny;
                a.vx -= f * nx; a.vy -= f * ny;
            }
        }
    }

    // ── 2. Spring: solo aristas demasiado largas ───────────────────────────────
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    const float SPRING_THRESH = s_rest_len * 1.25f;

    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;
            float dx = ib->second.x - ia->second.x;
            float dy = ib->second.y - ia->second.y;
            float d = sqrtf(dx * dx + dy * dy) + 0.001f;
            if (d < SPRING_THRESH) continue;
            float stretch = (d - s_rest_len) / d;
            float f = K_ATTRACT * cooling * stretch;
            ia->second.vx += f * dx; ia->second.vy += f * dy;
            ib->second.vx -= f * dx; ib->second.vy -= f * dy;
        }
    }

    // ── 3. Anclaje radial ────────────────────────────────────────────────────
    // FIX: el anclaje arranca con más fuerza (0.30 en lugar de 0.15) para que
    // la repulsión no tenga tiempo de dispersar los nodos lejos de sus anillos
    // antes de que el anclaje tome control.
    //   t_norm=0 → factor = 0.30
    //   t_norm=1 → factor = 1.00
    float radial_str = K_RADIAL * (0.30f + 0.70f * t_norm);
    for (auto& [id, target_r] : s_node_target_r) {
        if (id == s_focus_id) continue;
        auto it = s_phys.find(id);
        if (it == s_phys.end()) continue;
        auto& p = it->second;
        float r = sqrtf(p.x * p.x + p.y * p.y);
        if (r < 0.5f) { p.x += randf(-6.f, 6.f); continue; }
        float err = (r - target_r) / r;
        float f = radial_str * err;
        p.vx -= f * p.x;
        p.vy -= f * p.y;
    }

    // ── 4. Fuerza de descruce ─────────────────────────────────────────────────
    // Solo para grafos pequeños (≤ 80 nodos) y primera mitad de la simulación,
    // dado que el layout radial ya minimiza cruces estructuralmente.
    if (N <= 80 && cooling > 0.40f && (s_sim_step % 2 == 0)) {
        std::vector<Seg> segs;
        segs.reserve(use_graph.nodes().size() * 2);
        for (auto& [id, node] : use_graph.nodes()) {
            if (s_phys.find(id) == s_phys.end()) continue;
            for (auto& dep_id : node.depends_on) {
                if (s_phys.find(dep_id) == s_phys.end()) continue;
                segs.push_back(make_seg(id, dep_id));
            }
        }
        for (int si = 0; si < (int)segs.size(); si++) {
            for (int sj = si + 1; sj < (int)segs.size(); sj++) {
                float t, u;
                if (!seg_intersect(segs[si], segs[sj], t, u)) continue;
                float cx = segs[si].x0 + t * (segs[si].x1 - segs[si].x0);
                float cy = segs[si].y0 + t * (segs[si].y1 - segs[si].y0);
                auto push_away = [&](const std::string& nid) {
                    auto it = s_phys.find(nid);
                    if (it == s_phys.end()) return;
                    float dx = it->second.x - cx, dy = it->second.y - cy;
                    float d = sqrtf(dx * dx + dy * dy) + 0.001f;
                    float f = K_UNCROSS * cooling / d;
                    it->second.vx += f * dx;
                    it->second.vy += f * dy;
                    };
                push_away(segs[si].id_a); push_away(segs[si].id_b);
                push_away(segs[sj].id_a); push_away(segs[sj].id_b);
            }
        }
    }

    // ── 5. Atracción del foco al origen ───────────────────────────────────────
    auto fi = s_phys.find(s_focus_id);
    if (fi != s_phys.end()) {
        fi->second.vx -= fi->second.x * K_CENTER * cooling;
        fi->second.vy -= fi->second.y * K_CENTER * cooling;
    }

    // ── 6. Integración ────────────────────────────────────────────────────────
    const float MAX_V = 30.f * cooling + 4.f;
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