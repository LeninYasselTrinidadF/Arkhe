#include "dep_view.hpp"
#include "data/position_state.hpp"
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

// ── Estado interno ────────────────────────────────────────────────────────────

struct NodePhys {
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    float w = 150.f;
    float h = 40.f;
};

static std::unordered_map<std::string, NodePhys> s_phys;
static std::string  s_focus_id;
static int          s_sim_step  = 0;
static bool         s_ready     = false;

static constexpr int   SIM_MAX   = 500;
static constexpr float K_REPEL   = 22000.f;
static constexpr float K_ATTRACT = 0.025f;
static constexpr float K_CENTER  = 0.06f;
static constexpr float DAMPING   = 0.80f;
// Distancia de reposo de aristas (para que los nodos no queden aplastados)
static constexpr float REST_LEN  = 260.f;

// ── Utilidades ────────────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

// Truncar label de forma segura (sin tocar chars de multibyte incorrectamente)
static std::string safe_trunc(const std::string& s, int max_chars) {
    if ((int)s.size() <= max_chars) return s;
    // Retroceder hasta un espacio si es posible
    int cut = max_chars - 1;
    while (cut > 4 && s[cut] != ' ') cut--;
    return s.substr(0, std::max(cut, max_chars - 1)) + "\xE2\x80\xA6"; // U+2026 ellipsis UTF-8
}

// ── dep_view_init ─────────────────────────────────────────────────────────────

void dep_view_init(AppState& state, const std::string& focus_id) {
    s_phys.clear();
    s_sim_step = 0;
    s_ready    = false;

    // Use dep graph corresponding to the current mode
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty()) return;

    // Validar que focus_id no sea vacío ni tenga caracteres peligrosos
    if (focus_id.empty() || focus_id.size() > 64) return;

    s_focus_id = focus_id;
    state.dep_focus_id = focus_id;

    // ── Colectar nodos visibles ────────────────────────────────────────────────
    std::unordered_set<std::string> ids;
    ids.insert(focus_id);

    const DepNode* focus = use_graph.get(focus_id);
    if (focus) {
        for (auto& d : focus->depends_on) {
            if (!d.empty() && d.size() < 64) ids.insert(d);
        }
        for (auto& d : state.dep_graph.get_dependents(focus_id)) {
            if (!d.empty() && d.size() < 64) ids.insert(d);
        }

        // Si el grafo local es pequeño, agregar segundo grado
        if (ids.size() < 7) {
            for (auto& d1 : focus->depends_on) {
                const DepNode* n1 = state.dep_graph.get(d1);
                if (n1) {
                    for (auto& d2 : n1->depends_on) {
                        if (!d2.empty() && d2.size() < 64) ids.insert(d2);
                    }
                }
            }
            for (auto& d1 : state.dep_graph.get_dependents(focus_id)) {
                for (auto& d2 : state.dep_graph.get_dependents(d1)) {
                    if (!d2.empty() && d2.size() < 64) ids.insert(d2);
                }
            }
        }
    }

    // ── Posiciones iniciales: foco al centro, resto en anillos ────────────────
    int n_others = (int)ids.size() - 1;
    float angle_step = (n_others > 0) ? (2.f * PI / n_others) : 0.f;
    float base_dist  = REST_LEN * 0.85f;
    int   i = 0;

    for (auto& id : ids) {
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
        p.vx = p.vy = 0.f;
        s_phys[id] = p;
    }

    // Aplicar posiciones temporales guardadas para este modo
    std::string prefix = (state.mode==ViewMode::MSC2020?"MSC:":state.mode==ViewMode::Mathlib?"ML:":"STD:");
    for (auto& [k, v] : state.temp_positions) {
        if (k.rfind(prefix, 0) == 0) {
            std::string id = k.substr(prefix.size());
            auto it = s_phys.find(id);
            if (it != s_phys.end()) { it->second.x = v.x; it->second.y = v.y; }
        }
    }

    s_ready = true;
}

