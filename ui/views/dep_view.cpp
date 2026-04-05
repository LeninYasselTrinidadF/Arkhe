#include "ui/views/dep_view.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "ui/views/dep/dep_pivot.hpp"
#include "ui/views/dep/dep_input.hpp"
#include "ui/views/dep/dep_camera.hpp"
#include "ui/views/dep/dep_scene.hpp"
#include "ui/views/dep/dep_hud.hpp"
#include "ui/views/bubble/bubble_controls.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "data/loaders/dep_graph.hpp"
#include "raylib.h"
#include <unordered_map>
#include <string>
#include <vector>

// ── Historial de navegación ───────────────────────────────────────────────────
// Stack de focus_ids visitados. dep_view_init empuja el foco actual antes de
// cambiar. dep_view_back() lo desapila y restaura sin volver a empujar.
// Se limpia al entrar al view desde selección externa (dep_view_init_from_selection).
static std::vector<std::string> s_nav_history;
static constexpr size_t         NAV_HISTORY_MAX = 50;

static constexpr size_t DEP_MAX_VISIBLE = 80;

// ── dep_view_init_impl ────────────────────────────────────────────────────────
// Núcleo sin tocar el historial.
static void dep_view_init_impl(AppState& state, const std::string& focus_id) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty() || focus_id.empty() || focus_id.size() > 64) return;

    state.dep_focus_id = focus_id;

    // ── Construir el conjunto visible con prioridad explícita ─────────────────
    //
    //  Prioridad 1: nodo foco (siempre presente)
    //  Prioridad 2: dependencias directas del foco (upstream 1 salto)
    //  Prioridad 3: dependientes directos del foco (downstream 1 salto)
    //  Prioridad 4: upstream 2 saltos (solo si hay cupo)
    //  Prioridad 5: downstream 2 saltos (solo si hay cupo)
    //
    //  El límite DEP_MAX_VISIBLE se respeta estrictamente.
    //  Para nodos de alta conectividad (ej. Mathlib.Tactic con ~800 deps)
    //  esto evita que el layout colapse en una estrella ilegible.

    std::unordered_map<std::string, bool> visible;
    visible.reserve(DEP_MAX_VISIBLE + 4);

    // P1: foco
    visible[focus_id] = true;

    const DepNode* focus = use_graph.get(focus_id);
    if (focus) {
        // P2: upstream 1 salto
        for (auto& d : focus->depends_on) {
            if (d.empty() || d.size() >= 64) continue;
            if (visible.size() >= DEP_MAX_VISIBLE) break;
            visible[d] = true;
        }

        // P3: downstream 1 salto
        for (auto& d : use_graph.get_dependents(focus_id)) {
            if (d.empty() || d.size() >= 64) continue;
            if (visible.size() >= DEP_MAX_VISIBLE) break;
            visible[d] = true;
        }

        // P4 y P5: 2 saltos solo si hay cupo suficiente
        if (visible.size() < DEP_MAX_VISIBLE) {
            // P4: upstream 2 saltos
            for (auto& d1 : focus->depends_on) {
                const DepNode* n1 = use_graph.get(d1);
                if (!n1) continue;
                for (auto& d2 : n1->depends_on) {
                    if (d2.empty() || d2.size() >= 64) continue;
                    if (visible.size() >= DEP_MAX_VISIBLE) goto done_expansion;
                    visible[d2] = true;
                }
            }
            // P5: downstream 2 saltos
            for (auto& d1 : use_graph.get_dependents(focus_id)) {
                for (auto& d2 : use_graph.get_dependents(d1)) {
                    if (d2.empty() || d2.size() >= 64) continue;
                    if (visible.size() >= DEP_MAX_VISIBLE) goto done_expansion;
                    visible[d2] = true;
                }
            }
        }
    }
done_expansion:;

    dep_sim_init(state, focus_id, visible);

    if (g_dep_pivot.active) {
        if (s_phys.find(g_dep_pivot.pivot_id) == s_phys.end())
            g_dep_pivot.pivot_id = focus_id;
        dep_pivot_rebuild(state);
    }
}

// ── dep_view_init (público) ───────────────────────────────────────────────────
// Empuja el foco actual al historial antes de cambiar, salvo que:
//   - el historial ya tenga ese id en la cima (navegación repetida)
//   - s_ready sea false (todavía no se ha mostrado ningún grafo)

void dep_view_init(AppState& state, const std::string& focus_id) {
    if (s_ready && !s_focus_id.empty() && s_focus_id != focus_id) {
        if (s_nav_history.empty() || s_nav_history.back() != s_focus_id) {
            s_nav_history.push_back(s_focus_id);
            if (s_nav_history.size() > NAV_HISTORY_MAX)
                s_nav_history.erase(s_nav_history.begin());
        }
    }
    dep_view_init_impl(state, focus_id);
}

