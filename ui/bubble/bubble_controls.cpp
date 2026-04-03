#include "bubble_controls.hpp"
#include "dep_view.hpp"
#include "../../data/position_state.hpp"
#include "../../data/vscode_bridge.hpp"
#include "../core/overlay.hpp"
#include "../core/theme.hpp"
#include "../core/font_manager.hpp"
#include "../core/nine_patch.hpp"
#include "../core/skin.hpp"
#include "../constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <queue>

// ── Helpers de modo ───────────────────────────────────────────────────────────

static const char* mode_name(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return "MSC2020";
    case ViewMode::Mathlib:  return "Mathlib";
    case ViewMode::Standard: return "Estandar";
    }
    return "";
}
static ViewMode mode_prev(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Standard;
    case ViewMode::Mathlib:  return ViewMode::MSC2020;
    case ViewMode::Standard: return ViewMode::Mathlib;
    }
    return ViewMode::MSC2020;
}
static ViewMode mode_next(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return ViewMode::Mathlib;
    case ViewMode::Mathlib:  return ViewMode::Standard;
    case ViewMode::Standard: return ViewMode::MSC2020;
    }
    return ViewMode::MSC2020;
}

// ── draw_arrow ────────────────────────────────────────────────────────────────

bool draw_arrow(int cx, int cy, bool left, Vector2 mouse) {
    if (overlay::blocks_mouse(mouse)) return false;
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx - HW - 4), (float)(cy - HH - 4),
                    (float)(HW * 2 + 8),  (float)(HH * 2 + 8) };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)
        DrawTriangle({ (float)(cx + HW),(float)(cy - HH) },
            { (float)(cx + HW),(float)(cy + HH) },
            { (float)(cx - HW),(float)cy }, col);
    else
        DrawTriangle({ (float)(cx - HW),(float)(cy + HH) },
            { (float)(cx - HW),(float)(cy - HH) },
            { (float)(cx + HW),(float)cy }, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_mode_switcher ────────────────────────────────────────────────────────

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int lbl_sz = 20;
    int nw = MeasureTextF(mname, lbl_sz);
    int arrow_w = g_fonts.scale(50);
    int bar_w = nw + arrow_w * 2 + g_fonts.scale(16);
    int bar_x = CX() - bar_w / 2;
    int lbl_h = MeasureTextF("Ag", lbl_sz);
    int bar_h = lbl_h + g_fonts.scale(16);
    int bar_y = UI_TOP() + g_fonts.scale(8);

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y,
            (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    int sep_top = bar_y + g_fonts.scale(5), sep_bot = bar_y + bar_h - g_fonts.scale(5);
    DrawLine(bar_x + arrow_w, sep_top, bar_x + arrow_w, sep_bot, th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - arrow_w, sep_top, bar_x + bar_w - arrow_w, sep_bot, th_alpha(th.ctrl_text_dim));

    DrawTextF(mname, CX() - nw / 2, bar_y + (bar_h - lbl_h) / 2, lbl_sz, th.ctrl_text);
    if (draw_arrow(bar_x + arrow_w / 2, bar_y + bar_h / 2, true, mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - arrow_w / 2, bar_y + bar_h / 2, false, mouse)) state.mode = mode_next(state.mode);
}

// ── draw_zoom_buttons ─────────────────────────────────────────────────────────

void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int btn_sz = MeasureTextF("+", 20) + g_fonts.scale(14);
    int bx = 14;

    const int vs_top = g_split_y - 22 - 10;
    int by = vs_top - 40 - (btn_sz * 2 + g_fonts.scale(6));
    if (by < UI_TOP() + 10) by = UI_TOP() + 10;

    Rectangle rp = { (float)bx, (float)by, (float)btn_sz, (float)btn_sz };
    Rectangle rm = { (float)bx, (float)(by + btn_sz + g_fonts.scale(6)),
                     (float)btn_sz, (float)btn_sz };
    bool hp = CheckCollisionPointRec(mouse, rp);
    bool hm = CheckCollisionPointRec(mouse, rm);

    auto draw_btn = [&](Rectangle r, bool hov) {
        if (g_skin.button.valid())
            g_skin.button.draw(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        else {
            DrawRectangleRec(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
            DrawRectangleLinesEx(r, 1, th.ctrl_border);
        }
        };
    draw_btn(rp, hp); draw_btn(rm, hm);

    int sym_sz = 20, sym_w = MeasureTextF("+", sym_sz), sym_h = MeasureTextF("A", sym_sz);
    DrawTextF("+", (int)rp.x + (btn_sz - sym_w) / 2, (int)rp.y + (btn_sz - sym_h) / 2,
        sym_sz, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", (int)rm.x + (btn_sz - MeasureTextF("-", sym_sz)) / 2,
        (int)rm.y + (btn_sz - sym_h) / 2,
        sym_sz, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 6.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 6.0f);
    }

    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int lbl_sz = 11, tw = MeasureTextF(zbuf, lbl_sz);
    DrawTextF(zbuf, bx + btn_sz / 2 - tw / 2,
        (int)rm.y + btn_sz + g_fonts.scale(4), lbl_sz, th_alpha(th.ctrl_text_dim));
}

// ── Botones externos (VS Code / Mathlib) — helper compartido ─────────────────

static void draw_ext_btn(Vector2 mouse, int bx, int by,
    const char* lbl, void(*on_click)(const AppState&), const AppState& state)
{
    const Theme& th = g_theme;
    constexpr int bw = 54, bh = 22;
    Rectangle r = { (float)bx, (float)by, bw, bh };
    bool hov = CheckCollisionPointRec(mouse, r);
    DrawRectangleRec(r, hov ? th.bg_button_hover : th_alpha(th.bg_button));
    DrawRectangleLinesEx(r, 1.f, th_alpha(th.ctrl_border));
    int tw = MeasureTextF(lbl, 11);
    DrawTextF(lbl, bx + (bw - tw) / 2, by + (bh - 11) / 2, 11, th.ctrl_text);
    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        on_click(state);
}

void draw_vscode_button(AppState& state, Vector2 mouse) {
    draw_ext_btn(mouse, 10, g_split_y - 32, "\xE2\x86\x92 VS",
        bridge_launch_vscode, state);
}

void draw_mathlib_button(AppState& state, Vector2 mouse) {
    draw_ext_btn(mouse, 10 + 54 + 6, g_split_y - 32, "ML",
        bridge_launch_vscode_mathlib, state);
}

// ── canvas_btn_impl ───────────────────────────────────────────────────────────

static constexpr int BTN_FONT = 14;
static constexpr int BTN_PAD_X = 10;
static constexpr int BTN_PAD_Y = 7;

static void btn_dims(const char* label, float& bw, float& bh) {
    int tw = MeasureTextF(label, BTN_FONT);
    int font_h = MeasureTextF("Ag", BTN_FONT);
    bw = (float)(tw + g_fonts.scale(BTN_PAD_X) * 2);
    bh = (float)(font_h + g_fonts.scale(BTN_PAD_Y) * 2);
}

static bool canvas_btn_impl(const Theme& th, Vector2 mouse, bool canvas_blocked,
    float bx, float by, const char* label,
    bool enabled, bool active = false,
    float* out_bw = nullptr, float* out_bh = nullptr)
{
    float bw, bh;
    btn_dims(label, bw, bh);
    if (out_bw) *out_bw = bw;
    if (out_bh) *out_bh = bh;

    bool hov = enabled && !canvas_blocked
        && CheckCollisionPointRec(mouse, { bx, by, bw, bh });

    Color bg = active ? th.accent
        : hov ? th.bg_button_hover
        : enabled ? th_alpha(th.bg_button)
        : Color{ th.bg_button.r, th.bg_button.g, th.bg_button.b, 80 };
    Color txt = active ? th.bg_app
        : hov ? th.ctrl_text
        : enabled ? th_alpha(th.ctrl_text_dim)
        : Color{ th.ctrl_text_dim.r, th.ctrl_text_dim.g, th.ctrl_text_dim.b, 80 };
    Color brd = active ? th.accent
        : enabled ? th.ctrl_border
        : th_alpha(th.border);

    if (g_skin.button.valid())
        g_skin.button.draw(bx, by, bw, bh, bg);
    else {
        DrawRectangleRec({ bx, by, bw, bh }, bg);
        DrawRectangleLinesEx({ bx, by, bw, bh }, active ? 2.f : 1.f, brd);
    }
    DrawTextF(label,
        (int)bx + g_fonts.scale(BTN_PAD_X),
        (int)by + g_fonts.scale(BTN_PAD_Y),
        BTN_FONT, txt);

    return hov && enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static float btn_gap() { return (float)g_fonts.scale(6); }

// ── nav_to_dep_node ────────────────────────────────────────────────────────────
// Dado un dep_focus_id, navega el nav_stack de burbujas hasta que el padre
// del nodo sea el tope (para que el nodo quede visible como burbuja hija),
// y deja selected_code apuntando a él.
//
// Estrategia: BFS desde la raíz del árbol actual buscando el nodo cuyo code
// coincide con dep_focus_id (exacto) o cuyo code es prefijo del dep_focus_id.
// Cuando lo encuentra, reconstruye el nav_stack con el camino hasta su padre.

static void nav_to_dep_node(AppState& state, const std::string& dep_id) {
    if (dep_id.empty()) return;

    // Raíz del árbol actual
    std::shared_ptr<MathNode> root;
    switch (state.mode) {
    case ViewMode::MSC2020:  root = state.msc_root;      break;
    case ViewMode::Mathlib:  root = state.mathlib_root;  break;
    case ViewMode::Standard: root = state.standard_root; break;
    }
    if (!root) return;

    // BFS: cada entrada es (nodo, camino desde la raíz inclusive)
    using Path = std::vector<std::shared_ptr<MathNode>>;
    std::queue<std::pair<std::shared_ptr<MathNode>, Path>> q;
    q.push({ root, { root } });

    std::shared_ptr<MathNode> found;
    Path found_path;

    while (!q.empty()) {
        auto [node, path] = q.front(); q.pop();

        // Coincidencia exacta, o el código del árbol es prefijo del dep_id
        // (ej. dep_id="34A12", node->code="34A12" o "34A" o "34")
        bool match = (node->code == dep_id)
            || (!dep_id.empty() && dep_id.rfind(node->code, 0) == 0
                && dep_id.size() > node->code.size()
                && (dep_id[node->code.size()] == '.' || dep_id[node->code.size()] == '_'));

        if (match) {
            // Si tiene hijos, preferir quedar en este nodo como tope
            // Si es hoja, quedarnos en el padre (ya cargado al iterar hijos)
            if (!node->children.empty()) {
                found = node;
                found_path = path;
            }
            else if (path.size() >= 2) {
                // El padre es path[size-2]; el nodo es la selección
                found_path = Path(path.begin(), path.end() - 1);
                found = path[path.size() - 2];
            }
            else {
                found = node;
                found_path = path;
            }
            break;
        }

        for (auto& child : node->children) {
            Path child_path = path;
            child_path.push_back(child);
            q.push({ child, child_path });
        }
    }

    if (!found) return;  // nodo no existe en este árbol — no tocar nada

    // Reconstruir nav_stack con el camino encontrado
    state.nav_stack.clear();
    for (auto& n : found_path)
        state.nav_stack.push_back(n);

    // Dejar selected_code apuntando al nodo del grafo
    state.selected_code = dep_id;
    state.selected_label = found_path.back()->label;  // label del nodo visible
    state.resource_scroll = 0.f;
}

// ── draw_canvas_buttons (modo Burbujas) ───────────────────────────────────────

void draw_canvas_buttons(AppState& state, Vector2 mouse, bool canvas_blocked) {
    const Theme& th = g_theme;
    const float BX = 14.f;
    float by = (float)(UI_TOP() + 10), bw, bh;

    // [Casa]
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]", true,
        false, &bw, &bh)
        && state.nav_stack.size() > 1) {
        auto root = state.nav_stack[0];
        state.nav_stack.clear();
        state.nav_stack.push_back(root);
        state.selected_code.clear();
        state.selected_label.clear();
        state.resource_scroll = 0.f;
    }
    by += bh + btn_gap();

    // Burbujas ↔ Dependencias
    const DepGraph& use_graph = get_dep_graph_for_const(state);
    bool dep_loaded = !use_graph.empty();
    bool dep_active = state.dep_view_active;
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by,
        dep_active ? "Burbujas" : "Dependencias",
        dep_active || dep_loaded, dep_active, &bw, &bh)) {
        if (dep_active) {
            // Dep → Burbujas: navegar al nodo enfocado en el grafo
            nav_to_dep_node(state, state.dep_focus_id);
            state.dep_view_active = false;
        }
        else {
            // Burbujas → Dep: abrir en el nodo actual/seleccionado
            dep_view_init_from_selection(state);
            state.dep_view_active = true;
        }
    }
    if (!dep_loaded) {
        int hsz = 10, tw = MeasureTextF("(sin deps.json)", hsz);
        DrawTextF("(sin deps.json)",
            (int)(BX + bw / 2 - tw / 2), (int)(by + bh + 2),
            hsz, ColorAlpha(th.text_dim, 0.4f));
        by += (float)g_fonts.scale(hsz) + 4.f;
    }
    by += bh + btn_gap();

    // Posicion
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Posicion",
        true, state.position_mode_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active) position_state_save(state);
    }
    by += bh + btn_gap();

    // < Atrás
    if (!dep_active && state.can_go_back())
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "< Atras",
            true, false, &bw, &bh))
            state.pop();
}

