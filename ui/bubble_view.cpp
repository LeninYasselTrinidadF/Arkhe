#include "bubble_view.hpp"
#include "core/overlay.hpp"
#include "core/font_manager.hpp"
#include "bubble_stats.hpp"
#include "core/theme.hpp"
#include "core/nine_patch.hpp"
#include "core/skin.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

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

// ── Color de burbuja con oscuridad según stats ────────────────────────────────
static Color bubble_color(Color base, const BubbleStats& st) {
    if (st.connected) return base;
    float factor = 0.40f;
    return Color{
        (unsigned char)(base.r * factor),
        (unsigned char)(base.g * factor),
        (unsigned char)(base.b * factor),
        base.a
    };
}

// ── Arco de progreso ──────────────────────────────────────────────────────────
static void draw_progress_arc(float cx, float cy, float r, float progress,
    float thick, Color col)
{
    if (progress <= 0.001f) return;
    float angle_deg = progress * 360.0f;
    float outer = r + thick + 2.0f;
    float inner = r + 2.0f;
    int segments = std::max(6, (int)(angle_deg / 4.0f));
    DrawRing({ cx, cy }, inner, outer, -90.0f, -90.0f + angle_deg, segments, col);
}

// ── Radio proporcional ────────────────────────────────────────────────────────
static float proportional_radius(int weight, int max_weight,
    float min_r, float max_r)
{
    if (max_weight <= 1) return (min_r + max_r) * 0.5f;
    float t = std::sqrt((float)weight / (float)max_weight);
    return min_r + t * (max_r - min_r);
}

// ── Label con fuente custom ───────────────────────────────────────────────────
// FONT CHANGE: escalado a 1.5× — antes usaba 7.0f de divisor, ahora 4.8f
// para que quepan más caracteres en el mismo radio (fuente más grande → labels
// que antes cabían ahora necesitan más espacio, así que recortamos antes).
static std::string short_label(const std::string& label, float radius) {
    int mc = (int)(radius * 1.6f / 4.8f);
    if (mc < 1) mc = 1;
    if ((int)label.size() <= mc) return label;
    return label.substr(0, mc - 1) + ".";
}

// ── draw_arrow ────────────────────────────────────────────────────────────────
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse) {
    if (overlay::blocks_mouse(mouse)) return false;
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx - HW - 4),(float)(cy - HH - 4),(float)(HW * 2 + 8),(float)(HH * 2 + 8) };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)  DrawTriangle({ (float)(cx + HW),(float)(cy - HH) }, { (float)(cx + HW),(float)(cy + HH) }, { (float)(cx - HW),(float)cy }, col);
    else       DrawTriangle({ (float)(cx - HW),(float)(cy + HH) }, { (float)(cx - HW),(float)(cy - HH) }, { (float)(cx + HW),(float)cy }, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── Switcher de modo ──────────────────────────────────────────────────────────
void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    // FONT CHANGE: 17 → 26 (≈1.5×)
    int nw = MeasureTextF(mname, 26);
    int bar_w = nw + 130, bar_x = CX() - bar_w / 2, bar_y = UI_TOP() + 10, bar_h = 40;

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    }
    DrawLine(bar_x + 50, bar_y + 6, bar_x + 50, bar_y + bar_h - 6, th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - 50, bar_y + 6, bar_x + bar_w - 50, bar_y + bar_h - 6, th_alpha(th.ctrl_text_dim));
    // FONT CHANGE: 17 → 26
    DrawTextF(mname, CX() - nw / 2, bar_y + 8, 26, th.ctrl_text);

    if (draw_arrow(bar_x + 25, bar_y + bar_h / 2, true, mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - 25, bar_y + bar_h / 2, false, mouse)) state.mode = mode_next(state.mode);
}

