#include "bubble_view.hpp"
#include "bubble/bubble_draw.hpp"
#include "bubble/bubble_layout.hpp"
#include "bubble/bubble_controls.hpp"
#include "bubble/bubble_stats.hpp"
#include "core/overlay.hpp"
#include "core/theme.hpp"
#include "core/font_manager.hpp"
#include "constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include "../data/vscode_bridge.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

// Cuenta palabras sin construir un vector (para la lógica de expansión de radio)
static int split_words_count(const std::string& s) {
    std::istringstream ss(s);
    std::string w;
    int n = 0;
    while (ss >> w) ++n;
    return n;
}

// Inserta espacios en transiciones minúscula→Mayúscula y dígito→Mayúscula.
// Solo para display: no modifica datos del nodo.
// Ejemplos: "LinearAlgebra" → "Linear Algebra"
//           "CompactlyGenerated" → "Compactly Generated"
//           "smul_mk" → "smul_mk"   (sin cambio)
static std::string split_camel(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (int i = 0; i < (int)s.size(); ++i) {
        if (i > 0 && std::isupper((unsigned char)s[i])
            && (std::islower((unsigned char)s[i - 1])
                || std::isdigit((unsigned char)s[i - 1])))
            out += ' ';
        out += s[i];
    }
    return out;
}

// ── Nombre del modo (solo necesario aquí para el label central) ───────────────
static const char* mode_name(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return "MSC2020";
    case ViewMode::Mathlib:  return "Mathlib";
    case ViewMode::Standard: return "Estandar";
    }
    return "";
}


// ── draw_bubble_view ──────────────────────────────────────────────────────────