// ── dep_view_init_from_selection ──────────────────────────────────────────────

bool dep_view_init_from_selection(AppState& state) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty()) return false;

    // 1. Intentar con el nodo seleccionado
    if (!state.selected_code.empty()) {
        const DepNode* dn = use_graph.find_best(state.selected_code);
        if (dn) { dep_view_init(state, dn->id); return true; }
    }

    // 2. Intentar con el nodo actual de la pila de navegación
    MathNode* cur = state.current();
    if (cur && !cur->code.empty() && cur->code != "ROOT") {
        const DepNode* dn = use_graph.find_best(cur->code);
        if (dn) { dep_view_init(state, dn->id); return true; }
    }

    // 3. Fallback: primer nodo del grafo
    if (!use_graph.nodes().empty()) {
        const std::string& first_id = use_graph.nodes().begin()->second.id;
        dep_view_init(state, first_id);
        return true;
    }

    return false;
}

// ── simulate_step ─────────────────────────────────────────────────────────────

static void simulate_step(const AppState& state) {
    if (s_phys.size() < 2) { s_sim_step = SIM_MAX; return; }

    // Recolectar ids para iterar de forma estable
    std::vector<std::string> ids;
    ids.reserve(s_phys.size());
    for (auto& [id, _] : s_phys) ids.push_back(id);

    // ── Repulsión ─────────────────────────────────────────────────────────────
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

    // ── Atracción spring con longitud de reposo ────────────────────────────────
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;
            float dx = ib->second.x - ia->second.x;
            float dy = ib->second.y - ia->second.y;
            float d  = sqrtf(dx*dx + dy*dy) + 0.001f;
            // Spring: fuerza proporcional a la desviación de REST_LEN
            float stretch = (d - REST_LEN) / d;
            float fx = K_ATTRACT * stretch * dx;
            float fy = K_ATTRACT * stretch * dy;
            ia->second.vx += fx;
            ia->second.vy += fy;
            ib->second.vx -= fx;
            ib->second.vy -= fy;
        }
    }

    // ── Centrípeto sobre el nodo foco ─────────────────────────────────────────
    auto fi = s_phys.find(s_focus_id);
    if (fi != s_phys.end()) {
        fi->second.vx -= fi->second.x * K_CENTER;
        fi->second.vy -= fi->second.y * K_CENTER;
    }

    // ── Integrar + clamp de velocidad ─────────────────────────────────────────
    const float MAX_V = 80.f;
    for (auto& [id, p] : s_phys) {
        p.vx *= DAMPING;
        p.vy *= DAMPING;
        // Clamp para evitar explosión numérica
        float spd = sqrtf(p.vx*p.vx + p.vy*p.vy);
        if (spd > MAX_V) { p.vx = p.vx/spd*MAX_V; p.vy = p.vy/spd*MAX_V; }
        p.x += p.vx;
        p.y += p.vy;
    }

    s_sim_step++;
}

// ── Helpers de dibujo ─────────────────────────────────────────────────────────

// Clasifica un nodo respecto al foco: upstream (dep del foco), downstream
// (depende del foco), o neutro.
enum class NodeRole { Focus, Upstream, Downstream, Other };

static NodeRole get_role(const AppState& state, const std::string& id) {
    if (id == s_focus_id) return NodeRole::Focus;
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    const DepNode* focus = use_graph.get(s_focus_id);
    if (focus) {
        for (auto& d : focus->depends_on)
            if (d == id) return NodeRole::Upstream;
    }
    for (auto& d : use_graph.get_dependents(s_focus_id))
        if (d == id) return NodeRole::Downstream;
    return NodeRole::Other;
}

