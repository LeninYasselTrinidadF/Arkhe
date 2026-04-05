#include "ui/views/dep/dep_input.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "ui/views/dep/dep_pivot.hpp"
#include "ui/views/dep_view.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "data/loaders/dep_graph.hpp"
#include "raylib.h"
#include <cmath>

// ── dep_nearest_in_dir ────────────────────────────────────────────────────────
// Encuentra el nodo más cercano en la dirección (dx,dy) desde cur_id.

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

// ── dep_input_handle ──────────────────────────────────────────────────────────

bool dep_input_handle(AppState& state, Camera2D& dep_cam) {
    const DepGraph& use_graph = get_dep_graph_for_const(state);

    bool ctrl_held = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift_held = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // ── Shift+flechas: pan de cámara (siempre, incluso con Ctrl) ─────────────
    if (g_kbnav.in(FocusZone::Canvas) && shift_held) {
        const float PAN_SPEED = 12.f / dep_cam.zoom;
        if (IsKeyDown(KEY_LEFT))  dep_cam.target.x -= PAN_SPEED;
        if (IsKeyDown(KEY_RIGHT)) dep_cam.target.x += PAN_SPEED;
        if (IsKeyDown(KEY_UP))    dep_cam.target.y -= PAN_SPEED;
        if (IsKeyDown(KEY_DOWN))  dep_cam.target.y += PAN_SPEED;
    }

    // ── Ctrl: gestión del pivote ──────────────────────────────────────────────
    if (g_kbnav.in(FocusZone::Canvas)) {
        if (ctrl_held && !g_dep_pivot.active)
            dep_pivot_activate(state);
        else if (!ctrl_held && g_dep_pivot.active)
            dep_pivot_deactivate();
    }
    else {
        if (g_dep_pivot.active) dep_pivot_deactivate();
    }

    // ── Navegación por teclado (no shift) ─────────────────────────────────────
    if (!g_kbnav.in(FocusZone::Canvas) || shift_held)
        return false;

    // Sincronizar selector geográfico
    if (g_kbnav.dep_sel_id.empty() ||
        s_phys.find(g_kbnav.dep_sel_id) == s_phys.end())
        g_kbnav.dep_sel_id = s_focus_id;

    bool pivot_nav = ctrl_held && !shift_held;

    if (pivot_nav) {
        // ── Modo pivote: navegación por anillos ───────────────────────────────
        bool repivot = false;
        dep_pivot_handle_keys(repivot);

        if (repivot) {
            std::string new_pivot = g_dep_pivot.pivot_id;
            dep_view_init(state, new_pivot);
            g_kbnav.dep_sel_id = new_pivot;
            const DepNode* dn = use_graph.get(new_pivot);
            if (dn) { state.selected_code = dn->id; state.selected_label = dn->label; }
            dep_cam.target = { 0.f, 0.f };
            dep_cam.zoom = 1.f;
            dep_pivot_rebuild(state);
            return true;   // frame debe reiniciarse
        }
    }
    else {
        // ── Modo geográfico: navegación espacial ──────────────────────────────
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
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            std::string sel = g_kbnav.dep_sel_id;
            if (!sel.empty()) {
                dep_view_init(state, sel);
                const DepNode* dn = use_graph.get(sel);
                if (dn) { state.selected_code = dn->id; state.selected_label = dn->label; }
                dep_cam.target = { 0.f, 0.f };
                dep_cam.zoom = 1.f;
                g_kbnav.dep_sel_id = sel;
                return true;
            }
        }
        // Backspace → retroceder al nodo anterior en el historial
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (dep_view_back(state, dep_cam))
                return true;
        }
    }

    return false;
}