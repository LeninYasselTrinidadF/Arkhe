#include "ui/search_panel/search_widgets.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/nine_patch.hpp"
#include "ui/aesthetic/skin.hpp"
#include "ui/aesthetic/theme.hpp"
#include "raylib.h"
#include "ui/constants.hpp"
#include <algorithm>

void search_draw_divider_h(int y) {
    DrawLine(CANVAS_W() + 8, y, SW() - 8, y, th_alpha(g_theme.border));
}

void search_draw_field(int px, int y, int pw, int h, const char* buf) {
    const Theme& th = g_theme;
    if (g_skin.field.valid())
        g_skin.field.draw((float)px, (float)y, (float)pw, (float)h, th.bg_field);
    else {
        DrawRectangle(px, y, pw, h, th.bg_field);
        DrawRectangleLines(px, y, pw, h, th.border);
    }
    DrawTextF(buf, px + 6, y + (h - 13) / 2, 13, th.text_primary);
}

bool search_draw_button(const char* label, int x, int y, int w, int h, bool hov) {
    const Theme& th = g_theme;
    Color bg = hov ? th.accent : th_alpha(th.accent_dim);
    if (g_skin.button.valid())
        g_skin.button.draw((float)x, (float)y, (float)w, (float)h, bg);
    else
        DrawRectangleRec({ (float)x, (float)y, (float)w, (float)h }, bg);
    DrawTextF(label, x + (w - MeasureTextF(label, 12)) / 2, y + (h - 12) / 2,
              12, th.text_primary);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void search_draw_result_card(int px, int y, int pw, int h, bool hov, bool selected) {
    const Theme& th = g_theme;
    Color bg     = selected ? Color{ 38, 70, 110, 255 }
                 : hov      ? th.bg_card_hover
                            : th.bg_card;
    Color border = hov ? th.border_hover : th_alpha(th.border);
    if (g_skin.card.valid()) {
        g_skin.card.draw((float)px, (float)y, (float)pw, (float)h, bg);
        DrawRectangleLinesEx({ (float)px, (float)y, (float)pw, (float)h }, 1.0f, border);
    } else {
        DrawRectangle(px, y, pw, h, bg);
        DrawRectangleLinesEx({ (float)px, (float)y, (float)pw, (float)h }, 1, border);
    }
}

float search_draw_scrollbar(int sx, int area_top, int area_h,
    float cont_h, float offset,
    Vector2 mouse, bool in_area,
    bool& dragging, float& drag_start_y, float& drag_start_off)
{
    const float max_off = std::max(0.f, cont_h - (float)area_h);
    if (max_off <= 0.f) return 0.f;

    const Theme& th   = g_theme;
    constexpr int SB_W = 6;

    DrawRectangle(sx, area_top, SB_W, area_h, th_alpha(th.bg_button));

    float ratio   = std::clamp((float)area_h / cont_h, 0.05f, 1.f);
    int   thumb_h = std::max(18, (int)((float)area_h * ratio));
    float t       = std::clamp(offset / max_off, 0.f, 1.f);
    int   thumb_y = area_top + (int)(t * (float)(area_h - thumb_h));

    Rectangle thumb     = { (float)sx, (float)thumb_y, (float)SB_W, (float)thumb_h };
    bool      hov_thumb = CheckCollisionPointRec(mouse, thumb);

    DrawRectangleRec(thumb, (hov_thumb || dragging) ? th.accent : th_alpha(th.ctrl_border));

    if (hov_thumb && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging       = true;
        drag_start_y   = mouse.y;
        drag_start_off = offset;
    }
    if (dragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float scale = max_off / (float)std::max(1, area_h - thumb_h);
            offset = std::clamp(drag_start_off + (mouse.y - drag_start_y) * scale,
                                0.f, max_off);
        } else {
            dragging = false;
        }
    }
    if (in_area) {
        float wh = GetMouseWheelMove();
        if (wh != 0.f)
            offset = std::clamp(offset - wh * 40.f, 0.f, max_off);
    }
    return offset;
}

int search_draw_pager(int px, int pw, int y,
    int page, int total_pages,
    Vector2 mouse, bool panel_active)
{
    if (total_pages <= 1) return page;
    const Theme& th = g_theme;
    int  btn_sz = g_fonts.scale(14) + g_fonts.scale(10);
    char info[32];
    snprintf(info, sizeof(info), "%d / %d", page + 1, total_pages);
    int iw      = MeasureTextF(info, 11);
    int total_w = btn_sz * 2 + iw + g_fonts.scale(20);
    int bx      = px + (pw - total_w) / 2;

    Rectangle pr = { (float)bx,                                        (float)y, (float)btn_sz, (float)btn_sz };
    Rectangle nr = { (float)(bx + btn_sz + iw + g_fonts.scale(20)), (float)y, (float)btn_sz, (float)btn_sz };

    bool hp = panel_active && page > 0               && CheckCollisionPointRec(mouse, pr);
    bool hn = panel_active && page < total_pages - 1 && CheckCollisionPointRec(mouse, nr);

    auto draw_pbtn = [&](Rectangle r, const char* lbl, bool active) {
        DrawRectangleRec(r, active ? th.bg_button_hover : th_alpha(th.bg_button));
        DrawRectangleLinesEx(r, 1, th_alpha(th.ctrl_border));
        int tw  = MeasureTextF(lbl, 12), fh = g_fonts.scale(12);
        DrawTextF(lbl, (int)r.x + ((int)r.width  - tw) / 2,
                       (int)r.y + ((int)r.height - fh) / 2,
                  12, active ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    };
    draw_pbtn(pr, "<", hp);
    DrawTextF(info, bx + btn_sz + g_fonts.scale(10),
              y + (btn_sz - g_fonts.scale(11)) / 2, 11, th_alpha(th.text_dim));
    draw_pbtn(nr, ">", hn);

    if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return page - 1;
    if (hn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return page + 1;
    return page;
}
