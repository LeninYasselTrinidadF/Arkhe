#include "search_panel.hpp"
#include "../search/loogle.hpp"
#include "raylib.h"
#include <cstring>
#include <algorithm>

static int focus = 1;

static void draw_divider_h(int y) {
    DrawLine(CANVAS_W() + 8, y, SW() - 8, y, { 50, 50, 70, 255 });
}
static void draw_label(const char* t, int x, int y) {
    DrawText(t, x, y, 11, { 110, 110, 150, 255 });
}

void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse) {
    int px = CANVAS_W() + 10;
    int pw = PANEL_W() - 20;
    int y  = 0;

    DrawRectangle(CANVAS_W(), 0, PANEL_W(), TOP_H(), { 13, 13, 21, 255 });

    DrawText("BUSQUEDA", px, y + 8, 12, { 130, 130, 165, 255 });
    draw_divider_h(y + 24);
    y += 30;

    draw_label("Local (fuzzy)", px, y);
    y += 14;

    bool local_hov = CheckCollisionPointRec(mouse, {(float)px,(float)y,(float)pw,26.0f});
    DrawRectangle(px, y, pw, 26, { 20, 20, 34, 255 });
    DrawRectangleLines(px, y, pw, 26,
        focus == 1 ? Color{100,140,220,255} : Color{60,60,100,255});
    DrawText(state.search_buf, px + 6, y + 7, 13, WHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && local_hov) focus = 1;
    y += 30;

    if (focus == 1) {
        int key = GetCharPressed();
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
    }

    if (strlen(state.search_buf) > 0 && search_root) {
        auto hits = fuzzy_search(search_root, state.search_buf, 5);
        for (auto& hit : hits) {
            if (y + 30 > TOP_H() / 2) break;
            bool sel = (hit.node->code == state.selected_code);
            DrawRectangle(px, y, pw, 28,
                sel ? Color{35,65,105,255} : Color{18,18,30,255});
            DrawText(hit.node->code.c_str(),               px+5, y+3,  11, {100,185,255,255});
            DrawText(hit.node->label.substr(0,38).c_str(), px+5, y+16, 10, {170,170,195,255});
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                mouse.x > px && mouse.y > y && mouse.y < y+28) {
                state.selected_code  = hit.node->code;
                state.selected_label = hit.node->label;
                if (!hit.node->children.empty()) state.push(hit.node);
            }
            y += 30;
        }
    }

    draw_divider_h(TOP_H() / 2);
    y = TOP_H() / 2 + 6;

    draw_label("Loogle - Mathlib", px, y);
    y += 14;

    int field_w = pw - 62;
    bool loogle_hov = CheckCollisionPointRec(mouse,
        {(float)px, (float)y, (float)field_w, 26.0f});
    DrawRectangle(px, y, field_w, 26, { 20, 20, 34, 255 });
    DrawRectangleLines(px, y, field_w, 26,
        focus == 2 ? Color{100,140,220,255} : Color{60,60,100,255});
    DrawText(state.loogle_buf, px + 6, y + 7, 13, WHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && loogle_hov) focus = 2;

    Rectangle btn = {(float)(px + field_w + 4), (float)y, 56.0f, 26.0f};
    bool btn_h = CheckCollisionPointRec(mouse, btn);
    DrawRectangleRec(btn, btn_h ? Color{55,95,175,255} : Color{38,65,125,255});
    DrawText("Buscar", (int)btn.x + 5, y + 7, 11, WHITE);
    y += 30;

    if (focus == 2) {
        int key = GetCharPressed();
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
        if (IsKeyPressed(KEY_ENTER) && strlen(state.loogle_buf) > 0)
            loogle_search_async(state, state.loogle_buf);
    }
    if (btn_h && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && strlen(state.loogle_buf) > 0)
        loogle_search_async(state, state.loogle_buf);

    if (state.loogle_loading.load()) {
        DrawText("Buscando...", px, y, 11, {100,200,120,255});
        y += 16;
    } else if (!state.loogle_error.empty()) {
        DrawText(state.loogle_error.c_str(), px, y, 10, {190,90,90,255});
        y += 14;
    }

    for (auto& r : state.loogle_results) {
        if (y + 40 > TOP_H() - 4) break;
        DrawRectangle(px, y, pw, 38, {16,16,28,255});
        DrawRectangleLinesEx({(float)px,(float)y,(float)pw,38.0f}, 1, {45,45,75,200});
        DrawText(r.name.c_str(),                  px+5, y+3,  11, {130,205,255,255});
        DrawText(r.module.substr(0,44).c_str(),   px+5, y+18, 10, {90,130,170,200});
        DrawText(r.type_sig.substr(0,48).c_str(), px+5, y+29,  9, {140,140,165,180});
        y += 42;
    }
}
