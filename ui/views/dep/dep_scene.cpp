#include "ui/views/dep/dep_scene.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "ui/views/dep/dep_draw.hpp"
#include "ui/views/dep/dep_pivot.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

// ── Estado de drag (módulo-local) ─────────────────────────────────────────────
static std::string s_drag_id;
static Vector2     s_drag_start_mouse = { 0.f, 0.f };
static Vector2     s_drag_start_world = { 0.f, 0.f };

bool dep_scene_has_drag() { return !s_drag_id.empty(); }

// ── dep_scene_draw ────────────────────────────────────────────────────────────

std::string dep_scene_draw(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool in_canvas, bool canvas_blocked) {
    const Theme& th = g_theme;
    const int    cw = CANVAS_W();
    const int    ch = TOP_H();
    const DepGraph& use_graph = get_dep_graph_for_const(state);

    std::string mode_prefix =
        state.mode == ViewMode::MSC2020 ? "MSC:" :
        state.mode == ViewMode::Mathlib ? "ML:" : "STD:";

    // ── Detectar hover sobre algún nodo ───────────────────────────────────────
    Vector2 wm_pre = in_canvas
        ? GetScreenToWorld2D(mouse, dep_cam) : Vector2{ -99999.f, -99999.f };
    bool over_node = false;
    if (in_canvas && !canvas_blocked) {
        for (auto& [id, p] : s_phys) {
            Rectangle r = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
            if (CheckCollisionPointRec(wm_pre, r)) { over_node = true; break; }
        }
    }

    // ── Fondo con grid ────────────────────────────────────────────────────────
    DrawRectangle(0, UI_TOP(), cw, ch, ColorAlpha(th.bg_app, 1.f));
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

    Vector2 wm = in_canvas
        ? GetScreenToWorld2D(mouse, dep_cam) : Vector2{ -99999.f, -99999.f };

    std::string pivot_sel_id = g_dep_pivot.active ? dep_pivot_selected_id() : "";

    // ── Anillos concéntricos (guías visuales — no afectan navegación) ─────────
    // Se dibujan centrados en el nodo foco actual.
    // Upstream (d<0) → verde tenue; downstream (d>0) → azul tenue.
    // Cada RingVisInfo puede tener num_subrings sub-anillos en el mismo nivel.
    if (!s_ring_vis.empty()) {
        auto foc_it = s_phys.find(s_focus_id);
        float fx = foc_it != s_phys.end() ? foc_it->second.x : 0.f;
        float fy = foc_it != s_phys.end() ? foc_it->second.y : 0.f;

        for (auto& rv : s_ring_vis) {
            // Colores diferenciados upstream / downstream
            Color ring_col = rv.ring_idx > 0
                ? ColorAlpha(th.accent, 0.09f)
                : ColorAlpha(th.success, 0.09f);
            Color sub_col = rv.ring_idx > 0
                ? ColorAlpha(th.accent, 0.045f)
                : ColorAlpha(th.success, 0.045f);

            for (int sr = 0; sr < rv.num_subrings; sr++) {
                float r = rv.base_radius + sr * SUB_RING_GAP;
                if (r < 8.f) continue;   // no dibujar radios insignificantes
                DrawCircleLines((int)fx, (int)fy, r, sr == 0 ? ring_col : sub_col);
            }
        }
    }

    // ── Aristas ───────────────────────────────────────────────────────────────
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;

            Color edge_col;
            if (g_dep_pivot.active) {
                bool touches = (id == g_dep_pivot.pivot_id || dep_id == g_dep_pivot.pivot_id);
                edge_col = touches
                    ? ColorAlpha({ 220, 80, 80, 255 }, 0.80f)
                    : ColorAlpha(th.ctrl_border, 0.25f);
            }
            else {
                edge_col =
                    id == s_focus_id ? ColorAlpha(th.success, 0.75f) :
                    dep_id == s_focus_id ? ColorAlpha(th.accent, 0.75f) :
                    ColorAlpha(th.ctrl_border, 0.45f);
            }
            dep_draw_edge(
                { ib->second.x, ib->second.y }, { ia->second.x, ia->second.y },
                ib->second.w, ib->second.h,
                ia->second.w, ia->second.h, edge_col);
        }
    }

    // ── Drag activo ───────────────────────────────────────────────────────────
    if (state.position_mode_active && !s_drag_id.empty()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 cur_wm = GetScreenToWorld2D(mouse, dep_cam);
            Vector2 delta = { cur_wm.x - s_drag_start_mouse.x,
                               cur_wm.y - s_drag_start_mouse.y };
            auto it = s_phys.find(s_drag_id);
            if (it != s_phys.end()) {
                it->second.x = s_drag_start_world.x + delta.x;
                it->second.y = s_drag_start_world.y + delta.y;
                it->second.vx = 0.f; it->second.vy = 0.f;
            }
        }
        else {
            auto it = s_phys.find(s_drag_id);
            if (it != s_phys.end())
                state.temp_positions[mode_prefix + s_drag_id] = { it->second.x, it->second.y };
            s_drag_id.clear();
        }
    }

    // ── Nodos (2 pasadas: no-foco primero, foco después) ──────────────────────
    std::string clicked_id;

    for (int pass = 0; pass < 2; pass++) {
        for (auto& [id, p] : s_phys) {
            bool is_focus = (id == s_focus_id);
            if ((pass == 0) == is_focus) continue;

            NodeRole  role = dep_get_role(state, id);
            Rectangle rect = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
            bool hov = !canvas_blocked && CheckCollisionPointRec(wm, rect)
                && s_drag_id.empty();

            dep_draw_node(id, p, role, hov, th, state);

            // Selector geográfico azul
            if (g_kbnav.in(FocusZone::Canvas) && id == g_kbnav.dep_sel_id) {
                float t = (float)GetTime();
                float pulse = 1.f + 0.05f * sinf(t * 4.f);
                float pad = 6.f * pulse;
                float rnd = 6.f;
                Rectangle sel_r = { rect.x - pad, rect.y - pad,
                                    rect.width + pad * 2.f, rect.height + pad * 2.f };
                float corner = rnd / std::max(sel_r.height, 1.f);
                DrawRectangleRoundedLinesEx(sel_r, corner, 6, 2.f, ColorAlpha(th.accent, 0.9f));
                DrawRectangleRoundedLinesEx(
                    { sel_r.x - 2.f, sel_r.y - 2.f, sel_r.width + 4.f, sel_r.height + 4.f },
                    corner, 6, 1.f, ColorAlpha(th.accent, 0.3f));
            }

            // Selectores del pivote rojo
            if (g_dep_pivot.active) {
                if (id == g_dep_pivot.pivot_id)
                    dep_draw_pivot_node(p, th);
                if (!pivot_sel_id.empty() && id == pivot_sel_id)
                    dep_draw_pivot_selector(p, th);
            }

            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (state.position_mode_active) {
                    s_drag_id = id;
                    s_drag_start_mouse = GetScreenToWorld2D(mouse, dep_cam);
                    s_drag_start_world = { p.x, p.y };
                }
                else if (!is_focus) {
                    clicked_id = id;
                }
            }
        }
    }

    EndMode2D();
    EndScissorMode();

    return clicked_id;
}