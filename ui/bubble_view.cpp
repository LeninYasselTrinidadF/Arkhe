#include "bubble_view.hpp"
#include "theme.hpp"
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
    constexpr int HW = 12, HH = 10;
    Rectangle r = { (float)(cx - HW - 4),(float)(cy - HH - 4),(float)(HW * 2 + 8),(float)(HH * 2 + 8) };
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

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* mname = mode_name(state.mode);
    int nw = MeasureText(mname, 17);
    int bar_w = nw + 130;
    int bar_x = CX() - bar_w / 2;
    int bar_y = UI_TOP() + 10;
    int bar_h = 36;

    DrawRectangle(bar_x, bar_y, bar_w, bar_h, th_alpha(th.ctrl_bg));
    DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, th.ctrl_border);
    DrawLine(bar_x + 50, bar_y + 6, bar_x + 50, bar_y + bar_h - 6,
        th_alpha(th.ctrl_text_dim));
    DrawLine(bar_x + bar_w - 50, bar_y + 6, bar_x + bar_w - 50, bar_y + bar_h - 6,
        th_alpha(th.ctrl_text_dim));
    DrawText(mname, CX() - nw / 2, bar_y + 10, 17, th.ctrl_text);

    if (draw_arrow(bar_x + 25, bar_y + bar_h / 2, true, mouse))
        state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x + bar_w - 25, bar_y + bar_h / 2, false, mouse))
        state.mode = mode_next(state.mode);
}

static void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    const int bx = 14;
    const int by = g_split_y - 86;
    const int bw = 36, bh = 36;

    Rectangle r_plus = { (float)bx, (float)by,       (float)bw, (float)bh };
    Rectangle r_minus = { (float)bx, (float)(by + 42), (float)bw, (float)bh };

    bool hp = CheckCollisionPointRec(mouse, r_plus);
    bool hm = CheckCollisionPointRec(mouse, r_minus);

    DrawRectangleRec(r_plus, hp ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
    DrawRectangleLinesEx(r_plus, 1, th.ctrl_border);
    DrawText("+", bx + 11, by + 8, 18, hp ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    DrawRectangleRec(r_minus, hm ? th.ctrl_bg_hover : th_alpha(th.ctrl_bg));
    DrawRectangleLinesEx(r_minus, 1, th.ctrl_border);
    DrawText("-", bx + 13, by + 50, 18, hm ? th.ctrl_text : th_alpha(th.ctrl_text_dim));

    if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
    if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);

    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureText(zbuf, 11);
    DrawText(zbuf, bx + bw / 2 - tw / 2, by + bh * 2 + 10, 11,
        th_alpha(th.ctrl_text_dim));
}

static std::string short_label(const std::string& label, float radius) {
    int max_chars = (int)(radius * 1.6f / 7.0f);
    if (max_chars < 1) max_chars = 1;
    if ((int)label.size() <= max_chars) return label;
    return label.substr(0, max_chars - 1) + ".";
}

static void draw_bubble_sprite(AppState& state, const std::string& key,
    float cx, float cy, float radius)
{
    if (key.empty()) return;
    Texture2D tex = state.textures.get(key);
    if (tex.id == 0) return;
    float size = radius * 1.4f;
    float scale = size / (float)std::max(tex.width, tex.height);
    float dw = tex.width * scale;
    float dh = tex.height * scale;
    DrawTexturePro(tex,
        { 0, 0, (float)tex.width, (float)tex.height },
        { cx - dw * 0.5f, cy - dh * 0.5f, dw, dh },
        { 0, 0 }, 0.0f, { 255, 255, 255, 200 });
}

