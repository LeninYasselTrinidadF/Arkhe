#include "search_panel.hpp"
#include "../search/loogle.hpp"
#include "raylib.h"
#include <cstring>
#include <algorithm>
#include <queue>

// BFS exacto
static std::shared_ptr<MathNode> find_node_by_code(
    const std::shared_ptr<MathNode>& root,
    const std::string& target_code)
{
    if (!root) return nullptr;
    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto node = q.front(); q.pop();
        if (node->code == target_code) return node;
        for (auto& child : node->children) q.push(child);
    }
    return nullptr;
}

// BFS por modulo: match exacto primero, luego por substring mas largo
static std::shared_ptr<MathNode> find_node_by_module(
    const std::shared_ptr<MathNode>& root,
    const std::string& module)
{
    if (!root || module.empty()) return nullptr;

    auto exact = find_node_by_code(root, module);
    if (exact) return exact;

    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    std::shared_ptr<MathNode> best = nullptr;
    int best_len = 0;

    while (!q.empty()) {
        auto node = q.front(); q.pop();
        if (!node->code.empty() &&
            module.find(node->code) != std::string::npos &&
            (int)node->code.size() > best_len)
        {
            best = node;
            best_len = (int)node->code.size();
        }
        for (auto& child : node->children) q.push(child);
    }
    return best;
}

static void draw_divider_h(int y) {
    DrawLine(CANVAS_W() + 8, y, SW() - 8, y, { 50,50,70,255 });
}