void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    MathNode* cur = state.current();

    bool in_canvas = mouse.x < CANVAS_W()
        && mouse.y >= UI_TOP()
        && mouse.y < g_split_y;

    bool any_panel = state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open || state.toolbar.config_open;
    bool canvas_blocked = overlay::blocks_mouse(mouse) || any_panel;

    // ── Stats de burbujas ─────────────────────────────────────────────────────
    const MathNode* stats_root = nullptr;
    if (state.mode == ViewMode::MSC2020 && state.msc_root)           stats_root = state.msc_root.get();
    else if (state.mode == ViewMode::Mathlib && state.mathlib_root)   stats_root = state.mathlib_root.get();
    else if (state.mode == ViewMode::Standard && state.standard_root) stats_root = state.standard_root.get();
    bubble_stats_ensure(stats_root, state.crossref_map, state.mode);

    // ── Radios de dibujo ──────────────────────────────────────────────────────
    constexpr float CENTER_R = 178.0f;
    constexpr float CHILD_R_MIN = 38.0f;
    constexpr float CHILD_R_MAX = 80.0f;
    constexpr int   LBL_FONT = 15;

    int max_weight = 1;
    if (cur)
        for (auto& child : cur->children)
            max_weight = std::max(max_weight, bubble_stats_get(child->code).weight);

    std::vector<float> draw_radii;
    if (cur) {
        draw_radii.reserve(cur->children.size());
        for (auto& child : cur->children) {
            float base_r = proportional_radius(
                bubble_stats_get(child->code).weight,
                max_weight, CHILD_R_MIN, CHILD_R_MAX);

            auto words = split_words_count(child->label);
            float max_r = base_r;
            if (words == 1)
                max_r = base_r * 3.0f;
            else if (words <= 3)
                max_r = base_r * 2.0f;

            float final_r = (words <= 3)
                ? fit_radius_for_label(child->label, base_r, max_r, LBL_FONT)
                : base_r;

            draw_radii.push_back(final_r);
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    std::vector<BubblePos> layout;
    if (cur && !cur->children.empty())
        layout = compute_layout(cur, draw_radii, CENTER_R);

    // ── Detección de nivel hoja (Mathlib) ─────────────────────────────────────
    // Cuando estamos en Mathlib y todos los hijos de cur son hojas (sin hijos
    // propios), cur representa un archivo .lean. La burbuja central se vuelve
    // pulsable para seleccionar el archivo directamente sin hacer push.
    bool at_leaf_level = state.mode == ViewMode::Mathlib
        && cur
        && cur->code != "ROOT"
        && !cur->children.empty()
        && cur->children[0]->children.empty();

    // ── Input: hover / pan / zoom ─────────────────────────────────────────────
    // over_center se rastrea por separado para el comportamiento del archivo
    bool over_center = false;
    bool over_bubble = false;
    if (cur && in_canvas && !canvas_blocked) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        float center_dist2 = wm.x * wm.x + wm.y * wm.y;
        if (center_dist2 < CENTER_R * CENTER_R) {
            over_bubble = true;
            over_center = true;
        }
        if (!over_center) {
            for (auto& bp : layout) {
                float dx = wm.x - bp.x, dy = wm.y - bp.y;
                if (dx * dx + dy * dy < bp.r * bp.r) { over_bubble = true; break; }
            }
        }
    }

    // ── Click en burbuja central (nivel hoja Mathlib) ─────────────────────────
    if (at_leaf_level && over_center && !canvas_blocked
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state.selected_code = cur->code;
        state.selected_label = cur->label;
    }

    bool positioning = state.position_mode_active && in_canvas && !canvas_blocked;
    static int drag_idx = -1;
    static Vector2 drag_start_mouse = { 0,0 };
    static Vector2 drag_start_pos = { 0,0 };

    if (in_canvas && !canvas_blocked) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble && !positioning)
            || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) {
            Vector2 d = GetMouseDelta();
            cam.target.x -= d.x / cam.zoom;
            cam.target.y -= d.y / cam.zoom;
        }
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && in_canvas && !canvas_blocked) {
        cam.offset = mouse;
        cam.target = GetScreenToWorld2D(mouse, cam);
        cam.zoom = Clamp(cam.zoom + wheel * 0.1f, 0.05f, 5.0f);
    }
    cam.offset = { (float)CX(), (float)CCY() };

    // ── Dibujo ────────────────────────────────────────────────────────────────
    BeginScissorMode(0, UI_TOP(), CANVAS_W(), TOP_H());
    BeginMode2D(cam);

    // Líneas de conexión
    if (cur && !cur->children.empty()) {
        for (int i = 0; i < (int)layout.size(); i++) {
            const auto& child = cur->children[i];
            float lx = layout[i].x, ly = layout[i].y;
            float len = sqrtf(lx * lx + ly * ly);
            if (len < 0.001f) continue;
            float nx = lx / len, ny = ly / len;
            const BubbleStats& cs = bubble_stats_get(child->code);
            float alpha = cs.connected
                ? th.bubble_line_alpha
                : th.bubble_line_alpha * 0.4f;
            DrawLineEx({ nx * CENTER_R, ny * CENTER_R },
                { lx - nx * layout[i].r, ly - ny * layout[i].r },
                1.5f, th_fade(child->color, alpha));
        }
    }

    // Burbuja central
    DrawCircleLines(0, 0, CENTER_R + 1.0f, th.bubble_ring);
    Color center_col = (cur && cur->code != "ROOT")
        ? th_fade(cur->color, th.bubble_center_alpha)
        : th.bubble_center_flat;
    draw_bubble(state, 0.0f, 0.0f, CENTER_R, center_col,
        cur ? cur->texture_key : "");

    // Arco de progreso central
    if (cur && cur->code != "ROOT" && !cur->children.empty()) {
        const BubbleStats& cs = bubble_stats_get(cur->code);
        draw_progress_arc(0.0f, 0.0f, CENTER_R, cs.progress, 5.0f,
            arc_color(state.mode, cs.progress));
    }

    // Label central
    const char* clabel = (!cur || cur->code == "ROOT")
        ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureTextF(cl_str.c_str(), 30);
    int cly = (cur && !cur->texture_key.empty()) ? 140 : -15;
    DrawTextF(cl_str.c_str(), -clw / 2, cly, 30, th.bubble_label_center);

    // ── Feedback visual de la burbuja central pulsable (nivel hoja Mathlib) ───
    if (at_leaf_level) {
        bool center_selected = (state.selected_code == cur->code);

        // Ring de selección activa (siempre visible cuando está seleccionado)
        if (center_selected)
            DrawCircleLinesV({ 0, 0 }, CENTER_R + 8.0f, th.accent_hover);

        // Ring de hover + hint
        if (over_center) {
            DrawCircleLinesV({ 0, 0 }, CENTER_R + 6.0f, th.bubble_hover_ring);
            const char* hint = center_selected ? "[ seleccionado ]" : "[ seleccionar archivo ]";
            int hw = MeasureTextF(hint, 10);
            Color hint_col = center_selected
                ? th_alpha(th.accent_hover)
                : th_alpha(th.text_secondary);
            DrawTextF(hint, -hw / 2, (int)(CENTER_R - 24), 10, hint_col);
        }
    }

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);

        std::vector<std::string> all_labels;
        all_labels.reserve(cur->children.size());
        for (auto& child : cur->children)
            all_labels.push_back(state.mode == ViewMode::Mathlib
                ? split_camel(child->label) : child->label);
        auto abbrev_map = build_child_abbrev_map(all_labels);

        for (int i = 0; i < (int)layout.size(); i++) {
            const auto& child = cur->children[i];
            const BubbleStats& cs = bubble_stats_get(child->code);
            float bx = layout[i].x, by = layout[i].y, draw_r = layout[i].r;

            std::string keypref = state.cam_key_for(state.mode, child->code);
            auto tp_it = state.temp_positions.find(keypref);
            if (tp_it != state.temp_positions.end()) {
                bx = tp_it->second.x; by = tp_it->second.y;
            }

            Color col = bubble_color(child->color, cs);

            float ddx = wm.x - bx, ddy = wm.y - by;
            bool hov = (ddx * ddx + ddy * ddy) < (draw_r * draw_r)
                && in_canvas && !canvas_blocked;

            DrawCircleV({ bx, by }, draw_r + 3.0f,
                th_fade(child->color,
                    th.bubble_glow_alpha * (cs.connected ? 1.0f : 0.3f)));

            draw_bubble(state, bx, by, draw_r, col, child->texture_key);

            if (!child->children.empty() && cs.progress > 0.001f)
                draw_progress_arc(bx, by, draw_r, cs.progress, 3.5f,
                    arc_color(state.mode, cs.progress));

            if (hov) {
                DrawCircleLinesV({ bx, by }, draw_r + 6.0f, th.bubble_hover_ring);
                if (positioning) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        drag_idx = i;
                        drag_start_mouse = GetMousePosition();
                        drag_start_pos = { bx, by };
                    }
                }
                else {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        // Para nodos hoja de Mathlib (lemmas/defs/theorems),
                        // construir el código completamente calificado:
                        // "Mathlib.A.B.C" + "." + "lemma_foo"
                        if (state.mode == ViewMode::Mathlib
                            && child->children.empty()
                            && cur && cur->code != "ROOT")
                        {
                            std::string norm = child->code;
                            for (char& c : norm) if (c == ' ') c = '_';
                            state.selected_code = cur->code + "." + norm;
                        }
                        else {
                            state.selected_code = child->code;
                        }
                        state.selected_label = child->label;
                        if (!child->children.empty()) state.push(child);
                    }
                }
            }

            bool has_tex = !child->texture_key.empty();

            // Solo display: CamelCase → palabras separadas en modo Mathlib
            const std::string display_label = (state.mode == ViewMode::Mathlib)
                ? split_camel(child->label) : child->label;

            BubbleLabelLines lbl_result = make_label_lines(display_label, draw_r, LBL_FONT);

            std::vector<std::string> draw_lines;
            if (lbl_result.needs_abbrev) {
                auto it = abbrev_map.find(display_label);
                std::string abbr = (it != abbrev_map.end()) ? it->second : display_label.substr(0, 4);
                draw_lines.push_back(abbr);
            }
            else {
                draw_lines = lbl_result.lines;
            }

            int line_h = MeasureTextF("Ag", LBL_FONT);
            int line_gap = 2;
            int total_h = (int)draw_lines.size() * line_h +
                ((int)draw_lines.size() - 1) * line_gap;

            int base_y;
            if (has_tex)
                base_y = (int)(by + draw_r * 0.55f);
            else
                base_y = (int)(by - total_h / 2);

            for (int li = 0; li < (int)draw_lines.size(); li++) {
                int tw = MeasureTextF(draw_lines[li].c_str(), LBL_FONT);
                int lx = (int)(bx - tw * 0.5f);
                int ly = base_y + li * (line_h + line_gap);
                DrawTextF(draw_lines[li].c_str(), lx, ly, LBL_FONT,
                    th.bubble_label_child);
            }

            if (!cs.connected && !child->children.empty()) {
                int iw = MeasureTextF("?", 14);
                DrawTextF("?", (int)(bx - iw * 0.5f), (int)(by - draw_r - 18),
                    14, th_alpha(th.text_dim));
            }
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        int tw = MeasureTextF(msg, 21);
        DrawTextF(msg, -tw / 2, 210, 21, th.bubble_empty_msg);
    }

    EndMode2D();
    EndScissorMode();

    // Handle dragging outside Mode2D
    if (positioning) {
        if (drag_idx >= 0) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 cur_mouse = GetMousePosition();
                Vector2 delta = { (cur_mouse.x - drag_start_mouse.x) / cam.zoom,
                                  (cur_mouse.y - drag_start_mouse.y) / cam.zoom };
                float nbx = drag_start_pos.x + delta.x;
                float nby = drag_start_pos.y + delta.y;
                auto child = cur->children[drag_idx];
                std::string key = state.cam_key_for(state.mode, child->code);
                state.temp_positions[key] = { nbx, nby };
            }
            else {
                drag_idx = -1;
            }
        }
    }

    // ── Controles HUD ─────────────────────────────────────────────────────────
    draw_zoom_buttons(cam, mouse);
    draw_canvas_buttons(state, mouse, canvas_blocked);
    draw_vscode_button(state, mouse);
    draw_mathlib_button(state, mouse);
}