#include "ui/search_panel/local_search_list.hpp"
#include "ui/search_panel/search_widgets.hpp"
#include "ui/key_controls/kbnav_search/kbnav_search.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include <algorithm>
#include <cstring>

static constexpr int PAGE_SIZE      = 25;
static constexpr int LOCAL_FULL_MAX = 2000;

void LocalSearchList::draw(AppState& state, const MathNode* search_root,
                            const SearchLayout& L, bool panel_active,
                            Vector2 mouse, int y_start, int y_bottom)
{
    const Theme& th = g_theme;
    int y = y_start;

    // ── Etiqueta + campo + botón ──────────────────────────────────────────────
    DrawTextF("Local (fuzzy)", L.px, y, L.LBL_SZ, th_alpha(th.text_dim));
    y += g_fonts.scale(L.LBL_SZ) + L.lbl_gap;

    const int field_y = y;
    const int btn_w   = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    search_draw_field(L.px, y, L.pw - btn_w - 4, L.field_h, state.search_buf);

    Rectangle btn_r   = { (float)(L.px + L.pw - btn_w), (float)y,
                           (float)btn_w, (float)L.field_h };
    bool btn_hov      = panel_active && CheckCollisionPointRec(mouse, btn_r);
    search_draw_button("Buscar", (int)btn_r.x, (int)btn_r.y, btn_w, L.field_h, btn_hov);
    y += L.field_h + L.item_gap;

    // ── Registro kbnav: campo y botón ─────────────────────────────────────────
    Rectangle field_r = { (float)L.px, (float)field_y,
                           (float)(L.pw - btn_w - 4), (float)L.field_h };
    const int field_idx = kbnav_search_register(SearchNavKind::LocalField,  field_r);
    const int btn_idx   = kbnav_search_register(SearchNavKind::LocalButton, btn_r);

    // ── Foco del campo ────────────────────────────────────────────────────────
    if (panel_active && !g_kbnav.in(FocusZone::Search)
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focus = CheckCollisionPointRec(mouse, field_r);
    if (g_kbnav.in(FocusZone::Search))
        focus = kbnav_search_is_focused(field_idx);
    if (!panel_active)
        focus = false;

    // ── Input de teclado (solo cuando el campo tiene foco) ────────────────────
    if (focus) {
        int prev = (int)strlen(state.search_buf);
        int key  = GetCharPressed();
        while (key > 0) {
            int len = (int)strlen(state.search_buf);
            if (key >= 32 && key <= 125 && len < 255) {
                state.search_buf[len]     = (char)key;
                state.search_buf[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(state.search_buf);
            if (len > 0) state.search_buf[len - 1] = '\0';
        }
        if ((int)strlen(state.search_buf) != prev) {
            scroll = 0.f; page = 0; full_loaded = false;
            full_hits.clear(); page_hits.clear();
        }
    }

    // ── Botón "Buscar" (mouse o kbnav Enter) ──────────────────────────────────
    const bool btn_activated = (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                             || kbnav_search_is_activated(btn_idx);
    if (btn_activated && strlen(state.search_buf) > 0 && search_root) {
        full_hits   = fuzzy_search(search_root, state.search_buf, LOCAL_FULL_MAX);
        full_loaded = true;
        page        = 0;
        scroll      = 0.f;
    }

    // ── Lista ─────────────────────────────────────────────────────────────────
    const int list_top = y;
    const int list_bot = y_bottom - L.pager_h - L.item_gap;
    const int list_h   = std::max(0, list_bot - list_top);

    if (strlen(state.search_buf) == 0 || !search_root || list_h <= 0) return;

    const int card_h = g_fonts.scale(L.ITEM_SZ) * 2 + L.lbl_gap * 3 + 4;
    const int stride = card_h + L.item_gap;

    if (!full_loaded
        && (page_hits.empty() || last_query != std::string(state.search_buf)))
    {
        last_query = state.search_buf;
        page_hits  = fuzzy_search(search_root, state.search_buf, PAGE_SIZE);
    }

    const std::vector<SearchHit>& all =
        full_loaded ? full_hits : page_hits;

    const int  known_total   = (int)all.size();
    const bool may_have_more = !full_loaded && (known_total >= PAGE_SIZE);
    int        tot_pg        = std::max(1, (known_total + PAGE_SIZE - 1) / PAGE_SIZE);
    if (may_have_more) tot_pg += 1;
    page = std::clamp(page, 0, tot_pg - 1);

    const int page_start = page * PAGE_SIZE;
    const int page_end   = std::min(page_start + PAGE_SIZE, known_total);
    const int page_count = std::max(0, page_end - page_start);

    const float cont_h  = (float)(page_count * stride);
    const float max_off = std::max(0.f, cont_h - (float)list_h);
    scroll = std::clamp(scroll, 0.f, max_off);

    const bool in_list = panel_active
        && mouse.x > L.px && mouse.x < L.px + L.pw
        && mouse.y >= list_top && mouse.y < list_bot;

    scroll = search_draw_scrollbar(L.SB_X, list_top, list_h,
                                   cont_h, scroll, mouse, in_list,
                                   sb_drag, sb_dy, sb_doff);
    scroll = std::clamp(scroll, 0.f, max_off);

    BeginScissorMode(CANVAS_W(), list_top, PANEL_W(), list_h);
    for (int i = 0; i < page_count; i++) {
        int cy = list_top + i * stride - (int)scroll;
        if (cy + card_h < list_top) continue;
        if (cy > list_bot)          break;

        const auto& hit = all[page_start + i];

        // Registrar la tarjeta como ítem navegable
        Rectangle card_r  = { (float)L.px, (float)cy, (float)L.pw, (float)card_h };
        const int card_idx = kbnav_search_register(SearchNavKind::LocalResult, card_r);

        const bool sel       = (hit.node->code == state.selected_code);
        const bool hov       = in_list && mouse.y > cy && mouse.y < cy + card_h;
        const bool kb_sel    = kbnav_search_is_focused(card_idx);
        const bool activated = kbnav_search_is_activated(card_idx);

        search_draw_result_card(L.px, cy, L.pw, card_h, hov, sel || kb_sel);
        DrawTextF(hit.node->code.c_str(), L.px + 6, cy + L.lbl_gap,
                  L.ITEM_SZ, th.text_code);
        std::string sl = hit.node->label.size() > 35
            ? hit.node->label.substr(0, 34) + "." : hit.node->label;
        DrawTextF(sl.c_str(), L.px + 6, cy + g_fonts.scale(L.ITEM_SZ) + L.lbl_gap * 2,
                  10, th_alpha(th.text_secondary));

        if ((hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || activated) {
            state.selected_code  = hit.node->code;
            state.selected_label = hit.node->label;
            if (!hit.node->children.empty()) state.push(hit.node);
        }
    }
    EndScissorMode();

    // ── Paginador ─────────────────────────────────────────────────────────────
    // Registrar los botones de página antes de dibujarlos para que kbnav los vea.
    const int pager_y = list_bot + L.item_gap;
    int prev_idx = -1, next_idx = -1;
    if (tot_pg > 1) {
        const int btn_sz  = g_fonts.scale(14) + g_fonts.scale(10);
        char info[32]; snprintf(info, sizeof(info), "%d / %d", page + 1, tot_pg);
        const int iw      = MeasureTextF(info, 11);
        const int total_w = btn_sz * 2 + iw + g_fonts.scale(20);
        const int bx      = L.px + (L.pw - total_w) / 2;
        Rectangle pr = { (float)bx, (float)pager_y, (float)btn_sz, (float)btn_sz };
        Rectangle nr = { (float)(bx + btn_sz + iw + g_fonts.scale(20)),
                         (float)pager_y, (float)btn_sz, (float)btn_sz };
        if (page > 0)            prev_idx = kbnav_search_register(SearchNavKind::LocalPagerPrev, pr);
        if (page < tot_pg - 1)   next_idx = kbnav_search_register(SearchNavKind::LocalPagerNext, nr);
    }

    int new_pg = search_draw_pager(L.px, L.pw, pager_y, page, tot_pg, mouse, panel_active);

    // Activación por kbnav
    if (kbnav_search_is_activated(prev_idx)) new_pg = page - 1;
    if (kbnav_search_is_activated(next_idx)) new_pg = page + 1;

    if (new_pg != page) {
        if (!full_loaded) {
            full_hits   = fuzzy_search(search_root, state.search_buf, LOCAL_FULL_MAX);
            full_loaded = true;
            int real_tot = (int)full_hits.size();
            tot_pg  = std::max(1, (real_tot + PAGE_SIZE - 1) / PAGE_SIZE);
            new_pg  = std::clamp(new_pg, 0, tot_pg - 1);
        }
        page   = new_pg;
        scroll = 0.f;
    }

    // ── Contador de resultados ────────────────────────────────────────────────
    char cnt[48];
    if (may_have_more) snprintf(cnt, sizeof(cnt), "%d+ resultados", known_total);
    else               snprintf(cnt, sizeof(cnt), "%d resultado%s",
                                known_total, known_total == 1 ? "" : "s");
    DrawTextF(cnt,
              L.SB_X - MeasureTextF(cnt, 10) - 4,
              pager_y + (L.pager_h - g_fonts.scale(10)) / 2,
              10, th_alpha(th.text_dim));
}