// ── draw_dep_canvas_buttons (modo Dependencias) ───────────────────────────────

void draw_dep_canvas_buttons(AppState& state, Camera2D& dep_cam,
    Vector2 mouse, bool canvas_blocked)
{
    const Theme& th = g_theme;
    const float BX = 14.f;
    float by = (float)(UI_TOP() + 10), bw, bh;

    // [Casa]
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "[Casa]",
        true, false, &bw, &bh)) {
        dep_cam.target = { 0.f, 0.f }; dep_cam.zoom = 1.f;
        const DepGraph& g = get_dep_graph_for_const(state);
        if (!g.empty()) dep_view_init(state, g.nodes().begin()->second.id);
    }
    by += bh + btn_gap();

    // Burbujas (volver) — navega el árbol al nodo enfocado en el grafo
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Burbujas",
        true, false, &bw, &bh)) {
        nav_to_dep_node(state, state.dep_focus_id);
        state.dep_view_active = false;
    }
    by += bh + btn_gap();

    // Posicion
    if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "Posicion",
        true, state.position_mode_active, &bw, &bh)) {
        state.position_mode_active = !state.position_mode_active;
        if (!state.position_mode_active) position_state_save(state);
    }
    by += bh + btn_gap();

    // < Atrás
    if (!state.dep_focus_id.empty()) {
        auto parents = get_dep_graph_for_const(state).get_dependents(state.dep_focus_id);
        if (canvas_btn_impl(th, mouse, canvas_blocked, BX, by, "< Atras",
            !parents.empty(), false, &bw, &bh))
            dep_view_init(state, parents[0]);
    }
}