// Dibuja una flecha curva (cúbica bezier) entre dos rectángulos
static void draw_edge(Vector2 from, Vector2 to, float w_from, float h_from,
                      float w_to, float h_to, Color col) {
    // Calcular puntos en el borde de los rectángulos
    Vector2 dir = Vector2Normalize(Vector2Subtract(to, from));
    if (Vector2Length(dir) < 0.001f) return;

    Vector2 src = Vector2Add(from, Vector2Scale(dir, w_from * 0.5f + 2.f));
    Vector2 dst = Vector2Subtract(to, Vector2Scale(dir, w_to  * 0.5f + 6.f));

    // Bezier de control con ligera curvatura lateral
    Vector2 perp   = { -dir.y, dir.x };
    float   midlen = Vector2Length(Vector2Subtract(dst, src)) * 0.25f;
    Vector2 ctrl1  = Vector2Add(src, Vector2Add(Vector2Scale(dir, midlen),
                                                Vector2Scale(perp, midlen * 0.3f)));
    Vector2 ctrl2  = Vector2Subtract(dst, Vector2Add(Vector2Scale(dir, midlen),
                                                      Vector2Scale(perp, midlen * 0.3f)));

    // Dibujar bezier como segmentos
    const int SEG = 20;
    Vector2 prev = src;
    for (int k = 1; k <= SEG; k++) {
        float t  = (float)k / SEG;
        float t2 = t*t, t3 = t2*t;
        float u  = 1.f-t, u2 = u*u, u3 = u2*u;
        Vector2 pt = {
            u3*src.x + 3*u2*t*ctrl1.x + 3*u*t2*ctrl2.x + t3*dst.x,
            u3*src.y + 3*u2*t*ctrl1.y + 3*u*t2*ctrl2.y + t3*dst.y
        };
        DrawLineEx(prev, pt, 1.6f, col);
        prev = pt;
    }

    // Flecha en dst
    Vector2 arrow_dir = Vector2Normalize(Vector2Subtract(dst, ctrl2));
    Vector2 aperp = { -arrow_dir.y * 6.f, arrow_dir.x * 6.f };
    Vector2 tip   = Vector2Add(dst, Vector2Scale(arrow_dir, 4.f));
    DrawTriangle(
        tip,
        Vector2Add(Vector2Subtract(tip, Vector2Scale(arrow_dir, 12.f)), aperp),
        Vector2Subtract(Vector2Subtract(tip, Vector2Scale(arrow_dir, 12.f)), aperp),
        col
    );
}

// Dibuja un nodo con esquinas redondeadas simuladas + sombra + label
static void draw_node(const std::string& id, const NodePhys& p,
                      NodeRole role, bool hov, const Theme& th,
                      const AppState& state) {
    Rectangle rect = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
    float rnd = 6.f; // radio de esquinas

    // Colores según rol
    Color bg, border, text_c;
    switch (role) {
    case NodeRole::Focus:
        bg     = th.accent;
        border = th.success;
        text_c = th.bg_app;
        break;
    case NodeRole::Upstream:
        bg     = hov ? th.ctrl_bg_hover : ColorAlpha(th.success, 0.18f);
        border = th.success;
        text_c = th.ctrl_text;
        break;
    case NodeRole::Downstream:
        bg     = hov ? th.ctrl_bg_hover : ColorAlpha(th.accent, 0.18f);
        border = th.accent;
        text_c = th.ctrl_text;
        break;
    default:
        bg     = hov ? th.ctrl_bg_hover : ColorAlpha(th.ctrl_bg, 0.85f);
        border = hov ? th.bubble_hover_ring : th.ctrl_border;
        text_c = th.ctrl_text;
        break;
    }

    // Sombra con desplazamiento
    DrawRectangleRounded({ rect.x + 3.f, rect.y + 3.f, rect.width, rect.height },
                         rnd / rect.height, 6, ColorAlpha(BLACK, 0.40f));

    // Fondo
    DrawRectangleRounded(rect, rnd / rect.height, 6, bg);

    // Borde (grosor doble para foco)
    float bthick = (role == NodeRole::Focus) ? 2.2f : 1.2f;
    DrawRectangleRoundedLinesEx(rect, rnd / rect.height, 6, bthick, border);

    // Anillo de hover
    if (hov && role != NodeRole::Focus) {
        DrawRectangleRoundedLinesEx(
            { rect.x - 3.f, rect.y - 3.f, rect.width + 6.f, rect.height + 6.f },
            (rnd + 3.f) / (rect.height + 6.f), 6, 1.f,
            ColorAlpha(th.bubble_hover_ring, 0.5f));
    }

    // Label
    const DepNode* dn  = get_dep_graph_for_const(state).get(id);
    std::string    lbl = dn ? safe_trunc(dn->label, 20) : safe_trunc(id, 20);
    int tw = MeasureTextF(lbl.c_str(), 14);
    DrawTextF(lbl.c_str(), (int)(p.x - tw * 0.5f), (int)(p.y - 9), 14, text_c);

    // Sub-label: código en pequeño
    int cw = MeasureTextF(id.c_str(), 10);
    DrawTextF(id.c_str(), (int)(p.x - cw * 0.5f), (int)(p.y + 6),
              10, ColorAlpha(text_c, 0.55f));
}

