#include "ui/views/dep_view.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "ui/views/dep/dep_draw.hpp"
#include "ui/views/dep/dep_pivot.hpp"
#include "ui/views/bubble/bubble_controls.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "data/state/position_state.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include "data/connect/vscode_bridge.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cmath>

// ── dep_view_init ─────────────────────────────────────────────────────────────

void dep_view_init(AppState& state, const std::string& focus_id) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty()) return;
    if (focus_id.empty() || focus_id.size() > 64) return;

    state.dep_focus_id = focus_id;

    std::unordered_map<std::string, bool> visible;
    visible[focus_id] = true;

    const DepNode* focus = use_graph.get(focus_id);
    if (focus) {
        for (auto& d : focus->depends_on)
            if (!d.empty() && d.size() < 64) visible[d] = true;
        for (auto& d : use_graph.get_dependents(focus_id))
            if (!d.empty() && d.size() < 64) visible[d] = true;

        if (visible.size() < 12) {
            for (auto& d1 : focus->depends_on) {
                const DepNode* n1 = use_graph.get(d1);
                if (!n1) continue;
                for (auto& d2 : n1->depends_on)
                    if (!d2.empty() && d2.size() < 64) visible[d2] = true;
            }
            for (auto& d1 : use_graph.get_dependents(focus_id)) {
                for (auto& d2 : use_graph.get_dependents(d1))
                    if (!d2.empty() && d2.size() < 64) visible[d2] = true;
            }
        }
    }

    dep_sim_init(state, focus_id, visible);

    // Si el pivote estaba activo, recalcular sus anillos con el nuevo s_phys.
    // Si el pivote_id ya no está visible, lo movemos al nuevo foco.
    if (g_dep_pivot.active) {
        if (s_phys.find(g_dep_pivot.pivot_id) == s_phys.end())
            g_dep_pivot.pivot_id = focus_id;
        dep_pivot_rebuild(state);
    }
}

// ── dep_view_init_from_selection ──────────────────────────────────────────────

bool dep_view_init_from_selection(AppState& state) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty()) return false;

    if (!state.selected_code.empty()) {
        const DepNode* dn = use_graph.find_best(state.selected_code);
        if (dn) { dep_view_init(state, dn->id); return true; }
    }

    MathNode* cur = state.current();
    if (cur && !cur->code.empty() && cur->code != "ROOT") {
        const DepNode* dn = use_graph.find_best(cur->code);
        if (dn) { dep_view_init(state, dn->id); return true; }
    }

    if (!use_graph.nodes().empty()) {
        dep_view_init(state, use_graph.nodes().begin()->second.id);
        return true;
    }

    return false;
}

// ── dep_view_set_position_mode ────────────────────────────────────────────────

void dep_view_set_position_mode(AppState& state, bool on) {
    state.position_mode_active = on;
}

// ── Navegación espacial (modo geográfico) ─────────────────────────────────────

static std::string dep_nearest_in_dir(const std::string& cur_id, float dx, float dy) {
    auto it = s_phys.find(cur_id);
    if (it == s_phys.end()) return "";

    float cx = it->second.x, cy = it->second.y;
    std::string best;
    float best_score = 1e9f;

    for (auto& [id, p] : s_phys) {
        if (id == cur_id) continue;
        float rx = p.x - cx, ry = p.y - cy;
        float dot = rx * dx + ry * dy;
        if (dot <= 0.f) continue;
        float cross = fabsf(rx * dy - ry * dx);
        float dist = sqrtf(rx * rx + ry * ry) + 0.001f;
        float score = cross / dot + dist * 0.05f;
        if (score < best_score) { best_score = score; best = id; }
    }
    return best;
}

// ── draw_dep_view ─────────────────────────────────────────────────────────────

