#include "bubble_view.hpp"
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
    Rectangle r = {(float)(cx-HW-4),(float)(cy-HH-4),(float)(HW*2+8),(float)(HH*2+8)};
    bool hovered = CheckCollisionPointRec(mouse, r);
    Color col = hovered ? WHITE : Color{180,180,210,200};
    if (left)
        DrawTriangle({(float)(cx+HW),(float)(cy-HH)},
                     {(float)(cx+HW),(float)(cy+HH)},
                     {(float)(cx-HW),(float)cy}, col);
    else
        DrawTriangle({(float)(cx-HW),(float)(cy+HH)},
                     {(float)(cx-HW),(float)(cy-HH)},
                     {(float)(cx+HW),(float)cy}, col);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void draw_mode_switcher(AppState& state, Vector2 mouse) {
    const char* mname = mode_name(state.mode);
    int nw    = MeasureText(mname, 17);
    int bar_w = nw + 130;
    int bar_x = CX() - bar_w / 2;
    int bar_y = 10;
    int bar_h = 36;

    DrawRectangle(bar_x, bar_y, bar_w, bar_h, {20,20,35,230});
    DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, {80,80,130,255});
    DrawLine(bar_x+50, bar_y+6, bar_x+50, bar_y+bar_h-6, {70,70,110,200});
    DrawLine(bar_x+bar_w-50, bar_y+6, bar_x+bar_w-50, bar_y+bar_h-6, {70,70,110,200});
    DrawText(mname, CX() - nw/2, bar_y+10, 17, WHITE);

    if (draw_arrow(bar_x+25, bar_y+bar_h/2, true, mouse))
        state.mode = mode_prev(state.mode);
    if (draw_arrow(bar_x+bar_w-25, bar_y+bar_h/2, false, mouse))
        state.mode = mode_next(state.mode);
}

static void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    int bx = 14;
    int by = TOP_H() - 86;
    int bw = 36, bh = 36;

    Rectangle r_plus  = {(float)bx, (float)by,      (float)bw, (float)bh};
    Rectangle r_minus = {(float)bx, (float)(by+42),  (float)bw, (float)bh};

    bool hp = CheckCollisionPointRec(mouse, r_plus);
    bool hm = CheckCollisionPointRec(mouse, r_minus);

    DrawRectangleRec(r_plus,  hp ? Color{60,60,100,255} : Color{28,28,48,220});
    DrawRectangleLinesEx(r_plus,  1, {80,80,130,255});
    DrawText("+", bx+11, by+8,    18, hp ? WHITE : Color{180,180,210,220});

    DrawRectangleRec(r_minus, hm ? Color{60,60,100,255} : Color{28,28,48,220});
    DrawRectangleLinesEx(r_minus, 1, {80,80,130,255});
    DrawText("-", bx+13, by+50,   18, hm ? WHITE : Color{180,180,210,220});

    if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        cam.zoom = Clamp(cam.zoom + 0.15f, 0.05f, 5.0f);
    if (hm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        cam.zoom = Clamp(cam.zoom - 0.15f, 0.05f, 5.0f);

    char zbuf[16];
    snprintf(zbuf, sizeof(zbuf), "%.0f%%", cam.zoom * 100.0f);
    int tw = MeasureText(zbuf, 11);
    DrawText(zbuf, bx + bw/2 - tw/2, by + bh*2 + 10, 11, {120,120,160,200});
}

// Trunca el label para que quepa en la burbuja
static std::string short_label(const std::string& label, float radius) {
    int max_chars = (int)(radius * 1.6f / 7.0f);
    if (max_chars < 1) max_chars = 1;
    if ((int)label.size() <= max_chars) return label;
    return label.substr(0, max_chars - 1) + ".";
}

// ── Dibujar sprite dentro de una burbuja (espacio de mundo 2D) ───────────────
// cx, cy: centro de la burbuja; radius: radio de la burbuja
// La imagen se escala para ocupar ~70% del diametro de la burbuja.
static void draw_bubble_sprite(AppState& state, const std::string& key,
    float cx, float cy, float radius)
{
    if (key.empty()) return;
    Texture2D tex = state.textures.get(key);
    if (tex.id == 0) return;

    float size  = radius * 1.4f;   // 70% del diametro
    float scale = size / (float)std::max(tex.width, tex.height);
    float dw    = tex.width  * scale;
    float dh    = tex.height * scale;

    DrawTexturePro(tex,
        { 0, 0, (float)tex.width, (float)tex.height },
        { cx - dw * 0.5f, cy - dh * 0.5f, dw, dh },
        { 0, 0 }, 0.0f,
        { 255, 255, 255, 200 });   // ligera transparencia para que el label sea legible
}

