#include "app.hpp"
#include "data/msc_loader.hpp"
#include "data/mathlib_loader.hpp"
#include "ui/bubble_view.hpp"
#include "ui/search_panel.hpp"
#include "ui/info_panel.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "raymath.h"

// Definicion del divisor global
int g_split_y = 660;

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "Arkhe");
    SetTargetFPS(60);

    Camera2D cam = {};
    cam.zoom = 1.0f;

    auto root_msc = load_msc2020("assets/msc2020_tree.json");
    auto root_mathlib = load_mathlib("assets/mathlib_layout.json");
    std::shared_ptr<MathNode> root_std;

    AppState state;
    if (root_msc) state.push(root_msc);

    auto current_root = [&]() -> std::shared_ptr<MathNode> {
        switch (state.mode) {
        case ViewMode::Mathlib:  return root_mathlib;
        case ViewMode::Standard: return root_std;
        default:                 return root_msc;
        }
        };

    ViewMode prev_mode = state.mode;
    int      prev_depth = (int)state.nav_stack.size();
    bool     dragging_split = false;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // ?? Ajuste del divisor ????????????????????????????????????????????????
        // Zona de arrastre: banda de 8px alrededor del divisor
        bool near_split = mouse.y >= g_split_y - 4 && mouse.y <= g_split_y + 4;

        if (near_split || dragging_split)
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (near_split && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            dragging_split = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            dragging_split = false;

        if (dragging_split) {
            g_split_y = (int)mouse.y;
            // Limites: al menos 160px para los paneles superiores,
            // al menos 120px para el panel inferior
            g_split_y = Clamp(g_split_y, 160, SH() - 120);
        }

        // Scroll con la rueda en la banda del divisor mueve el divisor
        if (near_split || (mouse.y > g_split_y - 40 && mouse.y < g_split_y + 40)) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                g_split_y -= (int)(wheel * 30.0f);
                g_split_y = Clamp(g_split_y, 160, SH() - 120);
            }
        }

        // Scroll en cualquier zona: si el mouse esta en el canvas o el panel
        // derecho, mover el divisor con la rueda (solo si NO esta en el canvas)
        if (mouse.y < g_split_y && mouse.x > CANVAS_W()) {
            // panel de busqueda: la rueda no mueve el split
        }

        if (state.mode != prev_mode) {
            state.nav_stack.clear();
            auto root = current_root();
            if (root) state.push(root);
            prev_mode = state.mode;
        }

        int cur_depth = (int)state.nav_stack.size();
        if (cur_depth != prev_depth) {
            cam.target = { 0,0 };
            cam.zoom = 1.0f;
            prev_depth = cur_depth;
        }

        if (IsKeyPressed(KEY_ESCAPE)) state.pop();

        BeginDrawing();
        ClearBackground({ 11,11,18,255 });

        draw_bubble_view(state, cam, mouse);
        draw_mode_switcher(state, mouse);

        DrawLineEx({ (float)CANVAS_W(), 0 },
            { (float)CANVAS_W(), (float)TOP_H() },
            1.0f, { 50,50,70,255 });

        draw_search_panel(state, current_root().get(), mouse);
        draw_info_panel(state, mouse);

        // Linea divisora con handle visual
        Color split_col = (near_split || dragging_split)
            ? Color{ 100,120,200,255 } : Color{ 45,45,70,255 };
        DrawLineEx({ 0,(float)g_split_y }, { (float)SW(),(float)g_split_y },
            dragging_split ? 2.0f : 1.5f, split_col);

        // Handle central (indicador de que es arrastrable)
        int hx = SW() / 2;
        DrawRectangle(hx - 20, g_split_y - 3, 40, 6, split_col);
        DrawRectangle(hx - 12, g_split_y - 1, 4, 2, { 180,180,220,180 });
        DrawRectangle(hx - 2, g_split_y - 1, 4, 2, { 180,180,220,180 });
        DrawRectangle(hx + 8, g_split_y - 1, 4, 2, { 180,180,220,180 });

        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}