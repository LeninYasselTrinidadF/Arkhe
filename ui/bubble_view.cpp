#include "bubble_view.hpp"
#include "overlay.hpp"
#include "font_manager.hpp"
#include "bubble_stats.hpp"
#include "theme.hpp"
#include "nine_patch.hpp"
#include "skin.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <algorithm>
#include <cmath>

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
// Si el nodo no está conectado al crossref_map, oscurece el color base un 40%.
static Color bubble_color(Color base, const BubbleStats& st) {
    if (st.connected) return base;
    // ColorBrightness no existe en todas las versiones de raylib; implementación manual:
    float factor = 0.40f;   // 40% del brillo original
    return Color{
        (unsigned char)(base.r * factor),
        (unsigned char)(base.g * factor),
        (unsigned char)(base.b * factor),
        base.a
    };
}

// ── Arco de progreso ──────────────────────────────────────────────────────────
// Dibuja un arco de grosor `thick` alrededor del círculo de radio `r`
// representando el progreso [0,1]. 0 → sin arco; 1 → círculo completo.
// El arco empieza en la parte superior (-90°) y va en sentido horario.
static void draw_progress_arc(float cx, float cy, float r, float progress,
                               float thick, Color col)
{
    if (progress <= 0.001f) return;
    float angle_deg = progress * 360.0f;
    // DrawRing dibuja un anillo completo, usamos segmentos pequeños para el arco
    // Raylib 4.x tiene DrawRing con ángulos start/end:
    //   DrawRing(center, inner, outer, startAngle, endAngle, segments, color)
    // El ángulo 0 = derecha, -90 = arriba (top), en grados.
    float outer = r + thick + 2.0f;
    float inner = r + 2.0f;
    int segments = std::max(6, (int)(angle_deg / 4.0f));
    DrawRing({cx, cy}, inner, outer, -90.0f, -90.0f + angle_deg, segments, col);
}

// ── Radio proporcional ────────────────────────────────────────────────────────
// Dado el peso (hojas) del nodo y el peso máximo entre hermanos,
// devuelve un radio entre min_r y max_r proporcional al peso.
static float proportional_radius(int weight, int max_weight,
                                  float min_r, float max_r)
{
    if (max_weight <= 1) return (min_r + max_r) * 0.5f;
    float t = std::sqrt((float)weight / (float)max_weight);   // sqrt suaviza extremos
    return min_r + t * (max_r - min_r);
}

// ── Label con fuente custom ───────────────────────────────────────────────────
static std::string short_label(const std::string& label, float radius) {
    int mc = (int)(radius * 1.6f / 7.0f);
    if (mc < 1) mc = 1;
    if ((int)label.size() <= mc) return label;
    return label.substr(0, mc - 1) + ".";
}

// ── draw_arrow (con overlay guard) ───────────────────────────────────────────
bool draw_arrow(int cx, int cy, bool left, Vector2 mouse) {
    if (overlay::blocks_mouse(mouse)) return false;
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx-HW-4),(float)(cy-HH-4),(float)(HW*2+8),(float)(HH*2+8) };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)  DrawTriangle({(float)(cx+HW),(float)(cy-HH)},{(float)(cx+HW),(float)(cy+HH)},{(float)(cx-HW),(float)cy},col);
    else       DrawTriangle({(float)(cx-HW),(float)(cy+HH)},{(float)(cx-HW),(float)(cy-HH)},{(float)(cx+HW),(float)cy},col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── Switcher de modo ──────────────────────────────────────────────────────────
void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int nw = MeasureTextF(mname, 17);
    int bar_w = nw + 130, bar_x = CX() - bar_w/2, bar_y = UI_TOP() + 10, bar_h = 36;

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x,(float)bar_y,(float)bar_w,(float)bar_h,th_alpha(th.ctrl_bg));
    else {
        DrawRectangle(bar_x,bar_y,bar_w,bar_h,th_alpha(th.ctrl_bg));
        DrawRectangleLines(bar_x,bar_y,bar_w,bar_h,th.ctrl_border);
    }
    DrawLine(bar_x+50,bar_y+6,bar_x+50,bar_y+bar_h-6,th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x+bar_w-50,bar_y+6,bar_x+bar_w-50,bar_y+bar_h-6,th_alpha(th.ctrl_text_dim));
    DrawTextF(mname, CX()-nw/2, bar_y+10, 17, th.ctrl_text);

    if (draw_arrow(bar_x+25,       bar_y+bar_h/2, true,  mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x+bar_w-25, bar_y+bar_h/2, false, mouse)) state.mode = mode_next(state.mode);
}

