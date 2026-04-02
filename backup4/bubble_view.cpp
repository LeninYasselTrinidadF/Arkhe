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
    if (state.mode == ViewMode::MSC2020 && state.msc_root)      stats_root = state.msc_root.get();
    else if (state.mode == ViewMode::Mathlib && state.mathlib_root)   stats_root = state.mathlib_root.get();
    else if (state.mode == ViewMode::Standard && state.standard_root)  stats_root = state.standard_root.get();
    bubble_stats_ensure(stats_root, state.crossref_map, state.mode);

    // ── Radios de dibujo ──────────────────────────────────────────────────────
    constexpr float CENTER_R = 178.0f;
    constexpr float CHILD_R_MIN = 38.0f;
    constexpr float CHILD_R_MAX = 80.0f;
    constexpr int   LBL_FONT = 15;   // debe coincidir con el valor en el bloque de dibujo

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

            // Expandir radio si el label lo necesita
            auto words = split_words_count(child->label);   // helper inline abajo
            float max_r = base_r;
            if (words == 1)
                max_r = base_r * 3.0f;   // 1 palabra: hasta 3× para que entre
            else if (words <= 3)
                max_r = base_r * 2.0f;   // 2-3 palabras: hasta 2× (duplicar)
            // 4+ palabras: no expandir radio, se usa "..." o abreviatura

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

    // ── Input: hover / pan / zoom ─────────────────────────────────────────────
    bool over_bubble = false;
    if (cur && in_canvas && !canvas_blocked) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x * wm.x + wm.y * wm.y) < CENTER_R)
            over_bubble = true;
        if (!over_bubble)
            for (auto& bp : layout) {
                float dx = wm.x - bp.x, dy = wm.y - bp.y;
                if (dx * dx + dy * dy < bp.r * bp.r) { over_bubble = true; break; }
            }
    }

    // Si estamos en modo posicionamiento, bloquear selección y permitir arrastre
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
    cam.offset = { (float)CX(), (float)CY() };

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

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);

        // ── Construir mapa de abreviaturas (resuelve colisiones una vez) ──────
        std::vector<std::string> all_labels;
        all_labels.reserve(cur->children.size());
        for (auto& child : cur->children)
            all_labels.push_back(child->label);
        auto abbrev_map = build_child_abbrev_map(all_labels);

        for (int i = 0; i < (int)layout.size(); i++) {
            const auto& child = cur->children[i];
            const BubbleStats& cs = bubble_stats_get(child->code);
            float bx = layout[i].x, by = layout[i].y, draw_r = layout[i].r;

            // Override with temp positions if present
            std::string keypref = state.cam_key_for(state.mode, child->code);
            // keypref like "MSC:CODE"; temp_positions uses same format
            auto tp_it = state.temp_positions.find(keypref);
            if (tp_it != state.temp_positions.end()) {
                bx = tp_it->second.x; by = tp_it->second.y;
            }

            Color col = bubble_color(child->color, cs);

            float ddx = wm.x - bx, ddy = wm.y - by;
            bool hov = (ddx * ddx + ddy * ddy) < (draw_r * draw_r)
                && in_canvas && !canvas_blocked;

            // Glow
            DrawCircleV({ bx, by }, draw_r + 3.0f,
                th_fade(child->color,
                    th.bubble_glow_alpha * (cs.connected ? 1.0f : 0.3f)));

            // Burbuja
            draw_bubble(state, bx, by, draw_r, col, child->texture_key);

            // Arco de progreso
            if (!child->children.empty() && cs.progress > 0.001f)
                draw_progress_arc(bx, by, draw_r, cs.progress, 3.5f,
                    arc_color(state.mode, cs.progress));

            // Hover ring + click
            if (hov) {
                DrawCircleLinesV({ bx, by }, draw_r + 6.0f, th.bubble_hover_ring);
                if (positioning) {
                    // start dragging on press
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        drag_idx = i;
                        drag_start_mouse = GetMousePosition();
                        drag_start_pos = { bx, by };
                    }
                }
                else {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        state.selected_code = child->code;
                        state.selected_label = child->label;
                        if (!child->children.empty()) state.push(child);
                    }
                }
            }

            // ── Label multilínea ─────────────────────────────────────────────
            bool has_tex = !child->texture_key.empty();

            // Calcular líneas a dibujar
            BubbleLabelLines lbl_result = make_label_lines(child->label, draw_r, LBL_FONT);

            std::vector<std::string> draw_lines;
            if (lbl_result.needs_abbrev) {
                // Usar abreviatura resuelta (sin colisiones)
                auto it = abbrev_map.find(child->label);
                std::string abbr = (it != abbrev_map.end()) ? it->second : child->label.substr(0, 4);
                draw_lines.push_back(abbr);
            }
            else {
                draw_lines = lbl_result.lines;
            }

            // Altura total del bloque de texto
            int line_h = MeasureTextF("Ag", LBL_FONT);
            int line_gap = 2;
            int total_h = (int)draw_lines.size() * line_h +
                ((int)draw_lines.size() - 1) * line_gap;

            // Posición Y base: si hay textura, debajo del centro; si no, centrado
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

            // Indicador "desconectado"
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

    // Handle dragging outside Mode2D (to read raw mouse)
    if (positioning) {
        if (drag_idx >= 0) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 cur_mouse = GetMousePosition();
                Vector2 delta = { (cur_mouse.x - drag_start_mouse.x) / cam.zoom,
                                  (cur_mouse.y - drag_start_mouse.y) / cam.zoom };
                float nbx = drag_start_pos.x + delta.x;
                float nby = drag_start_pos.y + delta.y;
                // write back to temp_positions using key format
                auto child = cur->children[drag_idx];
                std::string key = state.cam_key_for(state.mode, child->code);
                state.temp_positions[key] = { nbx, nby };
            }
            else {
                // released
                drag_idx = -1;
            }
        }
    }

    // ── Controles HUD (fuera de Mode2D) ──────────────────────────────────────
    draw_zoom_buttons(cam, mouse);
    draw_canvas_buttons(state, mouse, canvas_blocked);
}