// ── Botones de zoom ───────────────────────────────────────────────────────────
static void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int bx = 14, by = g_split_y - 86, bw = 36, bh = 36;
    Rectangle rp = { (float)bx,(float)by,(float)bw,(float)bh };
    Rectangle rm = { (float)bx,(float)(by + 42),(float)bw,(float)bh };
    bool hp = CheckCollisionPointRec(mouse, rp);
    bool hm = CheckCollisionPointRec(mouse, rm);

    auto draw_btn = [&](Rectangle r, bool hov) {
        if (g_skin.button.valid()) g_skin.button.draw(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        else { DrawRectangleRec(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg)); DrawRectangleLinesEx(r, 1, th.ctrl_border); }
        };
    draw_btn(rp, hp); draw_btn(rm, hm);
    // FONT CHANGE: 18 → 27, 11 → 17
    DrawTextF("+", bx + 9, by + 7, 27, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", bx + 11, by + 49, 27, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);
    }
    char zbuf[16]; snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureTextF(zbuf, 17);
    // FONT CHANGE: 11 → 17
    DrawTextF(zbuf, bx + bw / 2 - tw / 2, by + bh * 2 + 10, 17, th_alpha(th.ctrl_text_dim));
}

// ── Dibujo de burbuja (círculo + sprite) ──────────────────────────────────────
static void draw_bubble(AppState& state, float cx, float cy, float radius,
    Color col, const std::string& tex_key)
{
    if (g_skin.bubble.valid())
        np_draw_bubble(cx, cy, radius, col);
    else
        DrawCircleV({ cx,cy }, radius, col);

    if (!tex_key.empty()) {
        Texture2D tex = state.textures.get(tex_key);
        if (tex.id > 0) {
            float size = radius * 1.4f;
            float scale = size / (float)std::max(tex.width, tex.height);
            float dw = tex.width * scale, dh = tex.height * scale;
            DrawTexturePro(tex,
                { 0,0,(float)tex.width,(float)tex.height },
                { cx - dw * 0.5f, cy - dh * 0.5f, dw, dh },
                { 0,0 }, 0.0f, { 255,255,255,200 });
        }
    }
}

// ── Colores del arco según modo ───────────────────────────────────────────────
static Color arc_color(ViewMode mode, float progress) {
    const Theme& th = g_theme;
    if (progress >= 0.99f) return th.success;
    if (progress >= 0.5f)  return th.warning;
    return Color{ th.success.r, th.success.g, th.success.b, 160 };
}

// ═════════════════════════════════════════════════════════════════════════════
// ── Layout sin superposición ──────────────────────────────────────────────────
// ═════════════════════════════════════════════════════════════════════════════
//
// Estrategia:
//   1. Distribuir los N hijos en anillos concéntricos.
//      El primer anillo cabe K burbujas; si N > K se abren anillos adicionales.
//   2. Colocar cada burbuja en su ángulo ideal del anillo.
//   3. Ejecutar un paso de separación de colisiones (spring-relaxation)
//      para empujar burbujas que se solapan.
//
// Las posiciones se almacenan en un vector auxiliar por frame; NO se mutan
// los MathNode (sus campos x, y, radius siguen siendo los del JSON).
// ─────────────────────────────────────────────────────────────────────────────

struct BubblePos {
    float x, y, r;         // posición y radio calculados para este frame
    int   node_idx;        // índice en cur->children
};