// ── Botones de zoom ───────────────────────────────────────────────────────────
static void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int bx = 14, by = g_split_y - 86, bw = 36, bh = 36;
    Rectangle rp = { (float)bx,(float)by,(float)bw,(float)bh };
    Rectangle rm = { (float)bx,(float)(by+42),(float)bw,(float)bh };
    bool hp = CheckCollisionPointRec(mouse, rp);
    bool hm = CheckCollisionPointRec(mouse, rm);

    auto draw_btn = [&](Rectangle r, bool hov) {
        if (g_skin.button.valid()) g_skin.button.draw(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        else { DrawRectangleRec(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg)); DrawRectangleLinesEx(r,1,th.ctrl_border); }
    };
    draw_btn(rp, hp); draw_btn(rm, hm);
    DrawTextF("+", bx+11, by+8,  18, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawTextF("-", bx+13, by+50, 18, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom+0.15f,0.05f,5.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom-0.15f,0.05f,5.0f);
    }
    char zbuf[16]; snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom*100.0f);
    int tw = MeasureTextF(zbuf, 11);
    DrawTextF(zbuf, bx+bw/2-tw/2, by+bh*2+10, 11, th_alpha(th.ctrl_text_dim));
}

// ── Dibujo de burbuja (círculo + sprite) ──────────────────────────────────────
static void draw_bubble(AppState& state, float cx, float cy, float radius,
                         Color col, const std::string& tex_key)
{
    if (g_skin.bubble.valid())
        np_draw_bubble(cx, cy, radius, col);
    else
        DrawCircleV({cx,cy}, radius, col);

    if (!tex_key.empty()) {
        Texture2D tex = state.textures.get(tex_key);
        if (tex.id > 0) {
            float size  = radius * 1.4f;
            float scale = size / (float)std::max(tex.width, tex.height);
            float dw = tex.width * scale, dh = tex.height * scale;
            DrawTexturePro(tex,
                {0,0,(float)tex.width,(float)tex.height},
                {cx-dw*0.5f, cy-dh*0.5f, dw, dh},
                {0,0}, 0.0f, {255,255,255,200});
        }
    }
}

// ── Colores del arco según modo ───────────────────────────────────────────────
static Color arc_color(ViewMode mode, float progress) {
    const Theme& th = g_theme;
    // Verde para Mathlib/bien conectado; ámbar para parcial; usa colores del tema
    if (progress >= 0.99f) return th.success;
    if (progress >= 0.5f)  return th.warning;
    return Color{ th.success.r, th.success.g, th.success.b, 160 };
}

