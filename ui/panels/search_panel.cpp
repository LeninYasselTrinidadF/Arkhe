#include "search_panel.hpp"
#include "../core/font_manager.hpp"
#include "../core/overlay.hpp"
#include "../search/loogle.hpp"
#include "../core/nine_patch.hpp"
#include "../core/skin.hpp"
#include "../core/theme.hpp"
#include "raylib.h"
#include <cstring>
#include <algorithm>
#include <queue>

static std::shared_ptr<MathNode> find_node_by_code(const std::shared_ptr<MathNode>& root, const std::string& target) {
    if (!root) return nullptr;
    std::queue<std::shared_ptr<MathNode>> q; q.push(root);
    while (!q.empty()) { auto n = q.front(); q.pop(); if (n->code == target)return n; for (auto& c : n->children)q.push(c); }
    return nullptr;
}

static std::shared_ptr<MathNode> find_node_by_module(const std::shared_ptr<MathNode>& root, const std::string& module) {
    if (!root || module.empty()) return nullptr;
    auto exact = find_node_by_code(root, module); if (exact)return exact;
    std::queue<std::shared_ptr<MathNode>> q; q.push(root);
    std::shared_ptr<MathNode> best; int best_len = 0;
    while (!q.empty()) { auto n = q.front(); q.pop(); if (!n->code.empty() && module.find(n->code) != std::string::npos && (int)n->code.size() > best_len) { best = n; best_len = (int)n->code.size(); }for (auto& c : n->children)q.push(c); }
    return best;
}

static void draw_divider_h(int y) {
    DrawLine(CANVAS_W() + 8, y, SW() - 8, y, th_alpha(g_theme.border));
}

static void draw_search_field(int px, int y, int pw, int h, const char* buf) {
    const Theme& th = g_theme;
    if (g_skin.field.valid()) g_skin.field.draw((float)px, (float)y, (float)pw, (float)h, th.bg_field);
    else { DrawRectangle(px, y, pw, h, th.bg_field); DrawRectangleLines(px, y, pw, h, th.border); }
    DrawTextF(buf, px + 6, y + (h - 13) / 2, 13, th.text_primary);
}