void draw_bubble_view(AppState& state, Camera2D& cam, Vector2 mouse) {
    MathNode* cur = state.current();
    bool in_canvas = mouse.x < CANVAS_W() && mouse.y < TOP_H();

    // Detectar si el mouse esta sobre una burbuja
    bool over_bubble = false;
    if (cur && in_canvas) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        if (sqrtf(wm.x*wm.x + wm.y*wm.y) < 178.0f) over_bubble = true;
        if (!over_bubble) {
            for (const auto& child : cur->children) {
                float dx = wm.x - child->x, dy = wm.y - child->y;
                if (dx*dx + dy*dy < child->radius * child->radius) {
                    over_bubble = true; break;
                }
            }
        }
    }

    // Pan
    if (in_canvas) {
        bool pan = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !over_bubble)
                || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        if (pan) {
            Vector2 d = GetMouseDelta();
            cam.target.x -= d.x / cam.zoom;
            cam.target.y -= d.y / cam.zoom;
        }
    }

    // Zoom rueda
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && in_canvas) {
        cam.offset = mouse;
        cam.target = GetScreenToWorld2D(mouse, cam);
        cam.zoom   = Clamp(cam.zoom + wheel * 0.1f, 0.05f, 5.0f);
    }

    cam.offset = { (float)CX(), (float)(TOP_H() / 2) };

    BeginScissorMode(0, 0, CANVAS_W(), TOP_H());
    BeginMode2D(cam);

    // Lineas de conexion
    if (cur && !cur->children.empty()) {
        for (const auto& child : cur->children) {
            float len = sqrtf(child->x*child->x + child->y*child->y);
            if (len < 0.001f) continue;
            float nx = child->x/len, ny = child->y/len;
            DrawLineEx({nx*180.0f, ny*180.0f},
                       {child->x - nx*child->radius, child->y - ny*child->radius},
                       1.0f, Fade(child->color, 0.30f));
        }
    }

    // ── Burbuja central ───────────────────────────────────────────────────────
    Color center_col = (cur && cur->code != "ROOT")
        ? Fade(cur->color, 0.20f) : Color{228,228,240,255};
    DrawCircle(0, 0, 178, center_col);
    DrawCircleLines(0, 0, 178, {170,170,200,255});

    // Sprite de la burbuja central
    if (cur && !cur->texture_key.empty())
        draw_bubble_sprite(state, cur->texture_key, 0.0f, 0.0f, 170.0f);

    const char* clabel = (!cur || cur->code == "ROOT")
        ? mode_name(state.mode) : cur->label.c_str();
    std::string cl_str(clabel);
    if ((int)cl_str.size() > 18) cl_str = cl_str.substr(0, 17) + ".";
    int clw = MeasureText(cl_str.c_str(), 20);
    // Si hay sprite, poner el label en la parte inferior de la burbuja
    int label_y = (cur && !cur->texture_key.empty()) ? 140 : -10;
    DrawText(cl_str.c_str(), -clw/2, label_y, 20, {25,25,50,255});

    // ── Anillo de hijos ───────────────────────────────────────────────────────
    if (cur && !cur->children.empty()) {
        Vector2 wm = GetScreenToWorld2D(mouse, cam);
        for (const auto& child : cur->children) {
            float dx = wm.x - child->x, dy = wm.y - child->y;
            bool hovered = (dx*dx + dy*dy) < (child->radius * child->radius)
                           && in_canvas;

            DrawCircleV({child->x,child->y}, child->radius+2, Fade(child->color,0.18f));
            DrawCircleV({child->x,child->y}, child->radius, child->color);

            // Sprite del hijo (si tiene texture_key)
            if (!child->texture_key.empty())
                draw_bubble_sprite(state, child->texture_key,
                    child->x, child->y, child->radius);

            if (hovered) {
                DrawCircleLinesV({child->x,child->y}, child->radius+5, WHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.selected_code  = child->code;
                    state.selected_label = child->label;
                    if (!child->children.empty()) state.push(child);
                }
            }

            // Label: debajo del sprite o centrado si no hay sprite
            std::string lbl = short_label(child->label, child->radius);
            int tw = MeasureText(lbl.c_str(), 10);
            int label_off = (!child->texture_key.empty()) ? (int)(child->radius * 0.55f) : -5;
            DrawText(lbl.c_str(),
                (int)(child->x - tw*0.5f),
                (int)(child->y + label_off), 10, BLACK);
        }
    }

    if (!cur || cur->children.empty()) {
        const char* msg = cur ? "Sin sub-areas" : "Datos no cargados";
        int tw = MeasureText(msg, 14);
        DrawText(msg, -tw/2, 210, 14, {90,90,110,255});
    }

    EndMode2D();
    EndScissorMode();

    draw_zoom_buttons(cam, mouse);

    if (state.can_go_back()) {
        Rectangle br = {14.0f, 56.0f, 88.0f, 28.0f};
        bool bh = CheckCollisionPointRec(mouse, br);
        DrawRectangleRec(br, bh ? Color{55,55,95,255} : Color{28,28,48,220});
        DrawRectangleLinesEx(br, 1, {80,80,130,255});
        DrawText("< Atras", 22, 63, 13, bh ? WHITE : Color{160,160,200,220});
        if (bh && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.pop();
    }
}
