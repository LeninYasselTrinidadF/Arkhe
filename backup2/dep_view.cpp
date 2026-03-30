#include "dep_view.hpp"
#include "core/theme.hpp"
#include "core/font_manager.hpp"
#include "core/overlay.hpp"
#include "constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

// ── Estado interno (persiste entre frames para la simulación) ─────────────────

struct NodePhys {
    float x, y;       // posición centro
    float vx, vy;     // velocidad
    float w = 140.f;
    float h = 38.f;
};

static std::unordered_map<std::string, NodePhys> s_phys;
static std::string  s_focus_id;
static int          s_sim_step   = 0;
static bool         s_ready      = false;

static constexpr int   SIM_MAX    = 400;
static constexpr float K_REPEL    = 18000.f;
static constexpr float K_ATTRACT  = 0.03f;
static constexpr float K_CENTER   = 0.08f;   // fuerza del foco hacia el origen
static constexpr float DAMPING    = 0.82f;

// ── randf ─────────────────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

// ── dep_view_init ─────────────────────────────────────────────────────────────

void dep_view_init(AppState& state, const std::string& focus_id) {
    s_phys.clear();
    s_focus_id = focus_id;
    s_sim_step = 0;
    s_ready    = true;
    state.dep_focus_id = focus_id;

    if (state.dep_graph.empty()) return;

    // Nodos a mostrar: foco + sus deps directas + sus dependents directos
    std::unordered_set<std::string> ids;
    ids.insert(focus_id);

    const DepNode* focus = state.dep_graph.get(focus_id);
    if (focus) {
        for (auto& d : focus->depends_on)            ids.insert(d);
        for (auto& d : state.dep_graph.get_dependents(focus_id)) ids.insert(d);

        // Si el grafo local es muy pequeño, agregar deps de segundo grado
        if (ids.size() < 6) {
            for (auto& d1 : focus->depends_on) {
                const DepNode* n1 = state.dep_graph.get(d1);
                if (n1) for (auto& d2 : n1->depends_on) ids.insert(d2);
            }
        }
    }

    // Inicializar posiciones: foco en el centro, el resto en espiral
    float angle_step = 2.f * PI / std::max(1, (int)ids.size() - 1);
    float base_dist  = 220.f;
    int   i = 0;
    for (auto& id : ids) {
        NodePhys p{};
        if (id == focus_id) {
            p.x = p.y = 0.f;
        } else {
            float a = angle_step * i + randf(-0.2f, 0.2f);
            float d = base_dist + randf(-40.f, 40.f);
            p.x = cosf(a) * d;
            p.y = sinf(a) * d;
            i++;
        }
        s_phys[id] = p;
    }
}

// ── simulate_step ─────────────────────────────────────────────────────────────

static void simulate_step(const AppState& state) {
    if (s_phys.size() < 2) { s_sim_step = SIM_MAX; return; }

    std::vector<std::string> ids;
    ids.reserve(s_phys.size());
    for (auto& [id, _] : s_phys) ids.push_back(id);

    // Repulsión entre todos los pares
    for (int i = 0; i < (int)ids.size(); i++) {
        auto& a = s_phys[ids[i]];
        for (int j = i + 1; j < (int)ids.size(); j++) {
            auto& b = s_phys[ids[j]];
            float dx = b.x - a.x, dy = b.y - a.y;
            float d2 = dx * dx + dy * dy + 1.f;
            float d  = sqrtf(d2);
            float f  = K_REPEL / d2;
            float fx = f * dx / d, fy = f * dy / d;
            b.vx += fx;  b.vy += fy;
            a.vx -= fx;  a.vy -= fy;
        }
    }

    // Atracción a lo largo de las aristas
    for (auto& [id, node] : state.dep_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;
            float dx = ib->second.x - ia->second.x;
            float dy = ib->second.y - ia->second.y;
            ia->second.vx += K_ATTRACT * dx;
            ia->second.vy += K_ATTRACT * dy;
            ib->second.vx -= K_ATTRACT * dx;
            ib->second.vy -= K_ATTRACT * dy;
        }
    }

    // Fuerza centrípeta en el nodo foco
    auto fi = s_phys.find(s_focus_id);
    if (fi != s_phys.end()) {
        fi->second.vx -= fi->second.x * K_CENTER;
        fi->second.vy -= fi->second.y * K_CENTER;
    }

    // Integrar
    for (auto& [id, p] : s_phys) {
        p.vx *= DAMPING;
        p.vy *= DAMPING;
        p.x  += p.vx;
        p.y  += p.vy;
    }

    s_sim_step++;
}

// ── draw_dep_view ─────────────────────────────────────────────────────────────