void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    MathNode* cur = state.current();

    bool in_canvas = mouse.x < CANVAS_W()
        && mouse.y >= UI_TOP()
        && mouse.y < g_split_y;

    bool over_bubble = false;
    if (cur && in_canvas) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x * wm.x + wm.y * wm.y) < 178.0f) over_bubble = true;
        if (!over_bubble) {
            for (const auto& child : cur->children) {
                float dx = wm.x - child->x, dy = wm.y - child->y;
                if (dx * dx + dy * dy < child->radius * child->radius)
                {
                    over_bubble = true; break;
                }
            }
        }
    }

    if (in_canvas) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble)
            || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) {
            Vector2 d = GetMouseDelta();
            cam.target.x -= d.x / cam.zoom;
            cam.target.y -= d.y / cam.zoom;
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && in_canvas) {
        cam.offset = mouse;
        cam.target = GetScreenToWorld2D(mouse, cam);
        cam.zoom = Clamp(cam.zoom + wheel * 0.1f, 0.05f, 5.0f);
    }

    cam.offset = { (float)CX(), (float)CY() };

    BeginScissorMode(0, UI_TOP(), CANVAS_W(), TOP_H());
    BeginMode2D(cam);

    // Líneas de conexión
    if (cur && !cur->children.empty()) {
        for (const auto& child : cur->children) {
            float len = sqrtf(child->x * child->x + child->y * child->y);
            if (len < 0.001f) continue;
            float nx = child->x / len, ny = child->y / len;
            DrawLineEx({ nx * 180.0f, ny * 180.0f },
                { child->x - nx * child->radius, child->y - ny * child->radius },
                1.0f, th_fade(child->color, th.bubble_line_alpha));
        }
    }

    // Burbuja central
    Color center_col = (cur && cur->code != "ROOT")
        ? th_fade(cur->color, th.bubble_center_alpha)
        : th.bubble_center_flat;
    DrawCircle(0, 0, 178, center_col);
    DrawCircleLines(0, 0, 178, th.bubble_ring);

    if (cur && !cur->texture_key.empty())
        draw_bubble_sprite(state, cur->texture_key, 0.0f, 0.0f, 170.0f);

    const char* clabel = (!cur || cur->code == "ROOT")
        ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureText(cl_str.c_str(), 20);
    int label_y = (cur && !cur->texture_key.empty()) ? 140 : -10;
    DrawText(cl_str.c_str(), -clw / 2, label_y, 20, th.bubble_label_center);

    // Burbujas hijas
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        for (const auto& child : cur->children) {
            float dx = wm.x - child->x, dy = wm.y - child->y;
            bool hovered = (dx * dx + dy * dy) < (child->radius * child->radius)
                && in_canvas;

            DrawCircleV({ child->x, child->y }, child->radius + 2,
                th_fade(child->color, th.bubble_glow_alpha));
            DrawCircleV({ child->x, child->y }, child->radius, child->color);

            if (!child->texture_key.empty())
                draw_bubble_sprite(state, child->texture_key,
                    child->x, child->y, child->radius);

            if (hovered) {
                DrawCircleLinesV({ child->x, child->y }, child->radius + 5,
                    th.bubble_hover_ring);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.selected_code = child->code;
                    state.selected_label = child->label;
                    if (!child->children.empty()) state.push(child);
                }
            }

            std::string lbl = short_label(child->label, child->radius);
            int tw = MeasureText(lbl.c_str(), 10);
            int loff = (!child->texture_key.empty()) ? (int)(child->radius * 0.55f) : -5;
            DrawText(lbl.c_str(),
                (int)(child->x - tw * 0.5f),
                (int)(child->y + loff), 10, th.bubble_label_child);
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        int tw = MeasureText(msg, 14);
        DrawText(msg, -tw / 2, 210, 14, th.bubble_empty_msg);
    }

    EndMode2D();
    EndScissorMode();

    draw_zoom_buttons(cam, mouse);

    // Botón Atrás
    if (state.can_go_back()) {
        Rectangle br = { 14.0f, (float)(UI_TOP() + 56), 88.0f, 28.0f };
        bool bh = CheckCollisionPointRec(mouse, br);
        DrawRectangleRec(br, bh ? th.bg_button_hover : th_alpha(th.bg_button));
        DrawRectangleLinesEx(br, 1, th.ctrl_border);
        DrawText("< Atras", 22, UI_TOP() + 63, 13,
            bh ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        if (bh && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.pop();
    }
}