// ── draw_dep_view ─────────────────────────────────────────────────────────────

void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse) {
    if (!s_ready) {
        // Mostrar placeholder si el grafo está vacío
        const Theme& th = g_theme;
        const char* msg = "Grafo de dependencias vacío o no cargado";
        int tw = MeasureTextF(msg, 18);
        DrawTextF(msg, (CANVAS_W() - tw) / 2, UI_TOP() + TOP_H() / 2, 18,
                  ColorAlpha(th.text_dim, 0.6f));
        // Botón Volver incluso en placeholder
        float bx = 14.f, by = (float)(UI_TOP() + 10), bw = 90.f, bh = 32.f;
        bool hb = CheckCollisionPointRec(mouse, {bx,by,bw,bh});
        DrawRectangleRounded({bx,by,bw,bh}, 0.3f, 4,
                             hb ? th.bg_button_hover : ColorAlpha(th.bg_button, 0.9f));
        DrawRectangleRoundedLinesEx({bx,by,bw,bh}, 0.3f, 4, 1.f, th.ctrl_border);
        DrawTextF("< Volver", (int)bx+10, (int)by+8, 17, th.ctrl_text);
        if (hb && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            state.dep_view_active = false;
        return;
    }

    const Theme& th  = g_theme;
    const int cw     = CANVAS_W();
    const int ch     = TOP_H();
    bool in_canvas   = mouse.x < cw && mouse.y >= UI_TOP() && mouse.y < g_split_y;
    bool canvas_blocked = overlay::blocks_mouse(mouse)
        || state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open      || state.toolbar.config_open;

    const DepGraph& use_graph = get_dep_graph_for_const(state);

    // ── Simulación ───────────────────────────────────────────────────────────
    if (s_sim_step < SIM_MAX) {
        int steps = (s_sim_step < SIM_MAX / 3) ? 10 : (s_sim_step < SIM_MAX*2/3 ? 5 : 2);
        for (int i = 0; i < steps && s_sim_step < SIM_MAX; i++)
            simulate_step(state);
    }

    // ── Pan y zoom ────────────────────────────────────────────────────────────
    // Solo pan con botón medio o con arrastre en zona sin nodo
    Vector2 wm_pre = in_canvas ? GetScreenToWorld2D(mouse, dep_cam)
                               : Vector2{-99999.f, -99999.f};
    bool over_node = false;
    if (in_canvas && !canvas_blocked) {
        for (auto& [id, p] : s_phys) {
            Rectangle r = {p.x-p.w*0.5f, p.y-p.h*0.5f, p.w, p.h};
            if (CheckCollisionPointRec(wm_pre, r)) { over_node = true; break; }
        }
    }

    if (in_canvas && !canvas_blocked) {
        bool pan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)
                || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_node);
        if (pan) {
            Vector2 d = GetMouseDelta();
            dep_cam.target.x -= d.x / dep_cam.zoom;
            dep_cam.target.y -= d.y / dep_cam.zoom;
        }
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.f && in_canvas && !canvas_blocked) {
        dep_cam.offset = mouse;
        dep_cam.target = GetScreenToWorld2D(mouse, dep_cam);
        dep_cam.zoom   = Clamp(dep_cam.zoom + wheel * 0.12f, 0.05f, 6.f);
    }
    dep_cam.offset = { (float)CX(), (float)CY() };

    // ── Fondo del canvas ─────────────────────────────────────────────────────
    DrawRectangle(0, UI_TOP(), cw, ch, ColorAlpha(th.bg_app, 1.f));

    // Grid de puntos sutil
    {
        float grid = 50.f * dep_cam.zoom;
        float ox = fmodf(dep_cam.offset.x - dep_cam.target.x * dep_cam.zoom, grid);
        float oy = fmodf(dep_cam.offset.y - dep_cam.target.y * dep_cam.zoom, grid);
        for (float gx = ox; gx < cw; gx += grid)
            for (float gy = UI_TOP() + oy; gy < g_split_y; gy += grid)
                DrawPixel((int)gx, (int)gy, ColorAlpha(th.ctrl_border, 0.25f));
    }

    BeginScissorMode(0, UI_TOP(), cw, ch);
    BeginMode2D(dep_cam);

    // ── Aristas ───────────────────────────────────────────────────────────────
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;

            // Color de arista según si involucra al nodo foco
            Color edge_col;
            if (id == s_focus_id)
                edge_col = ColorAlpha(th.success, 0.75f);
            else if (dep_id == s_focus_id)
                edge_col = ColorAlpha(th.accent, 0.75f);
            else
                edge_col = ColorAlpha(th.ctrl_border, 0.45f);

            draw_edge({ia->second.x, ia->second.y},
                      {ib->second.x, ib->second.y},
                      ia->second.w, ia->second.h,
                      ib->second.w, ib->second.h,
                      edge_col);
        }
    }

    // ── Nodos ─────────────────────────────────────────────────────────────────
    Vector2 wm = in_canvas ? GetScreenToWorld2D(mouse, dep_cam)
                           : Vector2{-99999.f, -99999.f};

    // Dibujar en dos pasadas: primero no-foco, luego foco (encima)
    std::string clicked_id;
    for (int pass = 0; pass < 2; pass++) {
        for (auto& [id, p] : s_phys) {
            bool is_focus = (id == s_focus_id);
            if ((pass == 0) == is_focus) continue; // foco en segunda pasada

            NodeRole role = get_role(state, id);
            Rectangle rect = {p.x-p.w*0.5f, p.y-p.h*0.5f, p.w, p.h};
            bool hov = !canvas_blocked && CheckCollisionPointRec(wm, rect);

            draw_node(id, p, role, hov, th, state);

            if (hov && !is_focus && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (state.position_mode_active) {
                    // start moving: record temp position for this id keyed by mode
                    std::string key = (state.mode==ViewMode::MSC2020?"MSC:":state.mode==ViewMode::Mathlib?"ML:":"STD:") + id;
                    Vector2 wp = GetScreenToWorld2D(mouse, dep_cam);
                    state.temp_positions[key] = wp;
                    // Apply immediately to physics map if present
                    auto it = s_phys.find(id);
                    if (it != s_phys.end()) { it->second.x = wp.x; it->second.y = wp.y; }
                } else {
                    clicked_id = id;
                }
            }
        }
    }

    EndMode2D();
    EndScissorMode();

    // Procesar click fuera del BeginMode2D para evitar romper el stack de Raylib
    if (!clicked_id.empty()) {
        dep_view_init(state, clicked_id);
        dep_cam.target = {0.f, 0.f};
        dep_cam.zoom   = 1.f;
        return;
    }

    // ── HUD ───────────────────────────────────────────────────────────────────
    // Barra superior translúcida
    DrawRectangle(0, UI_TOP(), cw, 48, ColorAlpha(th.bg_app, 0.88f));
    DrawLineEx({0.f,(float)(UI_TOP()+48)},{(float)cw,(float)(UI_TOP()+48)},
               1.f, ColorAlpha(th.ctrl_border, 0.4f));

    // Botón "< Volver"
    {
        float bx = 12.f, by = (float)(UI_TOP() + 8), bw = 88.f, bh = 30.f;
        bool hov = CheckCollisionPointRec(mouse, {bx,by,bw,bh}) && !canvas_blocked;
        DrawRectangleRounded({bx,by,bw,bh}, 0.3f, 6,
                             hov ? th.bg_button_hover : ColorAlpha(th.bg_button, 0.9f));
        DrawRectangleRoundedLinesEx({bx,by,bw,bh}, 0.3f, 6, 1.f, th.ctrl_border);
        DrawTextF("< Volver", (int)bx+10, (int)by+7, 16,
                  hov ? th.ctrl_text : ColorAlpha(th.ctrl_text_dim, 0.9f));
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            state.dep_view_active = false;
    }

    // Título central
    {
        const DepNode* fn   = use_graph.get(s_focus_id);
        std::string    title = fn ? safe_trunc(fn->label, 38) : safe_trunc(s_focus_id, 38);
        std::string    full  = s_focus_id + "  \xE2\x80\x94  " + title; // em-dash UTF-8
        int tw = MeasureTextF(full.c_str(), 17);
        DrawTextF(full.c_str(), cw/2 - tw/2, UI_TOP() + 15, 17, th.ctrl_text);
    }

    // Leyenda de roles (esquina superior derecha)
    {
        int lx = cw - 180, ly = UI_TOP() + 8;
        auto dot = [&](Color c, const char* label, int row){
            DrawCircle(lx+7, ly+7+row*18, 5.f, c);
            DrawTextF(label, lx+16, ly+row*18, 12, ColorAlpha(th.ctrl_text, 0.75f));
        };
        dot(th.success,           "Upstream",   0);
        dot(th.accent,            "Downstream", 1);
        dot(ColorAlpha(th.ctrl_border, 0.8f), "Otro", 2);
    }

    // Indicador de simulación
    if (s_sim_step < SIM_MAX) {
        float pct = (float)s_sim_step / SIM_MAX;
        int bw = 120, bh = 6;
        int bx = 12, by2 = g_split_y - 20;
        DrawRectangle(bx, by2, bw, bh, ColorAlpha(th.ctrl_bg, 0.7f));
        DrawRectangle(bx, by2, (int)(bw*pct), bh, ColorAlpha(th.accent, 0.8f));
        DrawTextF("simulando...", bx, by2 - 14, 11, ColorAlpha(th.text_dim, 0.6f));
    }

    // Info del grafo (esquina inferior derecha)
    {
        char info[80];
        snprintf(info, sizeof(info), "%d nodos totales  |  %zu en vista",
                 (int)use_graph.size(), s_phys.size());
        int iw = MeasureTextF(info, 12);
        DrawTextF(info, cw - iw - 10, g_split_y - 18, 12,
                  ColorAlpha(th.text_dim, 0.55f));
    }

    // Hint de interacción
    {
        const char* hint = "Clic en nodo para re-enfocar  |  Rueda: zoom  |  Arrastre: pan";
        int hw = MeasureTextF(hint, 11);
        DrawTextF(hint, cw/2 - hw/2, g_split_y - 18, 11,
                  ColorAlpha(th.text_dim, 0.4f));
    }
}
