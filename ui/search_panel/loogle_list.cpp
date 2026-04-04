#include "ui/search_panel/loogle_list.hpp"
#include "ui/search_panel/search_widgets.hpp"
#include "data/search/loogle.hpp"
#include "data/search/search_utils.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include <algorithm>
#include <cstring>

static constexpr int PAGE_SIZE = 25;

void LoogleList::draw(AppState& state, const SearchLayout& L,
                       bool panel_active, Vector2 mouse,
                       int y_start, int y_bottom)
{
    const Theme& th = g_theme;
    int ly = y_start;

    // ── Etiqueta + campo + botón ──────────────────────────────────────────────
    DrawTextF("Loogle (Mathlib)", L.px, ly, L.LBL_SZ, th_alpha(th.text_dim));
    ly += g_fonts.scale(L.LBL_SZ) + L.lbl_gap;

    const int field_y = ly;
    const int btn_w   = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    search_draw_field(L.px, ly, L.pw - btn_w - 4, L.field_h, state.loogle_buf);

    Rectangle btn_r  = { (float)(L.px + L.pw - btn_w), (float)ly,
                          (float)btn_w, (float)L.field_h };
    bool btn_hov     = panel_active && CheckCollisionPointRec(mouse, btn_r);
    search_draw_button("Buscar", (int)btn_r.x, (int)btn_r.y, btn_w, L.field_h, btn_hov);

    // ── Foco del campo ────────────────────────────────────────────────────────
    Rectangle field_r = { (float)L.px, (float)field_y,
                           (float)(L.pw - btn_w - 4), (float)L.field_h };
    if (panel_active && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focus = CheckCollisionPointRec(mouse, field_r);
    if (g_kbnav.in(FocusZone::Search))
        focus = (g_kbnav.search_idx == 1);

    // ── Input de teclado ──────────────────────────────────────────────────────
    if (focus) {
        int prev = (int)strlen(state.loogle_buf);
        int key  = GetCharPressed();
        while (key > 0) {
            int len = (int)strlen(state.loogle_buf);
            if (key >= 32 && key <= 125 && len < 255) {
                state.loogle_buf[len]     = (char)key;
                state.loogle_buf[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(state.loogle_buf);
            if (len > 0) state.loogle_buf[len - 1] = '\0';
        }
        if ((int)strlen(state.loogle_buf) != prev) loogle_page = 0;
        if (IsKeyPressed(KEY_ENTER) && strlen(state.loogle_buf) > 0) {
            loogle_search_async(state, state.loogle_buf);
            loogle_page = 0; loogle_scroll = 0.f;
        }
    }

    // ── Botón "Buscar" ────────────────────────────────────────────────────────
    if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        && strlen(state.loogle_buf) > 0)
    {
        loogle_search_async(state, state.loogle_buf);
        loogle_page = 0; loogle_scroll = 0.f;
    }
    ly += L.field_h + L.item_gap;

    // ── Estado carga / error ──────────────────────────────────────────────────
    if (state.loogle_loading.load()) {
        DrawTextF("Buscando...", L.px, ly, 12, th.success);
        ly += g_fonts.scale(14);
    } else if (!state.loogle_error.empty()) {
        DrawTextF(state.loogle_error.c_str(), L.px, ly, 11, { 200, 100, 100, 255 });
        ly += g_fonts.scale(13);
    }

    // ── Lista ─────────────────────────────────────────────────────────────────
    if (state.loogle_results.empty()) return;

    const int lcard_h = g_fonts.scale(12) + g_fonts.scale(10) * 2 + L.lbl_gap * 4 + 2;
    const int lstride = lcard_h + L.item_gap;
    const int ll_top  = ly;
    const int ll_bot  = y_bottom - L.pager_h - L.item_gap - 4;
    const int ll_h    = std::max(0, ll_bot - ll_top);
    if (ll_h <= 0) return;

    const int total  = (int)state.loogle_results.size();
    int tot_pg       = std::max(1, (total + PAGE_SIZE - 1) / PAGE_SIZE);
    loogle_page      = std::clamp(loogle_page, 0, tot_pg - 1);

    const int page_start = loogle_page * PAGE_SIZE;
    const int page_end   = std::min(page_start + PAGE_SIZE, total);
    const int page_count = std::max(0, page_end - page_start);

    const float lcont = (float)(page_count * lstride);
    const float lmax  = std::max(0.f, lcont - (float)ll_h);
    loogle_scroll     = std::clamp(loogle_scroll, 0.f, lmax);

    const bool in_list = panel_active
        && mouse.x > L.px && mouse.x < L.px + L.pw
        && mouse.y >= ll_top && mouse.y < ll_bot;

    loogle_scroll = search_draw_scrollbar(L.SB_X, ll_top, ll_h,
                                          lcont, loogle_scroll, mouse, in_list,
                                          sb_drag, sb_dy, sb_doff);
    loogle_scroll = std::clamp(loogle_scroll, 0.f, lmax);

    BeginScissorMode(CANVAS_W(), ll_top, PANEL_W(), ll_h);
    for (int i = 0; i < page_count; i++) {
        int cy = ll_top + i * lstride - (int)loogle_scroll;
        if (cy + lcard_h < ll_top) continue;
        if (cy > ll_bot)           break;

        const auto& r   = state.loogle_results[page_start + i];
        const bool  hov = in_list && mouse.y > cy && mouse.y < cy + lcard_h;
        search_draw_result_card(L.px, cy, L.pw, lcard_h, hov, false);

        std::string name = r.name.size()     > 38 ? r.name.substr(0, 37)     + "." : r.name;
        std::string mod  = r.module.size()   > 44 ? r.module.substr(0, 43)   + "." : r.module;
        std::string sig  = r.type_sig.size() > 48 ? r.type_sig.substr(0, 47) + "." : r.type_sig;

        DrawTextF(name.c_str(), L.px + 8, cy + L.lbl_gap, 12, th.text_code);
        DrawTextF(mod.c_str(),  L.px + 8, cy + g_fonts.scale(12) + L.lbl_gap * 2,
                  10, th_alpha(th.text_secondary));
        DrawTextF(sig.c_str(),  L.px + 8,
                  cy + g_fonts.scale(12) + g_fonts.scale(10) + L.lbl_gap * 3,
                  10, th_alpha(th.text_dim));

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto found = find_node_by_module(state.mathlib_root, r.module);
            if (found) {
                std::shared_ptr<MathNode> decl;
                for (auto& child : found->children)
                    if (child->label == r.name || child->code == r.name)
                        { decl = child; break; }
                auto dest = decl ? decl : found;
                state.pending_nav = { true, ViewMode::Mathlib, dest->code,
                                      dest->label, state.mathlib_root, found };
                if (decl) {
                    state.pending_nav.code  = decl->code;
                    state.pending_nav.label = decl->label;
                }
            } else {
                OpenURL(("https://loogle.lean-lang.org/?q=" + r.name).c_str());
            }
        }
    }
    EndScissorMode();

    // ── Paginador ─────────────────────────────────────────────────────────────
    int new_lp = search_draw_pager(L.px, L.pw, ll_bot + L.item_gap,
                                   loogle_page, tot_pg, mouse, panel_active);
    if (new_lp != loogle_page) {
        loogle_page   = new_lp;
        loogle_scroll = 0.f;
    }

    // ── Contador de resultados ────────────────────────────────────────────────
    char cnt[32];
    snprintf(cnt, sizeof(cnt), "%d resultado%s", total, total == 1 ? "" : "s");
    DrawTextF(cnt,
              L.SB_X - MeasureTextF(cnt, 10) - 4,
              ll_bot + L.item_gap + (L.pager_h - g_fonts.scale(10)) / 2,
              10, th_alpha(th.text_dim));
}
