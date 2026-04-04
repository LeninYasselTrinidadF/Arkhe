#include "ui/search_panel/search_panel.hpp"
#include "ui/search_panel/search_widgets.hpp"
#include "ui/search_panel/search_layout.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/aesthetic/skin.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/constants.hpp"
#include "raylib.h"

SearchPanel search_panel;

void SearchPanel::draw(AppState& state, const MathNode* search_root, Vector2 mouse) {
    const Theme& th = g_theme;

    // ── Layout (calculado una vez por frame) ──────────────────────────────────
    constexpr int SB_W = 7;
    SearchLayout L;
    L.px       = CANVAS_W() + 10;
    L.pw       = PANEL_W() - SB_W - 16;
    L.SB_X     = CANVAS_W() + PANEL_W() - SB_W - 3;
    L.field_h  = g_fonts.scale(13) + g_fonts.scale(10);
    L.lbl_gap  = g_fonts.scale(4);
    L.item_gap = g_fonts.scale(5);
    L.pager_h  = g_fonts.scale(14) + g_fonts.scale(10) + L.item_gap * 2;
    L.LBL_SZ   = 11;
    L.ITEM_SZ  = 11;

    // ── Fondo del panel ───────────────────────────────────────────────────────
    const int panel_top = UI_TOP();
    if (g_skin.panel.valid())
        g_skin.panel.draw((float)CANVAS_W(), (float)panel_top,
                          (float)PANEL_W(),  (float)TOP_H(), th.bg_panel);
    else
        DrawRectangle(CANVAS_W(), panel_top, PANEL_W(), TOP_H(), th.bg_panel);

    const bool panel_hover  = mouse.x > CANVAS_W() && !overlay::blocks_mouse(mouse);
    const bool panel_active = panel_hover || g_kbnav.in(FocusZone::Search);

    // ── Cabecera ──────────────────────────────────────────────────────────────
    int y = panel_top;
    DrawTextF("BUSQUEDA", L.px, y + L.lbl_gap + 2, 13, th_alpha(th.text_dim));
    const int div1 = y + g_fonts.scale(13) + L.lbl_gap * 2 + 4;
    search_draw_divider_h(div1);

    // ── Mitad local (campo + lista) ───────────────────────────────────────────
    const int half_y = panel_top + TOP_H() / 2;
    local_list.draw(state, search_root, L, panel_active, mouse,
                    div1 + L.lbl_gap + 4, half_y);

    // ── Divisor central + sección Loogle ─────────────────────────────────────
    search_draw_divider_h(half_y);
    loogle_list.draw(state, L, panel_active, mouse,
                     half_y + L.lbl_gap + 4, panel_top + TOP_H());
}