static bool draw_search_button(const char* label, int x, int y, int w, int h, bool hov) {
    const Theme& th = g_theme;
    Color bg = hov ? th.accent : th_alpha(th.accent_dim);
    if (g_skin.button.valid()) g_skin.button.draw((float)x, (float)y, (float)w, (float)h, bg);
    else DrawRectangleRec({ (float)x,(float)y,(float)w,(float)h }, bg);
    DrawTextF(label, x + (w - MeasureTextF(label, 12)) / 2, y + (h - 12) / 2, 12, th.text_primary);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void draw_result_card(int px, int y, int pw, int h, bool hov, bool selected) {
    const Theme& th = g_theme;
    Color bg = selected ? Color{ 38,70,110,255 } : hov ? th.bg_card_hover : th.bg_card;
    Color border = hov ? th.border_hover : th_alpha(th.border);
    if (g_skin.card.valid()) { g_skin.card.draw((float)px, (float)y, (float)pw, (float)h, bg); DrawRectangleLinesEx({ (float)px,(float)y,(float)pw,(float)h }, 1.0f, border); }
    else { DrawRectangle(px, y, pw, h, bg); DrawRectangleLinesEx({ (float)px,(float)y,(float)pw,(float)h }, 1, border); }
}

// ── Constantes ────────────────────────────────────────────────────────────────

static constexpr int PAGE_SIZE = 25;
static constexpr int LOCAL_FULL_MAX = 2000;

// ── Estado global ─────────────────────────────────────────────────────────────

static float s_local_scroll = 0.f;
static int   s_local_page = 0;
static bool  s_local_full_loaded = false;
static std::string             s_local_last_query;
static std::vector<SearchHit>  s_local_full_hits;
static std::vector<SearchHit>  s_local_page_hits;

static float s_loogle_scroll = 0.f;
static int   s_loogle_page = 0;

// ── Scrollbar ─────────────────────────────────────────────────────────────────
// Navega dentro del contenido de la página actual.
// cont_h: altura total de los ítems de la página.
// area_h: altura visible del rectángulo de lista.

static float draw_scrollbar(int sx, int area_top, int area_h,
    float cont_h, float offset,
    Vector2 mouse, bool in_area,
    bool& dragging, float& drag_start_y, float& drag_start_off)
{
    const float max_off = std::max(0.f, cont_h - (float)area_h);
    if (max_off <= 0.f) return 0.f;

    const Theme& th = g_theme;
    constexpr int SB_W = 6;

    DrawRectangle(sx, area_top, SB_W, area_h, th_alpha(th.bg_button));

    float ratio = std::clamp((float)area_h / cont_h, 0.05f, 1.f);
    int   thumb_h = std::max(18, (int)((float)area_h * ratio));
    float t = (max_off > 0.f) ? std::clamp(offset / max_off, 0.f, 1.f) : 0.f;
    int   thumb_y = area_top + (int)(t * (float)(area_h - thumb_h));

    Rectangle thumb = { (float)sx, (float)thumb_y, (float)SB_W, (float)thumb_h };
    bool hov_thumb = CheckCollisionPointRec(mouse, thumb);

    DrawRectangleRec(thumb, (hov_thumb || dragging) ? th.accent : th_alpha(th.ctrl_border));

    // Iniciar drag sobre el thumb
    if (hov_thumb && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging = true;
        drag_start_y = mouse.y;
        drag_start_off = offset;
    }

    // Aplicar drag (funciona aunque el mouse salga del área)
    if (dragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float scale = max_off / (float)std::max(1, area_h - thumb_h);
            offset = std::clamp(drag_start_off + (mouse.y - drag_start_y) * scale, 0.f, max_off);
        }
        else {
            dragging = false;
        }
    }

    // Rueda (solo dentro del área de lista)
    if (in_area) {
        float wh = GetMouseWheelMove();
        if (wh != 0.f)
            offset = std::clamp(offset - wh * 40.f, 0.f, max_off);
    }

    return offset;
}

// ── Paginador ─────────────────────────────────────────────────────────────────