// Calcula los radios de dibujo para todos los hijos y los distribuye en anillos.
// center_r: radio de la burbuja central (zona exclusiva).
// Devuelve el vector de posiciones listo para dibujar.
static std::vector<BubblePos> compute_layout(
    const MathNode* cur,
    const std::vector<float>& draw_radii,   // radio de dibujo por hijo
    float center_r)
{
    const int N = (int)cur->children.size();
    std::vector<BubblePos> pos(N);

    if (N == 0) return pos;

    // ── Paso 1: distribuir en anillos ─────────────────────────────────────────
    // Queremos que las burbujas del anillo toquen el borde exterior del nodo
    // central sin superponerse entre sí.  Calculamos cuántas caben en cada anillo
    // dado el radio del anillo y los radios de las burbujas.

    // Ordenamos los hijos de mayor a menor radio para llenar los anillos internos
    // con los nodos más grandes (más visibles cerca del centro).
    std::vector<int> order(N);
    for (int i = 0; i < N; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return draw_radii[a] > draw_radii[b];
        });

    // Parámetros de anillo
    // gap: espacio mínimo entre burbujas del mismo anillo
    const float gap = 8.0f;

    // Asignación de hijos a anillos
    struct Ring {
        float orbit_r;                // radio del centro del anillo
        std::vector<int> members;     // índices en `order`
    };
    std::vector<Ring> rings;

    // Radio del primer anillo: centro_r + mayor radio hijo + margen
    float max_child_r = *std::max_element(draw_radii.begin(), draw_radii.end());
    float orbit = center_r + max_child_r + gap * 2.0f;

    int placed = 0;
    while (placed < N) {
        Ring ring;
        ring.orbit_r = orbit;

        // Cuántos hijos caben en este anillo:
        // La circunferencia del anillo es 2π·orbit.
        // Cada hijo ocupa un arco de (2·r_i + gap).
        // Llenamos hasta que no quepan más o hayamos colocado todos.
        float used_arc = 0.0f;
        float circ = 2.0f * PI * orbit;

        for (int i = placed; i < N; i++) {
            int idx = order[i];
            float needed = 2.0f * draw_radii[idx] + gap;
            if (used_arc + needed > circ + 0.01f && !ring.members.empty()) break;
            used_arc += needed;
            ring.members.push_back(idx);
        }

        placed += (int)ring.members.size();
        rings.push_back(ring);

        // El siguiente anillo empieza más afuera
        float ring_max_r = 0.0f;
        for (int idx : ring.members) ring_max_r = std::max(ring_max_r, draw_radii[idx]);
        orbit += ring_max_r * 2.0f + gap * 3.0f;
    }

    // ── Paso 2: ángulos equidistantes por anillo ──────────────────────────────
    for (auto& ring : rings) {
        int M = (int)ring.members.size();
        float angle_step = (M > 1) ? (2.0f * PI / M) : 0.0f;

        // Offset de inicio: si solo hay un hijo lo ponemos arriba; si hay varios,
        // comenzamos en -PI/2 (arriba) para que el primer nodo quede en la cima.
        float start_angle = -PI * 0.5f;

        for (int k = 0; k < M; k++) {
            int idx = ring.members[k];
            float angle = start_angle + k * angle_step;
            pos[idx].x = ring.orbit_r * cosf(angle);
            pos[idx].y = ring.orbit_r * sinf(angle);
            pos[idx].r = draw_radii[idx];
            pos[idx].node_idx = idx;
        }
    }

    // ── Paso 3: separación de colisiones (spring relaxation) ─────────────────
    // Iteramos varias veces empujando pares solapados.
    // Solo movemos en dirección tangencial (no radial) para mantener la
    // estructura de anillos.  En la práctica con 30-40 nodos converge en ~15
    // iteraciones.
    const int ITERS = 20;
    const float PUSH_STRENGTH = 0.6f;

    for (int iter = 0; iter < ITERS; iter++) {
        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                float dx = pos[j].x - pos[i].x;
                float dy = pos[j].y - pos[i].y;
                float dist2 = dx * dx + dy * dy;
                float min_dist = pos[i].r + pos[j].r + gap;
                float min_dist2 = min_dist * min_dist;

                if (dist2 < min_dist2 && dist2 > 0.0001f) {
                    float dist = sqrtf(dist2);
                    float overlap = min_dist - dist;
                    float nx = dx / dist, ny = dy / dist;

                    // Separar simétricamente
                    float push = overlap * PUSH_STRENGTH * 0.5f;
                    pos[i].x -= nx * push;
                    pos[i].y -= ny * push;
                    pos[j].x += nx * push;
                    pos[j].y += ny * push;
                }
            }
        }

        // Re-proyectar al radio de su anillo para no alejarse demasiado del centro
        // (evita que el relaxation desplace nodos a órbitas equivocadas)
        for (auto& ring : rings) {
            for (int idx : ring.members) {
                float len = sqrtf(pos[idx].x * pos[idx].x + pos[idx].y * pos[idx].y);
                if (len > 0.001f) {
                    // Suave pull hacia la órbita original (no lo forzamos del todo
                    // para que el relaxation tenga margen de maniobra)
                    float target_len = ring.orbit_r;
                    float correction = (target_len - len) * 0.15f;
                    pos[idx].x += (pos[idx].x / len) * correction;
                    pos[idx].y += (pos[idx].y / len) * correction;
                }
            }
        }
    }

    return pos;
}