bool dep_view_init_from_selection(AppState& state) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    if (use_graph.empty()) return false;

    // Punto de entrada externo: limpiar historial para que "< Atrás"
    // no intente volver a un nodo de una sesión anterior.
    s_nav_history.clear();

    if (!state.selected_code.empty()) {
        const DepNode* dn = use_graph.find_best(state.selected_code);
        if (dn) { dep_view_init_impl(state, dn->id); return true; }
    }
    MathNode* cur = state.current();
    if (cur && !cur->code.empty() && cur->code != "ROOT") {
        const DepNode* dn = use_graph.find_best(cur->code);
        if (dn) { dep_view_init_impl(state, dn->id); return true; }
    }
    if (!use_graph.nodes().empty()) {
        dep_view_init_impl(state, use_graph.nodes().begin()->second.id);
        return true;
    }
    return false;
}

// ── dep_view_back ─────────────────────────────────────────────────────────────
// Retrocede al nodo anterior en el historial.
// Devuelve true si había historia y se navegó atrás.
// El caller (botón "< Atrás" o Backspace) debe resetear dep_cam si devuelve true.

bool dep_view_back(AppState& state, Camera2D& dep_cam) {
    if (s_nav_history.empty()) return false;

    std::string prev = s_nav_history.back();
    s_nav_history.pop_back();

    dep_view_init_impl(state, prev);   // sin empujar al historial

    const DepGraph& use_graph = get_dep_graph_for_const(state);
    const DepNode* dn = use_graph.get(prev);
    if (dn) { state.selected_code = dn->id; state.selected_label = dn->label; }
    g_kbnav.dep_sel_id = prev;
    dep_cam.target = { 0.f, 0.f };
    dep_cam.zoom = 1.f;
    return true;
}

bool dep_view_has_history() {
    return !s_nav_history.empty();
}

void dep_view_set_position_mode(AppState& state, bool on) {
    state.position_mode_active = on;
}

void draw_dep_view(AppState& state, Camera2D& dep_cam, Vector2 mouse) {
    const Theme& th = g_theme;
    const int    cw = CANVAS_W();
    const int    ch = TOP_H();

    bool canvas_blocked = overlay::blocks_mouse(mouse)
        || state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open || state.toolbar.config_open;
    bool in_canvas = mouse.x < cw && mouse.y >= UI_TOP() && mouse.y < g_split_y;

    if (!s_ready) {
        const char* msg = "Grafo de dependencias vacio o no cargado";
        int tw = MeasureTextF(msg, 18);
        DrawTextF(msg, (cw - tw) / 2, UI_TOP() + ch / 2, 18, ColorAlpha(th.text_dim, 0.6f));
        draw_zoom_buttons(dep_cam, mouse);
        draw_dep_canvas_buttons(state, dep_cam, mouse, canvas_blocked);
        draw_vscode_button(state, mouse);
        draw_mathlib_button(state, mouse);
        return;
    }

    if (dep_input_handle(state, dep_cam)) return;

    if (s_sim_step < SIM_MAX) {
        int steps = s_sim_step < SIM_MAX / 3 ? 10 : s_sim_step < SIM_MAX * 2 / 3 ? 5 : 2;
        for (int i = 0; i < steps && s_sim_step < SIM_MAX; i++)
            dep_sim_step(state);
    }

    bool over_node = false;
    {
        Vector2 wm = in_canvas
            ? GetScreenToWorld2D(mouse, dep_cam) : Vector2{ -99999.f, -99999.f };
        for (auto& [id, p] : s_phys)
            if (CheckCollisionPointRec(wm, { p.x - p.w * .5f, p.y - p.h * .5f, p.w, p.h }))
            {
                over_node = true; break;
            }
    }
    dep_camera_update(dep_cam, in_canvas, canvas_blocked, over_node,
        state.position_mode_active);

    std::string clicked_id = dep_scene_draw(state, dep_cam, mouse, in_canvas, canvas_blocked);

    if (!clicked_id.empty() && !g_dep_pivot.active) {
        const DepGraph& use_graph = get_dep_graph_for_const(state);
        dep_view_init(state, clicked_id);
        g_kbnav.dep_sel_id = clicked_id;
        const DepNode* dn = use_graph.get(clicked_id);
        if (dn) { state.selected_code = dn->id; state.selected_label = dn->label; }
        dep_cam.target = { 0.f, 0.f };
        dep_cam.zoom = 1.f;
        return;
    }

    bool shift_held = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    dep_hud_draw(state, shift_held);

    draw_zoom_buttons(dep_cam, mouse);
    draw_dep_canvas_buttons(state, dep_cam, mouse, canvas_blocked);
    draw_vscode_button(state, mouse);
    draw_mathlib_button(state, mouse);
}