void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse) {
    const Theme& th = g_theme;
    const int    cw = CANVAS_W();
    const int    ch = TOP_H();

    bool canvas_blocked = overlay::blocks_mouse(mouse)
        || state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open || state.toolbar.config_open;
    bool in_canvas = mouse.x < cw && mouse.y >= UI_TOP() && mouse.y < g_split_y;

    // ── Placeholder ───────────────────────────────────────────────────────────
    if (!s_ready) {
        const char* msg = "Grafo de dependencias vacío o no cargado";
        int tw = MeasureTextF(msg, 18);
        DrawTextF(msg, (cw - tw) / 2, UI_TOP() + ch / 2, 18,
            ColorAlpha(th.text_dim, 0.6f));
        draw_zoom_buttons(dep_cam, mouse);
        draw_dep_canvas_buttons(state, dep_cam, mouse, canvas_blocked);
        draw_vscode_button(state, mouse);
        draw_mathlib_button(state, mouse);
        return;
    }

    const DepGraph& use_graph = get_dep_graph_for_const(state);

    // ── Modificadores ─────────────────────────────────────────────────────────
    bool ctrl_held = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift_held = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // Shift+flechas = pan de cámara (velocidad proporcional al zoom inverso).
    // Funciona siempre en Canvas, incluso combinado con Ctrl.
    if (g_kbnav.in(FocusZone::Canvas) && shift_held) {
        const float PAN_SPEED = 12.f / dep_cam.zoom;
        if (IsKeyDown(KEY_LEFT))  dep_cam.target.x -= PAN_SPEED;
        if (IsKeyDown(KEY_RIGHT)) dep_cam.target.x += PAN_SPEED;
        if (IsKeyDown(KEY_UP))    dep_cam.target.y -= PAN_SPEED;
        if (IsKeyDown(KEY_DOWN))  dep_cam.target.y += PAN_SPEED;
    }

    // ── Modo pivote: gestión de Ctrl ──────────────────────────────────────────
    // Ctrl solo (sin Shift) → activa pivote y su navegación.
    // Ctrl+Shift            → muestra colores rojo del pivote pero NO navega
    //                         (las flechas están ocupadas por el pan de Shift).
    bool pivot_nav_active = ctrl_held && !shift_held;

    if (g_kbnav.in(FocusZone::Canvas)) {
        if (ctrl_held && !g_dep_pivot.active) {
            dep_pivot_activate(state);   // activa visual en ambos casos (Ctrl solo o Ctrl+Shift)
        }
        else if (!ctrl_held && g_dep_pivot.active) {
            dep_pivot_deactivate();
        }
    }
    else {
        if (g_dep_pivot.active) dep_pivot_deactivate();
    }

    // ── Input de teclado ──────────────────────────────────────────────────────
    if (g_kbnav.in(FocusZone::Canvas) && !shift_held) {

        // Sincronizar selector geográfico con el foco actual
        if (g_kbnav.dep_sel_id.empty() ||
            s_phys.find(g_kbnav.dep_sel_id) == s_phys.end()) {
            g_kbnav.dep_sel_id = s_focus_id;
        }

        if (pivot_nav_active) {
            // ── Modo pivote: navegación por anillos ───────────────────────────
            bool repivot = false;
            dep_pivot_handle_keys(repivot);

            if (repivot) {
                // El usuario hizo Enter: re-pivotar al nuevo pivot_id
                std::string new_pivot = g_dep_pivot.pivot_id;

                // Opción B: re-inicializar el grafo con el nuevo pivote como foco
                dep_view_init(state, new_pivot);
                g_kbnav.dep_sel_id = new_pivot;

                const DepNode* dn = use_graph.get(new_pivot);
                if (dn) {
                    state.selected_code = dn->id;
                    state.selected_label = dn->label;
                }
                dep_cam.target = { 0.f, 0.f };
                dep_cam.zoom = 1.f;

                // Recalcular anillos del pivote (dep_view_init ya lo hace si activo)
                // pivot_id ya fue actualizado en dep_pivot_handle_keys
                dep_pivot_rebuild(state);
                return;
            }
        }
        else {
            // ── Modo geográfico: navegación espacial normal ────────────────────
            if (IsKeyPressed(KEY_LEFT)) {
                auto n = dep_nearest_in_dir(g_kbnav.dep_sel_id, -1.f, 0.f);
                if (!n.empty()) g_kbnav.dep_sel_id = n;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                auto n = dep_nearest_in_dir(g_kbnav.dep_sel_id, 1.f, 0.f);
                if (!n.empty()) g_kbnav.dep_sel_id = n;
            }
            if (IsKeyPressed(KEY_UP)) {
                auto n = dep_nearest_in_dir(g_kbnav.dep_sel_id, 0.f, -1.f);
                if (!n.empty()) g_kbnav.dep_sel_id = n;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                auto n = dep_nearest_in_dir(g_kbnav.dep_sel_id, 0.f, 1.f);
                if (!n.empty()) g_kbnav.dep_sel_id = n;
            }
            // Enter → re-enfocar en el nodo seleccionado (comportamiento original)
            if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                && !g_kbnav.dep_sel_id.empty()) {
                std::string sel = g_kbnav.dep_sel_id;
                dep_view_init(state, sel);
                const DepNode* dn = use_graph.get(sel);
                if (dn) { state.selected_code = dn->id; state.selected_label = dn->label; }
                dep_cam.target = { 0.f, 0.f };
                dep_cam.zoom = 1.f;
                g_kbnav.dep_sel_id = sel;
                return;
            }
        }
    }

    // ── Simulación ────────────────────────────────────────────────────────────
    if (s_sim_step < SIM_MAX) {
        int steps = s_sim_step < SIM_MAX / 3 ? 10 : s_sim_step < SIM_MAX * 2 / 3 ? 5 : 2;
        for (int i = 0; i < steps && s_sim_step < SIM_MAX; i++)
            dep_sim_step(state);
    }

    // ── Estado de drag para modo posición ─────────────────────────────────────
    static std::string s_drag_id;
    static Vector2     s_drag_start_mouse = { 0.f, 0.f };
    static Vector2     s_drag_start_world = { 0.f, 0.f };

    // ── Pan y zoom ────────────────────────────────────────────────────────────
    Vector2 wm_pre = in_canvas ? GetScreenToWorld2D(mouse, dep_cam)
        : Vector2{ -99999.f, -99999.f };
    bool over_node = false;
    if (in_canvas && !canvas_blocked) {
        for (auto& [id, p] : s_phys) {
            Rectangle r = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
            if (CheckCollisionPointRec(wm_pre, r)) { over_node = true; break; }
        }
    }
    if (in_canvas && !canvas_blocked) {
        bool dragging_node = state.position_mode_active && !s_drag_id.empty();
        bool pan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)
            || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_node && !dragging_node);
        if (pan) {
            Vector2 d = GetMouseDelta();
            dep_cam.target.x -= d.x / dep_cam.zoom;
            dep_cam.target.y -= d.y / dep_cam.zoom;
        }
    }
    if (float wh = GetMouseWheelMove(); wh != 0.f && in_canvas && !canvas_blocked) {
        dep_cam.offset = mouse;
        dep_cam.target = GetScreenToWorld2D(mouse, dep_cam);
        dep_cam.zoom = Clamp(dep_cam.zoom + wh * 0.12f, 0.05f, 6.f);
    }
    dep_cam.offset = { (float)CX(), (float)CCY() };

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

    // ── Escena 2D ─────────────────────────────────────────────────────────────
    BeginScissorMode(0, UI_TOP(), cw, ch);
    BeginMode2D(dep_cam);

    // Id del nodo seleccionado en modo pivote (para saber qué pintar)
    std::string pivot_sel_id = g_dep_pivot.active
        ? dep_pivot_selected_id() : "";

    // Aristas: resaltar las conectadas al pivote si está activo
    for (auto& [id, node] : use_graph.nodes()) {
        auto ia = s_phys.find(id);
        if (ia == s_phys.end()) continue;
        for (auto& dep_id : node.depends_on) {
            auto ib = s_phys.find(dep_id);
            if (ib == s_phys.end()) continue;

            Color edge_col;
            if (g_dep_pivot.active) {
                // Resaltar aristas conectadas al pivote rojo
                bool touches_pivot = (id == g_dep_pivot.pivot_id
                    || dep_id == g_dep_pivot.pivot_id);
                edge_col = touches_pivot
                    ? ColorAlpha({ 220, 80, 80, 255 }, 0.80f)   // rojo para el pivote
                    : ColorAlpha(th.ctrl_border, 0.25f);         // resto más tenue
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
                ia->second.w, ia->second.h,
                edge_col);
        }
    }

    // Nodos (dos pasadas: primero no-foco, luego foco)
    Vector2     wm = in_canvas ? GetScreenToWorld2D(mouse, dep_cam)
        : Vector2{ -99999.f, -99999.f };
    std::string clicked_id;
    std::string mode_prefix =
        state.mode == ViewMode::MSC2020 ? "MSC:" :
        state.mode == ViewMode::Mathlib ? "ML:" : "STD:";

    if (state.position_mode_active && !s_drag_id.empty()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 cur_wm = GetScreenToWorld2D(mouse, dep_cam);
            Vector2 delta = { cur_wm.x - s_drag_start_mouse.x,
                               cur_wm.y - s_drag_start_mouse.y };
            auto it = s_phys.find(s_drag_id);
            if (it != s_phys.end()) {
                it->second.x = s_drag_start_world.x + delta.x;
                it->second.y = s_drag_start_world.y + delta.y;
                it->second.vx = 0.f;
                it->second.vy = 0.f;
            }
        }
        else {
            auto it = s_phys.find(s_drag_id);
            if (it != s_phys.end()) {
                std::string key = mode_prefix + s_drag_id;
                state.temp_positions[key] = { it->second.x, it->second.y };
            }
            s_drag_id.clear();
        }
    }

    for (int pass = 0; pass < 2; pass++) {
        for (auto& [id, p] : s_phys) {
            bool is_focus = (id == s_focus_id);
            if ((pass == 0) == is_focus) continue;

            NodeRole  role = dep_get_role(state, id);
            Rectangle rect = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
            bool hov = !canvas_blocked && CheckCollisionPointRec(wm, rect)
                && s_drag_id.empty();

            dep_draw_node(id, p, role, hov, th, state);

            // ── Selector geográfico azul (siempre visible) ────────────────────
            if (g_kbnav.in(FocusZone::Canvas) && id == g_kbnav.dep_sel_id) {
                float t = (float)GetTime();
                float pulse = 1.f + 0.05f * sinf(t * 4.f);
                float pad = 6.f * pulse;
                float rnd = 6.f;
                Rectangle sel_r = { rect.x - pad, rect.y - pad,
                                    rect.width + pad * 2.f,
                                    rect.height + pad * 2.f };
                float corner = rnd / std::max(sel_r.height, 1.f);
                DrawRectangleRoundedLinesEx(sel_r, corner, 6, 2.f,
                    ColorAlpha(th.accent, 0.9f));
                DrawRectangleRoundedLinesEx(
                    { sel_r.x - 2.f, sel_r.y - 2.f,
                      sel_r.width + 4.f, sel_r.height + 4.f },
                    corner, 6, 1.f, ColorAlpha(th.accent, 0.3f));
            }

            // ── Selectores del modo pivote ─────────────────────────────────────
            if (g_dep_pivot.active) {
                // Anillo rojo: nodo pivote
                if (id == g_dep_pivot.pivot_id)
                    dep_draw_pivot_node(p, th);

                // Anillo púrpura: nodo azul seleccionado dentro del modo pivote
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

    if (!clicked_id.empty() && !g_dep_pivot.active) {
        // En modo pivote el clic no re-enfoca (para no interferir con el pivote)
        dep_view_init(state, clicked_id);
        g_kbnav.dep_sel_id = clicked_id;
        const DepNode* dn = use_graph.get(clicked_id);
        if (dn) {
            state.selected_code = dn->id;
            state.selected_label = dn->label;
        }
        dep_cam.target = { 0.f, 0.f };
        dep_cam.zoom = 1.f;
        return;
    }

    // ── HUD INFO ──────────────────────────────────────────────────────────────
    DrawRectangle(0, UI_TOP(), cw, 48, ColorAlpha(th.bg_app, 0.88f));
    DrawLineEx({ 0.f, (float)(UI_TOP() + 48) }, { (float)cw, (float)(UI_TOP() + 48) },
        1.f, ColorAlpha(th.ctrl_border, 0.4f));

    {
        const DepNode* fn = use_graph.get(s_focus_id);
        std::string    title = fn
            ? dep_safe_trunc(fn->label, 36)
            : dep_safe_trunc(s_focus_id, 36);
        std::string    full = s_focus_id + "  \xE2\x80\x94  " + title;

        // Si modo pivote activo, mostrar también el pivote
        if (g_dep_pivot.active) {
            const DepNode* pn = use_graph.get(g_dep_pivot.pivot_id);
            std::string plbl = pn
                ? dep_safe_trunc(pn->label, 24)
                : dep_safe_trunc(g_dep_pivot.pivot_id, 24);
            full += "   |   \xF0\x9F\x94\xB4 " + g_dep_pivot.pivot_id + " " + plbl;
        }

        int tw = MeasureTextF(full.c_str(), 15);
        int text_x = 170 + (cw - 170 - 190 - tw) / 2;
        DrawTextF(full.c_str(), text_x, UI_TOP() + 15, 15, th.ctrl_text);
    }

    {
        int lx = cw - 180, ly = UI_TOP() + 8;
        auto dot = [&](Color c, const char* label, int row) {
            DrawCircle(lx + 7, ly + 7 + row * 18, 5.f, c);
            DrawTextF(label, lx + 16, ly + row * 18, 12,
                ColorAlpha(th.ctrl_text, 0.75f));
            };
        dot(th.success, "Upstream", 0);
        dot(th.accent, "Downstream", 1);
        dot(ColorAlpha(th.ctrl_border, 0.8f), "Otro", 2);
        if (g_dep_pivot.active)
            dot({ 220, 50, 50, 230 }, "Pivote", 3);
    }

    if (s_sim_step < SIM_MAX) {
        float pct = (float)s_sim_step / SIM_MAX;
        int bw = 120, bh = 6, bx2 = 12, by2 = g_split_y - 20;
        DrawRectangle(bx2, by2, bw, bh, ColorAlpha(th.ctrl_bg, 0.7f));
        DrawRectangle(bx2, by2, (int)(bw * pct), bh, ColorAlpha(th.accent, 0.8f));
        DrawTextF("simulando...", bx2, by2 - 14, 11, ColorAlpha(th.text_dim, 0.6f));
    }

    {
        char info[80];
        snprintf(info, sizeof(info), "%d nodos totales  |  %zu en vista",
            (int)use_graph.size(), s_phys.size());
        int iw = MeasureTextF(info, 12);
        DrawTextF(info, cw - iw - 10, g_split_y - 18, 12,
            ColorAlpha(th.text_dim, 0.55f));
    }

    {
        const char* hint;
        if (state.position_mode_active)
            hint = "Modo posicion: arrastra nodos  |  Clic en 'Posicion' para salir";
        else if (g_dep_pivot.active) {
            // Mostrar en qué anillo y slot está el cursor pivote
            char pivot_hint[120];
            snprintf(pivot_hint, sizeof(pivot_hint),
                "CTRL activo — Anillo %+d | Slot %d/%d  "
                "|  Flechas: navegar  |  Enter: re-pivotar  |  Suelta Ctrl: salir",
                g_dep_pivot.cur_ring,
                g_dep_pivot.cur_slot + 1,
                [&]() -> int {
                    for (auto& r : g_dep_pivot.rings)
                        if (r.ring_idx == g_dep_pivot.cur_ring)
                            return (int)r.slots.size();
                    return 0;
                }());
            hint = pivot_hint;
        }
        else if (shift_held) {
            hint = "SHIFT activo — Flechas: mover cámara  |  Ctrl+Shift: pivote visual sin navegar";
        }
        else if (g_kbnav.in(FocusZone::Canvas)) {
            hint = "Flechas: navegar  |  Shift: mover cámara  |  Ctrl: pivote  |  Enter: re-enfocar";
        }
        else {
            hint = "Clic: re-enfocar  |  Shift+Flechas: mover cámara  |  Rueda: zoom  |  Ctrl: pivote";
        }
        int hw = MeasureTextF(hint, 11);
        DrawTextF(hint, cw / 2 - hw / 2, g_split_y - 18, 11,
            ColorAlpha(th.text_dim, 0.4f));
    }

    // ── Controles compartidos ─────────────────────────────────────────────────
    draw_zoom_buttons(dep_cam, mouse);
    draw_dep_canvas_buttons(state, dep_cam, mouse, canvas_blocked);
    draw_vscode_button(state, mouse);
    draw_mathlib_button(state, mouse);
}