void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse) {
    int px = CANVAS_W() + 10;
    int pw = PANEL_W() - 20;
    int y = 0;

    DrawRectangle(CANVAS_W(), 0, PANEL_W(), TOP_H(), { 14,14,22,255 });
    DrawText("BUSQUEDA", px, y + 10, 13, { 140,140,170,255 });
    draw_divider_h(y + 28);
    y += 34;

    // ── Campo busqueda local ──────────────────────────────────────────────────
    DrawText("Local (fuzzy)", px, y, 11, { 100,100,140,255 });
    y += 14;
    DrawRectangle(px, y, pw, 28, { 22,22,36,255 });
    DrawRectangleLines(px, y, pw, 28, { 70,70,110,255 });
    DrawText(state.search_buf, px + 6, y + 8, 13, WHITE);
    y += 34;

    if (mouse.x > CANVAS_W()) {
        static bool local_focus = true;
        Rectangle lf = { (float)(CANVAS_W() + 10), 48.0f, (float)pw, 28.0f };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            local_focus = CheckCollisionPointRec(mouse, lf);

        if (local_focus) {
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(state.search_buf);
                if (key >= 32 && key <= 125 && len < 255) {
                    state.search_buf[len] = (char)key;
                    state.search_buf[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = (int)strlen(state.search_buf);
                if (len > 0) state.search_buf[len - 1] = '\0';
            }
        }
    }

    // Resultados fuzzy locales
    if (strlen(state.search_buf) > 0 && search_root) {
        auto hits = fuzzy_search(search_root, state.search_buf, 6);
        for (auto& hit : hits) {
            if (y + 34 > TOP_H() / 2) break;
            bool sel = (hit.node->code == state.selected_code);
            DrawRectangle(px, y, pw, 32,
                sel ? Color{ 38,70,110,255 } : Color{ 20,20,32,255 });
            DrawText(hit.node->code.c_str(), px + 6, y + 4, 11, { 110,190,255,255 });
            std::string sl = hit.node->label.substr(0, 36);
            DrawText(sl.c_str(), px + 6, y + 18, 10, { 180,180,200,255 });

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                mouse.x > px && mouse.y > y && mouse.y < y + 32) {
                state.selected_code = hit.node->code;
                state.selected_label = hit.node->label;
                if (!hit.node->children.empty()) state.push(hit.node);
            }
            y += 34;
        }
    }

    // ── Loogle ────────────────────────────────────────────────────────────────
    draw_divider_h(TOP_H() / 2);
    int ly = TOP_H() / 2 + 6;
    DrawText("Loogle (Mathlib)", px, ly, 11, { 100,100,140,255 });
    ly += 14;

    DrawRectangle(px, ly, pw - 60, 28, { 22,22,36,255 });
    DrawRectangleLines(px, ly, pw - 60, 28, { 70,70,110,255 });
    DrawText(state.loogle_buf, px + 6, ly + 8, 13, WHITE);

    Rectangle btn = { (float)(px + pw - 56), (float)ly, 56.0f, 28.0f };
    bool      btn_h = CheckCollisionPointRec(mouse, btn);
    DrawRectangleRec(btn, btn_h ? Color{ 60,100,180,255 } : Color{ 40,70,130,255 });
    DrawText("Buscar", (int)btn.x + 6, ly + 8, 12, WHITE);

    if (mouse.x > CANVAS_W()) {
        static bool loogle_focus = false;
        Rectangle lf = { (float)px, (float)ly, (float)(pw - 60), 28.0f };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            loogle_focus = CheckCollisionPointRec(mouse, lf);

        if (loogle_focus) {
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(state.loogle_buf);
                if (key >= 32 && key <= 125 && len < 255) {
                    state.loogle_buf[len] = (char)key;
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
    }

    ly += 34;

    if (state.loogle_loading.load()) {
        DrawText("Buscando...", px, ly, 12, { 120,180,120,255 });
        ly += 18;
    }
    else if (!state.loogle_error.empty()) {
        DrawText(state.loogle_error.c_str(), px, ly, 11, { 200,100,100,255 });
        ly += 16;
    }

    // ── Resultados Loogle ─────────────────────────────────────────────────────
    for (auto& r : state.loogle_results) {
        if (ly + 52 > TOP_H() - 4) break;

        bool hovered = mouse.x > px && mouse.x < px + pw &&
            mouse.y > ly && mouse.y < ly + 48;

        DrawRectangle(px, ly, pw, 48,
            hovered ? Color{ 30,55,90,255 } : Color{ 18,18,30,255 });
        DrawRectangleLinesEx({ (float)px,(float)ly,(float)pw,48.0f },
            1, hovered ? Color{ 80,130,200,255 } : Color{ 45,45,75,200 });

        std::string name = r.name.size() > 38 ? r.name.substr(0, 37) + "." : r.name;
        DrawText(name.c_str(), px + 8, ly + 5, 12, { 140,210,255,255 });
        std::string mod = r.module.size() > 44 ? r.module.substr(0, 43) + "." : r.module;
        DrawText(mod.c_str(), px + 8, ly + 22, 10, { 90,140,190,200 });
        std::string sig = r.type_sig.size() > 48 ? r.type_sig.substr(0, 47) + "." : r.type_sig;
        DrawText(sig.c_str(), px + 8, ly + 35, 10, { 140,140,170,180 });

        // FIXED: usar pending_nav para que main.cpp no pise el nav_stack al
        // detectar el cambio de modo en el mismo frame del click.
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto found = find_node_by_module(state.mathlib_root, r.module);
            if (found) {
                // Buscar la declaracion especifica dentro del archivo
                std::shared_ptr<MathNode> decl_node = nullptr;
                for (auto& child : found->children) {
                    if (child->label == r.name || child->code == r.name) {
                        decl_node = child;
                        break;
                    }
                }

                // El nodo destino es la declaracion si existe, si no el archivo
                auto dest = decl_node ? decl_node : found;

                // Construir nav_stack intermedio: raiz -> archivo -> (decl)
                // Para eso necesitamos el padre de dest si dest es una declaracion.
                // Lo resolvemos poniendo found como nodo de pending_nav y
                // dejando que main.cpp haga push(found). Si habia decl_node,
                // seleccionamos su code/label para que info_panel lo muestre.
                state.pending_nav = {
                    true,
                    ViewMode::Mathlib,
                    dest->code,
                    dest->label,
                    state.mathlib_root,
                    found       // push hasta el archivo .lean
                };

                // Si hay declaracion especifica, la marcamos como seleccionada
                // (sera visible en info_panel cuando lleguemos al archivo)
                if (decl_node) {
                    state.pending_nav.code = decl_node->code;
                    state.pending_nav.label = decl_node->label;
                }
            }
            else {
                OpenURL(("https://loogle.lean-lang.org/?q=" + r.name).c_str());
            }
        }

        ly += 52;
    }
}