static int draw_pager(int px, int pw, int y, int page, int total_pages,
    Vector2 mouse, bool panel_active)
{
    if (total_pages <= 1) return page;
    const Theme& th = g_theme;
    int btn_sz = g_fonts.scale(14) + g_fonts.scale(10);
    char info[32];
    snprintf(info, sizeof(info), "%d / %d", page + 1, total_pages);
    int iw = MeasureTextF(info, 11);
    int total_w = btn_sz * 2 + iw + g_fonts.scale(20);
    int bx = px + (pw - total_w) / 2;

    Rectangle pr = { (float)bx,                                      (float)y, (float)btn_sz, (float)btn_sz };
    Rectangle nr = { (float)(bx + btn_sz + iw + g_fonts.scale(20)), (float)y, (float)btn_sz, (float)btn_sz };

    bool can_prev = (page > 0);
    bool can_next = (page < total_pages - 1);
    bool hp = panel_active && can_prev && CheckCollisionPointRec(mouse, pr);
    bool hn = panel_active && can_next && CheckCollisionPointRec(mouse, nr);

    auto draw_pbtn = [&](Rectangle r, const char* lbl, bool active) {
        DrawRectangleRec(r, active ? th.bg_button_hover : th_alpha(th.bg_button));
        DrawRectangleLinesEx(r, 1, th_alpha(th.ctrl_border));
        int tw2 = MeasureTextF(lbl, 12), fh = g_fonts.scale(12);
        DrawTextF(lbl, (int)r.x + ((int)r.width - tw2) / 2,
            (int)r.y + ((int)r.height - fh) / 2, 12,
            active ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
        };
    draw_pbtn(pr, "<", hp);
    DrawTextF(info, bx + btn_sz + g_fonts.scale(10),
        y + (btn_sz - g_fonts.scale(11)) / 2, 11, th_alpha(th.text_dim));
    draw_pbtn(nr, ">", hn);

    if (hp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return page - 1;
    if (hn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return page + 1;
    return page;
}

// ── draw_search_panel ─────────────────────────────────────────────────────────

void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse) {
    const Theme& th = g_theme;
    const int SB_W = 7;
    const int SB_X = CANVAS_W() + PANEL_W() - SB_W - 3;
    int px = CANVAS_W() + 10, pw = PANEL_W() - SB_W - 16;

    const int LBL_SZ = 11;
    const int HDR_SZ = 13;
    const int FIELD_SZ = 13;
    const int ITEM_SZ = 11;
    const int field_h = g_fonts.scale(FIELD_SZ) + g_fonts.scale(10);
    const int lbl_gap = g_fonts.scale(4);
    const int item_gap = g_fonts.scale(5);
    const int pager_h = g_fonts.scale(14) + g_fonts.scale(10) + item_gap * 2;

    int panel_top = UI_TOP();
    if (g_skin.panel.valid()) g_skin.panel.draw((float)CANVAS_W(), (float)panel_top, (float)PANEL_W(), (float)TOP_H(), th.bg_panel);
    else DrawRectangle(CANVAS_W(), panel_top, PANEL_W(), TOP_H(), th.bg_panel);

    bool panel_active = mouse.x > CANVAS_W() && !overlay::blocks_mouse(mouse);

    // ── Cabecera ──────────────────────────────────────────────────────────────
    int y = panel_top;
    DrawTextF("BUSQUEDA", px, y + lbl_gap + 2, HDR_SZ, th_alpha(th.text_dim));
    int div1 = y + g_fonts.scale(HDR_SZ) + lbl_gap * 2 + 4;
    draw_divider_h(div1);
    y = div1 + lbl_gap + 4;

    // ── Campo local ───────────────────────────────────────────────────────────
    DrawTextF("Local (fuzzy)", px, y, LBL_SZ, th_alpha(th.text_dim));
    y += g_fonts.scale(LBL_SZ) + lbl_gap;
    int field_y = y;
    // El campo ocupa pw menos el botón Buscar (simétrico a la sección Loogle)
    int local_btn_w = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    draw_search_field(px, y, pw - local_btn_w - 4, field_h, state.search_buf);
    Rectangle local_btn = { (float)(px + pw - local_btn_w), (float)y,
                            (float)local_btn_w, (float)field_h };
    bool local_btn_hov = panel_active && CheckCollisionPointRec(mouse, local_btn);
    draw_search_button("Buscar", (int)local_btn.x, (int)local_btn.y,
        local_btn_w, field_h, local_btn_hov);
    y += field_h + item_gap;

    if (panel_active) {
        static bool local_focus = true;
        Rectangle lf = { (float)px, (float)field_y, (float)(pw - local_btn_w - 4), (float)field_h };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            local_focus = CheckCollisionPointRec(mouse, lf);
        if (local_focus) {
            int prev = (int)strlen(state.search_buf);
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(state.search_buf);
                if (key >= 32 && key <= 125 && len < 255) {
                    state.search_buf[len] = (char)key; state.search_buf[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = (int)strlen(state.search_buf);
                if (len > 0) state.search_buf[len - 1] = '\0';
            }
            if ((int)strlen(state.search_buf) != prev) {
                s_local_scroll = 0.f;
                s_local_page = 0;
                s_local_full_loaded = false;
                s_local_full_hits.clear();
                s_local_page_hits.clear();
            }
        }
        // Botón Buscar local: fuerza carga completa de resultados
        if (local_btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && strlen(state.search_buf) > 0 && search_root)
        {
            s_local_full_hits = fuzzy_search(search_root, state.search_buf, LOCAL_FULL_MAX);
            s_local_full_loaded = true;
            s_local_page = 0;
            s_local_scroll = 0.f;
        }
    }

    // ── Lista local ───────────────────────────────────────────────────────────
    int half_y = panel_top + TOP_H() / 2;
    int list_top = y;
    int list_bot = half_y - pager_h - item_gap;
    int list_h = std::max(0, list_bot - list_top);

    if (strlen(state.search_buf) > 0 && search_root && list_h > 0) {
        int card_h = g_fonts.scale(ITEM_SZ) * 2 + lbl_gap * 3 + 4;
        int stride = card_h + item_gap;

        // Fuente de hits: rápida (página 0) o completa cacheada
        if (!s_local_full_loaded &&
            (s_local_page_hits.empty() || s_local_last_query != std::string(state.search_buf)))
        {
            s_local_last_query = state.search_buf;
            s_local_page_hits = fuzzy_search(search_root, state.search_buf, PAGE_SIZE);
        }

        const std::vector<SearchHit>& all =
            s_local_full_loaded ? s_local_full_hits : s_local_page_hits;

        int known_total = (int)all.size();
        bool may_have_more = !s_local_full_loaded && (known_total >= PAGE_SIZE);
        int  tot_pg = std::max(1, (known_total + PAGE_SIZE - 1) / PAGE_SIZE);
        if (may_have_more) tot_pg += 1;
        s_local_page = std::clamp(s_local_page, 0, tot_pg - 1);

        // Slice de la página
        int page_start = s_local_page * PAGE_SIZE;
        int page_end = std::min(page_start + PAGE_SIZE, known_total);
        int page_count = std::max(0, page_end - page_start);

        // Scrollbar sobre los ítems de la página
        float cont_h = (float)(page_count * stride);
        float max_off = std::max(0.f, cont_h - (float)list_h);
        s_local_scroll = std::clamp(s_local_scroll, 0.f, max_off);

        bool in_list = panel_active
            && mouse.x > px && mouse.x < px + pw
            && mouse.y >= list_top && mouse.y < list_bot;

        static bool  sb_drag = false;
        static float sb_dy = 0.f, sb_doff = 0.f;
        s_local_scroll = draw_scrollbar(SB_X, list_top, list_h,
            cont_h, s_local_scroll,
            mouse, in_list,
            sb_drag, sb_dy, sb_doff);
        s_local_scroll = std::clamp(s_local_scroll, 0.f, max_off);

        BeginScissorMode(CANVAS_W(), list_top, PANEL_W(), list_h);
        for (int i = 0; i < page_count; i++) {
            int cy = list_top + i * stride - (int)s_local_scroll;
            if (cy + card_h < list_top) continue;
            if (cy > list_bot)          break;
            const auto& hit = all[page_start + i];
            bool sel = (hit.node->code == state.selected_code);
            bool hov = in_list && mouse.y > cy && mouse.y < cy + card_h;
            draw_result_card(px, cy, pw, card_h, hov, sel);
            DrawTextF(hit.node->code.c_str(), px + 6, cy + lbl_gap, ITEM_SZ, th.text_code);
            std::string sl = hit.node->label.size() > 35
                ? hit.node->label.substr(0, 34) + "." : hit.node->label;
            DrawTextF(sl.c_str(), px + 6, cy + g_fonts.scale(ITEM_SZ) + lbl_gap * 2,
                10, th_alpha(th.text_secondary));
            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state.selected_code = hit.node->code;
                state.selected_label = hit.node->label;
                if (!hit.node->children.empty()) state.push(hit.node);
            }
        }
        EndScissorMode();

        // Paginador — al cambiar de página lanza búsqueda completa si hace falta
        int new_pg = draw_pager(px, pw, list_bot + item_gap, s_local_page, tot_pg, mouse, panel_active);
        if (new_pg != s_local_page) {
            if (!s_local_full_loaded) {
                s_local_full_hits = fuzzy_search(search_root, state.search_buf, LOCAL_FULL_MAX);
                s_local_full_loaded = true;
                int real_tot = (int)s_local_full_hits.size();
                tot_pg = std::max(1, (real_tot + PAGE_SIZE - 1) / PAGE_SIZE);
                new_pg = std::clamp(new_pg, 0, tot_pg - 1);
            }
            s_local_page = new_pg;
            s_local_scroll = 0.f;
        }

        char cnt[48];
        if (may_have_more) snprintf(cnt, sizeof(cnt), "%d+ resultados", known_total);
        else               snprintf(cnt, sizeof(cnt), "%d resultado%s", known_total, known_total == 1 ? "" : "s");
        DrawTextF(cnt, SB_X - MeasureTextF(cnt, 10) - 4,
            list_bot + item_gap + (pager_h - g_fonts.scale(10)) / 2,
            10, th_alpha(th.text_dim));
    }

    // ── Divisor Loogle ────────────────────────────────────────────────────────
    draw_divider_h(half_y);
    int ly = half_y + lbl_gap + 4;

    DrawTextF("Loogle (Mathlib)", px, ly, LBL_SZ, th_alpha(th.text_dim));
    ly += g_fonts.scale(LBL_SZ) + lbl_gap;

    int btn_w = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    int lfld_y = ly;
    draw_search_field(px, ly, pw - btn_w - 4, field_h, state.loogle_buf);
    Rectangle btn = { (float)(px + pw - btn_w), (float)ly, (float)btn_w, (float)field_h };
    bool btn_hov = panel_active && CheckCollisionPointRec(mouse, btn);
    draw_search_button("Buscar", (int)btn.x, (int)btn.y, btn_w, field_h, btn_hov);

    if (panel_active) {
        static bool loogle_focus = false;
        Rectangle lf = { (float)px, (float)lfld_y, (float)(pw - btn_w - 4), (float)field_h };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            loogle_focus = CheckCollisionPointRec(mouse, lf);
        if (loogle_focus) {
            int prev = (int)strlen(state.loogle_buf);
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(state.loogle_buf);
                if (key >= 32 && key <= 125 && len < 255) {
                    state.loogle_buf[len] = (char)key; state.loogle_buf[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = (int)strlen(state.loogle_buf);
                if (len > 0) state.loogle_buf[len - 1] = '\0';
            }
            if ((int)strlen(state.loogle_buf) != prev) s_loogle_page = 0;
            if (IsKeyPressed(KEY_ENTER) && strlen(state.loogle_buf) > 0) {
                loogle_search_async(state, state.loogle_buf);
                s_loogle_page = 0; s_loogle_scroll = 0.f;
            }
        }
        if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && strlen(state.loogle_buf) > 0) {
            loogle_search_async(state, state.loogle_buf);
            s_loogle_page = 0; s_loogle_scroll = 0.f;
        }
    }
    ly += field_h + item_gap;

    if (state.loogle_loading.load()) {
        DrawTextF("Buscando...", px, ly, 12, th.success); ly += g_fonts.scale(14);
    }
    else if (!state.loogle_error.empty()) {
        DrawTextF(state.loogle_error.c_str(), px, ly, 11, { 200,100,100,255 }); ly += g_fonts.scale(13);
    }

    // ── Lista Loogle ──────────────────────────────────────────────────────────
    if (!state.loogle_results.empty()) {
        int lcard_h = g_fonts.scale(12) + g_fonts.scale(10) * 2 + lbl_gap * 4 + 2;
        int lstride = lcard_h + item_gap;
        int ll_top = ly;
        int ll_bot = panel_top + TOP_H() - pager_h - item_gap - 4;
        int ll_h = std::max(0, ll_bot - ll_top);
        int total = (int)state.loogle_results.size();

        if (ll_h > 0) {
            int tot_pg = std::max(1, (total + PAGE_SIZE - 1) / PAGE_SIZE);
            s_loogle_page = std::clamp(s_loogle_page, 0, tot_pg - 1);

            int page_start = s_loogle_page * PAGE_SIZE;
            int page_end = std::min(page_start + PAGE_SIZE, total);
            int page_count = std::max(0, page_end - page_start);

            float lcont = (float)(page_count * lstride);
            float lmax = std::max(0.f, lcont - (float)ll_h);
            s_loogle_scroll = std::clamp(s_loogle_scroll, 0.f, lmax);

            bool in_llist = panel_active
                && mouse.x > px && mouse.x < px + pw
                && mouse.y >= ll_top && mouse.y < ll_bot;

            static bool  ls_drag = false;
            static float ls_dy = 0.f, ls_doff = 0.f;
            s_loogle_scroll = draw_scrollbar(SB_X, ll_top, ll_h,
                lcont, s_loogle_scroll,
                mouse, in_llist,
                ls_drag, ls_dy, ls_doff);
            s_loogle_scroll = std::clamp(s_loogle_scroll, 0.f, lmax);

            BeginScissorMode(CANVAS_W(), ll_top, PANEL_W(), ll_h);
            for (int i = 0; i < page_count; i++) {
                int cy = ll_top + i * lstride - (int)s_loogle_scroll;
                if (cy + lcard_h < ll_top) continue;
                if (cy > ll_bot)           break;
                auto& r = state.loogle_results[page_start + i];
                bool hov = in_llist && mouse.y > cy && mouse.y < cy + lcard_h;
                draw_result_card(px, cy, pw, lcard_h, hov, false);
                std::string name = r.name.size() > 38 ? r.name.substr(0, 37) + "." : r.name;
                std::string mod = r.module.size() > 44 ? r.module.substr(0, 43) + "." : r.module;
                std::string sig = r.type_sig.size() > 48 ? r.type_sig.substr(0, 47) + "." : r.type_sig;
                DrawTextF(name.c_str(), px + 8, cy + lbl_gap, 12, th.text_code);
                DrawTextF(mod.c_str(), px + 8, cy + g_fonts.scale(12) + lbl_gap * 2,
                    10, th_alpha(th.text_secondary));
                DrawTextF(sig.c_str(), px + 8,
                    cy + g_fonts.scale(12) + g_fonts.scale(10) + lbl_gap * 3,
                    10, th_alpha(th.text_dim));
                if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    auto found = find_node_by_module(state.mathlib_root, r.module);
                    if (found) {
                        std::shared_ptr<MathNode> decl;
                        for (auto& child : found->children)
                            if (child->label == r.name || child->code == r.name)
                            {
                                decl = child; break;
                            }
                        auto dest = decl ? decl : found;
                        state.pending_nav = { true, ViewMode::Mathlib, dest->code,
                                              dest->label, state.mathlib_root, found };
                        if (decl) {
                            state.pending_nav.code = decl->code;
                            state.pending_nav.label = decl->label;
                        }
                    }
                    else {
                        OpenURL(("https://loogle.lean-lang.org/?q=" + r.name).c_str());
                    }
                }
            }
            EndScissorMode();

            // Paginador Loogle — mueve scroll sólo al cambiar de página
            int new_lp = draw_pager(px, pw, ll_bot + item_gap, s_loogle_page, tot_pg, mouse, panel_active);
            if (new_lp != s_loogle_page) {
                s_loogle_page = new_lp;
                s_loogle_scroll = 0.f;
            }

            char cnt[32];
            snprintf(cnt, sizeof(cnt), "%d resultado%s", total, total == 1 ? "" : "s");
            DrawTextF(cnt, SB_X - MeasureTextF(cnt, 10) - 4,
                ll_bot + item_gap + (pager_h - g_fonts.scale(10)) / 2,
                10, th_alpha(th.text_dim));
        }
    }
}