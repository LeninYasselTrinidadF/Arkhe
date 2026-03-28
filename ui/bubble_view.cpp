#include "bubble_view.hpp"
#include "overlay.hpp"   // ← guarda de clicks
#include "theme.hpp"
#include "nine_patch.hpp"
#include "skin.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>

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

bool draw_arrow(int cx, int cy, bool left, Vector2 mouse) {
    // Las flechas del switcher están en el canvas → también se bloquean
    if (overlay::blocks_mouse(mouse)) return false;
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx - HW - 4),(float)(cy - HH - 4),(float)(HW * 2 + 8),(float)(HH * 2 + 8) };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color col = hov ? g_theme.ctrl_text : th_alpha(g_theme.ctrl_text_dim);
    if (left)  DrawTriangle({ (float)(cx + HW),(float)(cy - HH) }, { (float)(cx + HW),(float)(cy + HH) }, { (float)(cx - HW),(float)cy }, col);
    else       DrawTriangle({ (float)(cx - HW),(float)(cy + HH) }, { (float)(cx - HW),(float)(cy - HH) }, { (float)(cx + HW),(float)cy }, col);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int nw = MeasureText(mname, 17);
    int bar_w = nw + 130, bar_x = CX() - bar_w / 2, bar_y = UI_TOP() + 10, bar_h = 36;

    if (g_skin.button.valid())
        g_skin.button.draw((float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h, th_alpha(th.ctrl_bg));
    else { DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg)); DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border); }

    DrawLine(bar_x + 50, bar_y + 6, bar_x + 50, bar_y + bar_h - 6, th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - 50, bar_y + 6, bar_x + bar_w - 50, bar_y + bar_h - 6, th_alpha(th.ctrl_text_dim));
    DrawText(mname, CX() - nw / 2, bar_y + 10, 17, th.ctrl_text);

    if (draw_arrow(bar_x + 25, bar_y + bar_h / 2, true, mouse)) state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - 25, bar_y + bar_h / 2, false, mouse)) state.mode = mode_next(state.mode);
}

static void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int bx = 14, by = g_split_y - 86, bw = 36, bh = 36;
    Rectangle rp = { (float)bx,(float)by,(float)bw,(float)bh };
    Rectangle rm = { (float)bx,(float)(by + 42),(float)bw,(float)bh };
    bool hp = CheckCollisionPointRec(mouse, rp), hm = CheckCollisionPointRec(mouse, rm);

    auto draw_btn = [&](Rectangle r, bool hov) {
        if (g_skin.button.valid()) g_skin.button.draw(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
        else { DrawRectangleRec(r, hov ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg)); DrawRectangleLinesEx(r, 1, th.ctrl_border); }
        };
    draw_btn(rp, hp); draw_btn(rm, hm);
    DrawText("+", bx + 11, by + 8, 18, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    DrawText("-", bx + 13, by + 50, 18, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    // Zoom buttons: solo reaccionan si no hay overlay encima
    if (!overlay::blocks_mouse(mouse)) {
        if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
        if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);
    }
    char zbuf[16]; snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureText(zbuf, 11);
    DrawText(zbuf, bx + bw / 2 - tw / 2, by + bh * 2 + 10, 11, th_alpha(th.ctrl_text_dim));
}

static std::string short_label(const std::string& label, float radius) {
    int mc = (int)(radius * 1.6f / 7.0f); if (mc < 1)mc = 1;
    if ((int)label.size() <= mc) return label;
    return label.substr(0, mc - 1) + ".";
}

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
            float size = radius * 1.4f, scale = size / (float)std::max(tex.width, tex.height);
            float dw = tex.width * scale, dh = tex.height * scale;
            DrawTexturePro(tex, { 0,0,(float)tex.width,(float)tex.height },
                { cx - dw * 0.5f,cy - dh * 0.5f,dw,dh }, { 0,0 }, 0.0f, { 255,255,255,200 });
        }
    }
}

