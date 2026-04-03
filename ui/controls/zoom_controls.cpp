#include "zoom_controls.hpp"
#include "../core/overlay.hpp"
#include "../core/theme.hpp"
#include "../core/font_manager.hpp"
#include "../core/nine_patch.hpp"
#include "../core/skin.hpp"
#include "../constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>

void draw_zoom_buttons(Camera2D& cam, Vector2 mouse) {
    const Theme& th = g_theme;
    int btn_sz = MeasureTextF("+", 20) + g_fonts.scale(14);
    int bx = 14;

    const int vs_top = g_split_y - 22 - 10;
    int by = vs_top - 40 - (btn_sz * 2 + g_fonts.scale(6));
    if (by < UI_TOP() + 10) by = UI_TOP() + 10;

    Rectangle rp = { (float)bx, (float)by,                              (float)btn_sz, (float)btn_sz };
    Rectangle rm = { (float)bx, (float)(by + btn_sz + g_fonts.scale(6)), (float)btn_sz, (float)btn_sz };
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
    draw_btn(rp, hp);
    draw_btn(rm, hm);

    int sym_sz = 20;
    int sym_w = MeasureTextF("+", sym_sz);
    int sym_h = MeasureTextF("A", sym_sz);
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
    int lbl_sz = 11;
    int tw = MeasureTextF(zbuf, lbl_sz);
    DrawTextF(zbuf, bx + btn_sz / 2 - tw / 2,
        (int)rm.y + btn_sz + g_fonts.scale(4), lbl_sz, th_alpha(th.ctrl_text_dim));
}