void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse) {
    if (!s_ready) return;

    const Theme& th    = g_theme;
    const int cw       = CANVAS_W();
    const int ch       = TOP_H();
    bool in_canvas     = mouse.x < cw && mouse.y >= UI_TOP() && mouse.y < g_split_y;
    bool canvas_blocked = overlay::blocks_mouse(mouse)
        || state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open      || state.toolbar.config_open;

    // Simular varios pasos por frame hasta converger
    if (s_sim_step < SIM_MAX) {
        int steps_this_frame = (s_sim_step < SIM_MAX / 2) ? 8 : 3;
        for (int i = 0; i < steps_this_frame && s_sim_step < SIM_MAX; i++)
            simulate_step(state);
    }

    // ── Pan y zoom ────────────────────────────────────────────────────────────
    if (in_canvas && !canvas_blocked) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 d = GetMouseDelta();
            dep_cam.target.x -= d.x / dep_cam.zoom;
            dep_cam.target.y -= d.y / dep_cam.zoom;
        }
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.f && in_canvas && !canvas_blocked) {
        dep_cam.offset = mouse;
        dep_cam.target = GetScreenToWorld2D(mouse, dep_cam);
        dep_cam.zoom   = Clamp(dep_cam.zoom + wheel * 0.1f, 0.05f, 6.f);
    }
    dep_cam.offset = { (float)CX(), (float)CY() };

    BeginScissorMode(0, UI_TOP(), cw, ch);
    BeginMode2D(dep_cam);

    // ── Aristas ───────────────────────────────────────────────────────────────
    for (auto& [id, node] : state.dep_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;

            Vector2 from = { ia->second.x, ia->second.y };
            Vector2 to   = { ib->second.x, ib->second.y };

            // Punta de flecha simple
            Vector2 dir = Vector2Normalize(Vector2Subtract(to, from));
            Vector2 tip = Vector2Subtract(to, Vector2Scale(dir, ib->second.w * 0.5f + 4.f));
            Vector2 perp = { -dir.y * 5.f, dir.x * 5.f };

            DrawLineEx(from, tip, 1.5f, th_fade(th.ctrl_border, 0.65f));
            DrawTriangle(
                tip,
                Vector2Add(Vector2Subtract(tip, Vector2Scale(dir, 10.f)), perp),
                Vector2Subtract(Vector2Subtract(tip, Vector2Scale(dir, 10.f)), perp),
                th_fade(th.ctrl_border, 0.65f)
            );
        }
    }

    // ── Nodos ─────────────────────────────────────────────────────────────────
    Vector2 wm = in_canvas ? GetScreenToWorld2D(mouse, dep_cam) : Vector2{ -9999.f, -9999.f };

    for (auto& [id, p] : s_phys) {
        bool is_focus = (id == s_focus_id);
        Rectangle rect = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
        bool hov = !canvas_blocked && CheckCollisionPointRec(wm, rect);

        // Colores
        Color bg     = is_focus
                     ? th.accent
                     : (hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        Color border = is_focus ? th.success
                     : (hov    ? th.bubble_hover_ring : th.ctrl_border);
        Color text_c = is_focus ? th.bg_app : th.ctrl_text;

        // Sombra suave
        DrawRectangle((int)rect.x + 2, (int)rect.y + 2, (int)rect.width, (int)rect.height,
                      th_fade(BLACK, 0.35f));
        DrawRectangleRec(rect, bg);
        DrawRectangleLinesEx(rect, is_focus ? 2.f : 1.f, border);

        // Label
        const DepNode* dn  = state.dep_graph.get(id);
        std::string    lbl = dn ? dn->label : id;
        if ((int)lbl.size() > 18) lbl = lbl.substr(0, 17) + ".";
        int tw = MeasureTextF(lbl.c_str(), 15);
        DrawTextF(lbl.c_str(), (int)(p.x - tw * 0.5f), (int)(p.y - 8), 15, text_c);

        // Click para re-enfocar
        if (hov && !is_focus && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dep_view_init(state, id);
            dep_cam.target = { 0.f, 0.f };
            dep_cam.zoom   = 1.f;
            EndMode2D();
            EndScissorMode();
            return;
        }
    }

    EndMode2D();
    EndScissorMode();

    // ── HUD: título + botón Volver ────────────────────────────────────────────
    {
        // Título del nodo foco
        const DepNode* fn = state.dep_graph.get(s_focus_id);
        std::string title = "Dependencias: " + (fn ? fn->label : s_focus_id);
        DrawTextF(title.c_str(), CANVAS_W() / 2 - MeasureTextF(title.c_str(), 20) / 2,
                  UI_TOP() + 8, 20, th.ctrl_text);

        // Botón "< Volver"
        float bx = 14.f, by = (float)(UI_TOP() + 10), bw = 90.f, bh = 32.f;
        bool  hov = CheckCollisionPointRec(mouse, { bx, by, bw, bh }) && !canvas_blocked;
        DrawRectangleRec({ bx, by, bw, bh }, hov ? th.bg_button_hover : th_alpha(th.bg_button));
        DrawRectangleLinesEx({ bx, by, bw, bh }, 1.f, th.ctrl_border);
        DrawTextF("< Volver", (int)bx + 8, (int)by + 7, 19,
                  hov ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            state.dep_view_active = false;
    }

    // ── Indicador de simulación ───────────────────────────────────────────────
    if (s_sim_step < SIM_MAX) {
        char buf[32];
        snprintf(buf, sizeof(buf), "sim %d%%", s_sim_step * 100 / SIM_MAX);
        DrawTextF(buf, 14, g_split_y - 30, 13, th_alpha(th.text_dim));
    }

    // ── Info del grafo ────────────────────────────────────────────────────────
    {
        char info[64];
        snprintf(info, sizeof(info), "%d nodos  %zu en vista",
                 (int)state.dep_graph.size(), s_phys.size());
        int iw = MeasureTextF(info, 13);
        DrawTextF(info, CANVAS_W() - iw - 8, g_split_y - 20, 13, th_alpha(th.text_dim));
    }
}
