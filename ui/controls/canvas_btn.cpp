#include "canvas_btn.hpp"
#include "../core/font_manager.hpp"
#include "../core/nine_patch.hpp"
#include "../core/skin.hpp"

void btn_dims(const char* label, float& bw, float& bh) {
    int tw = MeasureTextF(label, BTN_FONT);
    int font_h = MeasureTextF("Ag", BTN_FONT);
    bw = (float)(tw + g_fonts.scale(BTN_PAD_X) * 2);
    bh = (float)(font_h + g_fonts.scale(BTN_PAD_Y) * 2);
}

float btn_gap() { return (float)g_fonts.scale(6); }

bool canvas_btn_impl(const Theme& th, Vector2 mouse, bool canvas_blocked,
    float bx, float by, const char* label,
    bool enabled, bool active,
    float* out_bw, float* out_bh)
{
    float bw, bh;
    btn_dims(label, bw, bh);
    if (out_bw) *out_bw = bw;
    if (out_bh) *out_bh = bh;

    bool hov = enabled && !canvas_blocked
        && CheckCollisionPointRec(mouse, { bx, by, bw, bh });

    Color bg = active ? th.accent
        : hov ? th.bg_button_hover
        : enabled ? th_alpha(th.bg_button)
        : Color{ th.bg_button.r, th.bg_button.g, th.bg_button.b, 80 };
    Color txt = active ? th.bg_app
        : hov ? th.ctrl_text
        : enabled ? th_alpha(th.ctrl_text_dim)
        : Color{ th.ctrl_text_dim.r, th.ctrl_text_dim.g, th.ctrl_text_dim.b, 80 };
    Color brd = active ? th.accent
        : enabled ? th.ctrl_border
        : th_alpha(th.border);

    if (g_skin.button.valid())
        g_skin.button.draw(bx, by, bw, bh, bg);
    else {
        DrawRectangleRec({ bx, by, bw, bh }, bg);
        DrawRectangleLinesEx({ bx, by, bw, bh }, active ? 2.f : 1.f, brd);
    }
    DrawTextF(label,
        (int)bx + g_fonts.scale(BTN_PAD_X),
        (int)by + g_fonts.scale(BTN_PAD_Y),
        BTN_FONT, txt);

    return hov && enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}