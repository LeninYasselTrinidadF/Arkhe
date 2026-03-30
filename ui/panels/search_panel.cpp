#include "search_panel.hpp"
#include "../core/font_manager.hpp"
#include "../core/overlay.hpp"   // ← guarda de clicks
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

void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse) {
    const Theme& th = g_theme;
    int px = CANVAS_W() + 10, pw = PANEL_W() - 20;

    int panel_top = UI_TOP();
    if (g_skin.panel.valid()) g_skin.panel.draw((float)CANVAS_W(), (float)panel_top, (float)PANEL_W(), (float)TOP_H(), th.bg_panel);
    else DrawRectangle(CANVAS_W(), panel_top, PANEL_W(), TOP_H(), th.bg_panel);

    int y = panel_top;
    DrawTextF("BUSQUEDA", px, y + 10, 13, th_alpha(th.text_dim));
    draw_divider_h(y + 28); y += 34;

    // ── Búsqueda local ────────────────────────────────────────────────────────
    DrawTextF("Local (fuzzy)", px, y, 11, th_alpha(th.text_dim)); y += 14;
    draw_search_field(px, y, pw, 28, state.search_buf); y += 34;

    // Input del panel solo si el mouse está en la zona del panel Y no hay overlay
    bool panel_active = mouse.x > CANVAS_W() && !overlay::blocks_mouse(mouse);

    if (panel_active) {
        static bool local_focus = true;
        Rectangle lf = { (float)px,(float)(panel_top + 34 + 14),(float)pw,28.0f };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) local_focus = CheckCollisionPointRec(mouse, lf);
        if (local_focus) {
            int key = GetCharPressed();
            while (key > 0) { int len = (int)strlen(state.search_buf); if (key >= 32 && key <= 125 && len < 255) { state.search_buf[len] = (char)key; state.search_buf[len + 1] = '\0'; }key = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE)) { int len = (int)strlen(state.search_buf); if (len > 0)state.search_buf[len - 1] = '\0'; }
        }
    }

    if (strlen(state.search_buf) > 0 && search_root) {
        auto hits = fuzzy_search(search_root, state.search_buf, 6);
        for (auto& hit : hits) {
            if (y + 34 > panel_top + TOP_H() / 2) break;
            bool sel = (hit.node->code == state.selected_code);
            bool hov = panel_active && mouse.x > px && mouse.x<px + pw && mouse.y>y && mouse.y < y + 32;
            draw_result_card(px, y, pw, 32, hov, sel);
            DrawTextF(hit.node->code.c_str(), px + 6, y + 4, 11, th.text_code);
            std::string sl = hit.node->label.substr(0, 36);
            DrawTextF(sl.c_str(), px + 6, y + 18, 10, th_alpha(th.text_secondary));
            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state.selected_code = hit.node->code; state.selected_label = hit.node->label;
                if (!hit.node->children.empty())state.push(hit.node);
            }
            y += 34;
        }
    }

    // ── Loogle ────────────────────────────────────────────────────────────────
    int loogle_half = panel_top + TOP_H() / 2;
    draw_divider_h(loogle_half);
    int ly = loogle_half + 6;
    DrawTextF("Loogle (Mathlib)", px, ly, 11, th_alpha(th.text_dim)); ly += 14;

    draw_search_field(px, ly, pw - 60, 28, state.loogle_buf);

    Rectangle btn = { (float)(px + pw - 56),(float)ly,56.0f,28.0f };
    bool btn_h = panel_active && CheckCollisionPointRec(mouse, btn);
    draw_search_button("Buscar", (int)btn.x, (int)btn.y, 56, 28, btn_h);

    if (panel_active) {
        static bool loogle_focus = false;
        Rectangle lf = { (float)px,(float)ly,(float)(pw - 60),28.0f };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) loogle_focus = CheckCollisionPointRec(mouse, lf);
        if (loogle_focus) {
            int key = GetCharPressed();
            while (key > 0) { int len = (int)strlen(state.loogle_buf); if (key >= 32 && key <= 125 && len < 255) { state.loogle_buf[len] = (char)key; state.loogle_buf[len + 1] = '\0'; }key = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE)) { int len = (int)strlen(state.loogle_buf); if (len > 0)state.loogle_buf[len - 1] = '\0'; }
            if (IsKeyPressed(KEY_ENTER) && strlen(state.loogle_buf) > 0) loogle_search_async(state, state.loogle_buf);
        }
        if (btn_h && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && strlen(state.loogle_buf) > 0) loogle_search_async(state, state.loogle_buf);
    }
    ly += 34;

    if (state.loogle_loading.load()) { DrawTextF("Buscando...", px, ly, 12, th.success); ly += 18; }
    else if (!state.loogle_error.empty()) { DrawTextF(state.loogle_error.c_str(), px, ly, 11, { 200,100,100,255 }); ly += 16; }

    // Resultados Loogle
    for (auto& r : state.loogle_results) {
        if (ly + 52 > panel_top + TOP_H() - 4) break;
        bool hov = panel_active && mouse.x > px && mouse.x<px + pw && mouse.y>ly && mouse.y < ly + 48;
        draw_result_card(px, ly, pw, 48, hov, false);
        std::string name = r.name.size() > 38 ? r.name.substr(0, 37) + "." : r.name;
        DrawTextF(name.c_str(), px + 8, ly + 5, 12, th.text_code);
        std::string mod = r.module.size() > 44 ? r.module.substr(0, 43) + "." : r.module;
        DrawTextF(mod.c_str(), px + 8, ly + 22, 10, th_alpha(th.text_secondary));
        std::string sig = r.type_sig.size() > 48 ? r.type_sig.substr(0, 47) + "." : r.type_sig;
        DrawTextF(sig.c_str(), px + 8, ly + 35, 10, th_alpha(th.text_dim));

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto found = find_node_by_module(state.mathlib_root, r.module);
            if (found) {
                std::shared_ptr<MathNode> decl = nullptr;
                for (auto& child : found->children)if (child->label == r.name || child->code == r.name) { decl = child; break; }
                auto dest = decl ? decl : found;
                state.pending_nav = { true,ViewMode::Mathlib,dest->code,dest->label,state.mathlib_root,found };
                if (decl) { state.pending_nav.code = decl->code; state.pending_nav.label = decl->label; }
            }
            else {
                OpenURL(("https://loogle.lean-lang.org/?q=" + r.name).c_str());
            }
        }
        ly += 52;
    }
}