// ── draw_bubble_view ──────────────────────────────────────────────────────────
void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    MathNode* cur = state.current();
    bool in_canvas = mouse.x < CANVAS_W() && mouse.y >= UI_TOP() && mouse.y < g_split_y;
    bool any_panel = state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open || state.toolbar.config_open;
    bool canvas_blocked = overlay::blocks_mouse(mouse) || any_panel;

    // ── Actualizar caché de stats ─────────────────────────────────────────────
    const MathNode* stats_root = nullptr;
    if (state.mode == ViewMode::MSC2020 && state.msc_root)      stats_root = state.msc_root.get();
    if (state.mode == ViewMode::Mathlib && state.mathlib_root)   stats_root = state.mathlib_root.get();
    if (state.mode == ViewMode::Standard && state.standard_root) stats_root = state.standard_root.get();
    bubble_stats_ensure(stats_root, state.crossref_map, state.mode);

    // ── Calcular radios de dibujo (proporcionales al peso) ────────────────────
    int max_weight = 1;
    if (cur) {
        for (auto& child : cur->children) {
            const BubbleStats& cs = bubble_stats_get(child->code);
            max_weight = std::max(max_weight, cs.weight);
        }
    }

    // Radio fijo de la burbuja central
    const float CENTER_R = 178.0f;
    // Rango de radios hijo: min y max en píxeles de mundo
    const float CHILD_R_MIN = 38.0f;
    const float CHILD_R_MAX = 80.0f;

    std::vector<float> draw_radii;
    if (cur) {
        draw_radii.reserve(cur->children.size());
        for (auto& child : cur->children) {
            const BubbleStats& cs = bubble_stats_get(child->code);
            draw_radii.push_back(
                proportional_radius(cs.weight, max_weight, CHILD_R_MIN, CHILD_R_MAX));
        }
    }

    // ── Calcular layout sin superposición ─────────────────────────────────────
    std::vector<BubblePos> layout;
    if (cur && !cur->children.empty()) {
        layout = compute_layout(cur, draw_radii, CENTER_R);
    }

    // ── Input: hover y pan/zoom ───────────────────────────────────────────────
    bool over_bubble = false;
    if (cur && in_canvas && !canvas_blocked) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x * wm.x + wm.y * wm.y) < CENTER_R) over_bubble = true;
        if (!over_bubble) {
            for (int i = 0; i < (int)layout.size(); i++) {
                float dx = wm.x - layout[i].x, dy = wm.y - layout[i].y;
                if (dx * dx + dy * dy < layout[i].r * layout[i].r) { over_bubble = true; break; }
            }
        }
    }

    if (in_canvas && !canvas_blocked) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble)
            || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) { Vector2 d = GetMouseDelta(); cam.target.x -= d.x / cam.zoom; cam.target.y -= d.y / cam.zoom; }
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

    // Líneas de conexión (usando posiciones del layout, no del JSON)
    if (cur && !cur->children.empty()) {
        for (int i = 0; i < (int)layout.size(); i++) {
            const auto& child = cur->children[i];
            float lx = layout[i].x, ly = layout[i].y;
            float len = sqrtf(lx * lx + ly * ly);
            if (len < 0.001f) continue;
            float nx = lx / len, ny = ly / len;
            const BubbleStats& cs = bubble_stats_get(child->code);
            float alpha = cs.connected ? th.bubble_line_alpha : th.bubble_line_alpha * 0.4f;
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
    draw_bubble(state, 0.0f, 0.0f, CENTER_R, center_col, cur ? cur->texture_key : "");

    if (cur && cur->code != "ROOT" && !cur->children.empty()) {
        const BubbleStats& cs = bubble_stats_get(cur->code);
        Color ac = arc_color(state.mode, cs.progress);
        draw_progress_arc(0.0f, 0.0f, CENTER_R, cs.progress, 5.0f, ac);
    }

    // Label central
    // FONT CHANGE: 20 → 30 (≈1.5×)
    const char* clabel = (!cur || cur->code == "ROOT") ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureTextF(cl_str.c_str(), 30);
    int ly = (cur && !cur->texture_key.empty()) ? 140 : -15;
    DrawTextF(cl_str.c_str(), -clw / 2, ly, 30, th.bubble_label_center);

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);

        for (int i = 0; i < (int)layout.size(); i++) {
            const auto& child = cur->children[i];
            const BubbleStats& cs = bubble_stats_get(child->code);
            float draw_r = layout[i].r;
            float bx = layout[i].x, by_pos = layout[i].y;

            Color col = bubble_color(child->color, cs);

            float ddx = wm.x - bx, ddy = wm.y - by_pos;
            bool hov = (ddx * ddx + ddy * ddy) < (draw_r * draw_r) && in_canvas && !canvas_blocked;

            // Glow
            DrawCircleV({ bx, by_pos }, draw_r + 3.0f,
                th_fade(child->color, th.bubble_glow_alpha * (cs.connected ? 1.0f : 0.3f)));

            // Burbuja
            draw_bubble(state, bx, by_pos, draw_r, col, child->texture_key);

            // Arco de progreso
            if (!child->children.empty() && cs.progress > 0.001f) {
                Color ac = arc_color(state.mode, cs.progress);
                draw_progress_arc(bx, by_pos, draw_r, cs.progress, 3.5f, ac);
            }

            // Hover ring + click
            if (hov) {
                DrawCircleLinesV({ bx, by_pos }, draw_r + 6.0f, th.bubble_hover_ring);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.selected_code = child->code;
                    state.selected_label = child->label;
                    if (!child->children.empty()) state.push(child);
                }
            }

            // Label de hijo
            // FONT CHANGE: 10 → 15 (1.5×)
            std::string lbl = short_label(child->label, draw_r);
            int tw = MeasureTextF(lbl.c_str(), 15);
            int loff = (!child->texture_key.empty()) ? (int)(draw_r * 0.55f) : -7;
            DrawTextF(lbl.c_str(), (int)(bx - tw * 0.5f), (int)(by_pos + loff),
                15, th.bubble_label_child);

            // Indicador "desconectado"
            if (!cs.connected && !child->children.empty()) {
                // FONT CHANGE: 9 → 14
                int iw = MeasureTextF("?", 14);
                DrawTextF("?", (int)(bx - iw * 0.5f), (int)(by_pos - draw_r - 18),
                    14, th_alpha(th.text_dim));
            }
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        // FONT CHANGE: 14 → 21
        int tw = MeasureTextF(msg, 21);
        DrawTextF(msg, -tw / 2, 210, 21, th.bubble_empty_msg);
    }

    EndMode2D();
    EndScissorMode();

    draw_zoom_buttons(cam, mouse);

    // Botón "< Atrás"
    if (state.can_go_back()) {
        float bx = 14, by2 = (float)(UI_TOP() + 56), bw = 100, bh = 32;
        bool bh3 = CheckCollisionPointRec(mouse, { bx,by2,bw,bh }) && !canvas_blocked;
        if (g_skin.button.valid())
            g_skin.button.draw(bx, by2, bw, bh, bh3 ? th.bg_button_hover : th_alpha(th.bg_button));
        else {
            DrawRectangleRec({ bx,by2,bw,bh }, bh3 ? th.bg_button_hover : th_alpha(th.bg_button));
            DrawRectangleLinesEx({ bx,by2,bw,bh }, 1, th.ctrl_border);
        }
        // FONT CHANGE: 13 → 19
        DrawTextF("< Atras", (int)bx + 8, (int)by2 + 7, 19,
            bh3 ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        if (bh3 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.pop();
    }
}