void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    MathNode* cur = state.current();
    bool in_canvas = mouse.x < CANVAS_W() && mouse.y >= UI_TOP() && mouse.y < g_split_y;

    // Si un panel flotante está encima, el canvas no procesa ningún input
    bool canvas_blocked = overlay::blocks_mouse(mouse);

    bool over_bubble = false;
    if (cur && in_canvas && !canvas_blocked) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x * wm.x + wm.y * wm.y) < 178.0f) over_bubble = true;
        if (!over_bubble)
            for (const auto& c : cur->children) {
                float dx = wm.x - c->x, dy = wm.y - c->y;
                if (dx * dx + dy * dy < c->radius * c->radius) { over_bubble = true; break; }
            }
    }

    if (in_canvas && !canvas_blocked) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) { Vector2 d = GetMouseDelta(); cam.target.x -= d.x / cam.zoom; cam.target.y -= d.y / cam.zoom; }
    }
    // Zoom con rueda: solo si está en el canvas Y no hay overlay
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && in_canvas && !canvas_blocked) {
        cam.offset = mouse;
        cam.target = GetScreenToWorld2D(mouse, cam);
        cam.zoom = Clamp(cam.zoom + wheel * 0.1f, 0.05f, 5.0f);
    }
    cam.offset = { (float)CX(),(float)CY() };

    BeginScissorMode(0, UI_TOP(), CANVAS_W(), TOP_H());
    BeginMode2D(cam);

    // Líneas de conexión
    if (cur && !cur->children.empty())
        for (const auto& child : cur->children) {
            float len = sqrtf(child->x * child->x + child->y * child->y);
            if (len < 0.001f) continue;
            float nx = child->x / len, ny = child->y / len;
            DrawLineEx({ nx * 180.0f,ny * 180.0f },
                { child->x - nx * child->radius,child->y - ny * child->radius },
                1.0f, th_fade(child->color, th.bubble_line_alpha));
        }

    // Burbuja central
    DrawCircleLines(0, 0, 179, th.bubble_ring);
    Color center_col = (cur && cur->code != "ROOT")
        ? th_fade(cur->color, th.bubble_center_alpha) : th.bubble_center_flat;
    draw_bubble(state, 0.0f, 0.0f, 178.0f, center_col, cur ? cur->texture_key : "");

    const char* clabel = (!cur || cur->code == "ROOT") ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureText(cl_str.c_str(), 20);
    int ly = (cur && !cur->texture_key.empty()) ? 140 : -10;
    DrawText(cl_str.c_str(), -clw / 2, ly, 20, th.bubble_label_center);

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        for (const auto& child : cur->children) {
            float dx = wm.x - child->x, dy = wm.y - child->y;
            // hover visual solo si el canvas no está bloqueado
            bool hov = (dx * dx + dy * dy) < (child->radius * child->radius)
                && in_canvas
                && !canvas_blocked;
            DrawCircleV({ child->x,child->y }, child->radius + 2, th_fade(child->color, th.bubble_glow_alpha));
            draw_bubble(state, child->x, child->y, child->radius, child->color, child->texture_key);
            if (hov) {
                DrawCircleLinesV({ child->x,child->y }, child->radius + 5, th.bubble_hover_ring);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.selected_code = child->code; state.selected_label = child->label;
                    if (!child->children.empty()) state.push(child);
                }
            }
            std::string lbl = short_label(child->label, child->radius);
            int tw = MeasureText(lbl.c_str(), 10);
            int loff = (!child->texture_key.empty()) ? (int)(child->radius * 0.55f) : -5;
            DrawText(lbl.c_str(), (int)(child->x - tw * 0.5f), (int)(child->y + loff), 10, th.bubble_label_child);
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        int tw = MeasureText(msg, 14); DrawText(msg, -tw / 2, 210, 14, th.bubble_empty_msg);
    }

    EndMode2D(); EndScissorMode();
    draw_zoom_buttons(cam, mouse);

    // Botón "< Atras"
    if (state.can_go_back()) {
        float bx = 14, by = (float)(UI_TOP() + 56), bw = 88, bh = 28;
        bool bh2 = CheckCollisionPointRec(mouse, { bx,by,bw,bh }) && !canvas_blocked;
        if (g_skin.button.valid()) g_skin.button.draw(bx, by, bw, bh, bh2 ? th.bg_button_hover : th_alpha(th.bg_button));
        else { DrawRectangleRec({ bx,by,bw,bh }, bh2 ? th.bg_button_hover : th_alpha(th.bg_button)); DrawRectangleLinesEx({ bx,by,bw,bh }, 1, th.ctrl_border); }
        DrawText("< Atras", (int)bx + 8, (int)by + 8, 13, bh2 ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        if (bh2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.pop();
    }
}