// ── draw_bubble_view ──────────────────────────────────────────────────────────
void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    MathNode* cur   = state.current();
    bool in_canvas  = mouse.x < CANVAS_W() && mouse.y >= UI_TOP() && mouse.y < g_split_y;
    bool canvas_blocked = overlay::blocks_mouse(mouse);

    // ── Actualizar caché de stats ─────────────────────────────────────────────
    // Pasamos la raíz del árbol activo (no el nodo actual, para cubrir toda la jerarquía).
    const MathNode* stats_root = nullptr;
    if (state.mode == ViewMode::MSC2020  && state.msc_root)      stats_root = state.msc_root.get();
    if (state.mode == ViewMode::Mathlib  && state.mathlib_root)   stats_root = state.mathlib_root.get();
    if (state.mode == ViewMode::Standard && state.standard_root)  stats_root = state.standard_root.get();
    bubble_stats_ensure(stats_root, state.crossref_map, state.mode);

    // ── Calcular peso máximo entre hijos (para radio proporcional) ────────────
    int max_weight = 1;
    if (cur) {
        for (auto& child : cur->children) {
            const BubbleStats& cs = bubble_stats_get(child->code);
            max_weight = std::max(max_weight, cs.weight);
        }
    }

    // ── Input ─────────────────────────────────────────────────────────────────
    bool over_bubble = false;
    if (cur && in_canvas && !canvas_blocked) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x*wm.x + wm.y*wm.y) < 178.0f) over_bubble = true;
        if (!over_bubble)
            for (const auto& c : cur->children) {
                float dx = wm.x - c->x, dy = wm.y - c->y;
                if (dx*dx + dy*dy < c->radius*c->radius) { over_bubble = true; break; }
            }
    }

    if (in_canvas && !canvas_blocked) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble)
                || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) { Vector2 d = GetMouseDelta(); cam.target.x -= d.x/cam.zoom; cam.target.y -= d.y/cam.zoom; }
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && in_canvas && !canvas_blocked) {
        cam.offset = mouse;
        cam.target = GetScreenToWorld2D(mouse, cam);
        cam.zoom   = Clamp(cam.zoom + wheel*0.1f, 0.05f, 5.0f);
    }
    cam.offset = {(float)CX(), (float)CY()};

    // ── Dibujo ────────────────────────────────────────────────────────────────
    BeginScissorMode(0, UI_TOP(), CANVAS_W(), TOP_H());
    BeginMode2D(cam);

    // Líneas de conexión
    if (cur && !cur->children.empty()) {
        for (const auto& child : cur->children) {
            float len = sqrtf(child->x*child->x + child->y*child->y);
            if (len < 0.001f) continue;
            float nx = child->x/len, ny = child->y/len;
            const BubbleStats& cs = bubble_stats_get(child->code);
            // Línea más tenue si no está conectada
            float alpha = cs.connected ? th.bubble_line_alpha : th.bubble_line_alpha * 0.4f;
            DrawLineEx({nx*180.0f, ny*180.0f},
                       {child->x - nx*child->radius, child->y - ny*child->radius},
                       1.0f, th_fade(child->color, alpha));
        }
    }

    // Burbuja central
    DrawCircleLines(0, 0, 179, th.bubble_ring);
    Color center_col = (cur && cur->code != "ROOT")
        ? th_fade(cur->color, th.bubble_center_alpha)
        : th.bubble_center_flat;
    draw_bubble(state, 0.0f, 0.0f, 178.0f, center_col, cur ? cur->texture_key : "");

    // Arco de progreso de la burbuja central (si tiene hijos)
    if (cur && cur->code != "ROOT" && !cur->children.empty()) {
        const BubbleStats& cs = bubble_stats_get(cur->code);
        Color ac = arc_color(state.mode, cs.progress);
        draw_progress_arc(0.0f, 0.0f, 178.0f, cs.progress, 5.0f, ac);
    }

    const char* clabel = (!cur || cur->code == "ROOT") ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureTextF(cl_str.c_str(), 20);
    int ly  = (cur && !cur->texture_key.empty()) ? 140 : -10;
    DrawTextF(cl_str.c_str(), -clw/2, ly, 20, th.bubble_label_center);

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);

        for (const auto& child : cur->children) {
            const BubbleStats& cs = bubble_stats_get(child->code);

            // Radio proporcional al peso dentro del grupo de hermanos
            // (respeta el radio original del JSON como referencia base)
            float base_r   = child->radius;
            float prop_r   = proportional_radius(cs.weight, max_weight,
                                                  base_r * 0.55f, base_r * 1.20f);
            // Guardamos en child->radius para que el layout y los clicks usen el mismo
            // valor — NOTA: hacemos cast temporal, no modificamos el JSON.
            // Si prefieres no mutar el nodo, usa prop_r en draw y base_r en hit-test.
            float draw_r = prop_r;

            // Color con oscuridad
            Color col = bubble_color(child->color, cs);

            float dx = wm.x - child->x, dy = wm.y - child->y;
            bool hov = (dx*dx + dy*dy) < (draw_r * draw_r) && in_canvas && !canvas_blocked;

            // Glow
            DrawCircleV({child->x, child->y}, draw_r + 2,
                        th_fade(child->color, th.bubble_glow_alpha * (cs.connected ? 1.0f : 0.3f)));

            // Burbuja
            draw_bubble(state, child->x, child->y, draw_r, col, child->texture_key);

            // Arco de progreso (solo si tiene hijos, es decir no es hoja)
            if (!child->children.empty() && cs.progress > 0.001f) {
                Color ac = arc_color(state.mode, cs.progress);
                draw_progress_arc(child->x, child->y, draw_r, cs.progress, 3.5f, ac);
            }

            // Hover ring + click
            if (hov) {
                DrawCircleLinesV({child->x, child->y}, draw_r + 5, th.bubble_hover_ring);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.selected_code  = child->code;
                    state.selected_label = child->label;
                    if (!child->children.empty()) state.push(child);
                }
            }

            // Label (usa fuente custom escalada)
            std::string lbl = short_label(child->label, draw_r);
            int tw   = MeasureTextF(lbl.c_str(), 10);
            int loff = (!child->texture_key.empty()) ? (int)(draw_r * 0.55f) : -5;
            DrawTextF(lbl.c_str(), (int)(child->x - tw*0.5f), (int)(child->y + loff),
                      10, th.bubble_label_child);

            // Indicador "desconectado" si no tiene crossref y no es hoja
            if (!cs.connected && !child->children.empty()) {
                int iw = MeasureTextF("?", 9);
                DrawTextF("?", (int)(child->x - iw*0.5f), (int)(child->y - draw_r - 14),
                          9, th_alpha(th.text_dim));
            }
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        int tw = MeasureTextF(msg, 14);
        DrawTextF(msg, -tw/2, 210, 14, th.bubble_empty_msg);
    }

    EndMode2D();
    EndScissorMode();

    draw_zoom_buttons(cam, mouse);

    // Botón "< Atrás"
    if (state.can_go_back()) {
        float bx = 14, by = (float)(UI_TOP()+56), bw = 88, bh = 28;
        bool bh2 = CheckCollisionPointRec(mouse, {bx,by,bw,bh}) && !canvas_blocked;
        if (g_skin.button.valid())
            g_skin.button.draw(bx,by,bw,bh, bh2 ? th.bg_button_hover : th_alpha(th.bg_button));
        else {
            DrawRectangleRec({bx,by,bw,bh}, bh2 ? th.bg_button_hover : th_alpha(th.bg_button));
            DrawRectangleLinesEx({bx,by,bw,bh}, 1, th.ctrl_border);
        }
        DrawTextF("< Atras", (int)bx+8, (int)by+8, 13,
                  bh2 ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        if (bh2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.pop();
    }
}
