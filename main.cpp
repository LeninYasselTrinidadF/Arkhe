#include "app.hpp"
#include "data/msc_loader.hpp"
#include "data/mathlib_loader.hpp"
#include "data/crossref_loader.hpp"
#include "ui/bubble_view.hpp"
#include "ui/search_panel.hpp"
#include "ui/info_panel.hpp"
#include "ui/toolbar.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstring>

int g_split_y = 660;

// ── Helpers de recarga de assets ──────────────────────────────────────────────
// Reconstruye la ruta de un asset dado el directorio raiz configurado.
static std::string asset_path(AppState& state, const char* rel) {
    std::string base = state.toolbar.assets_path;
    if (!base.empty() && base.back() != '/' && base.back() != '\\')
        base += '/';
    return base + rel;
}

static void reload_all_assets(AppState& state,
    std::shared_ptr<MathNode>& root_msc,
    std::shared_ptr<MathNode>& root_mathlib,
    std::shared_ptr<MathNode>& root_std)
{
    // Recargar datos
    root_msc     = load_msc2020(asset_path(state, "msc2020_tree.json"));
    root_mathlib = load_mathlib(asset_path(state, "mathlib_layout.json"));
    root_std     = nullptr;   // (sin implementacion todavia)

    auto crossrefs = load_crossref(asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root  = root_mathlib;
    state.msc_root      = root_msc;
    state.standard_root = root_std;
    state.crossref_map  = crossrefs;

    // Recargar texturas
    state.textures.set_root(asset_path(state, "graphics"));
    state.textures.preload_all();

    // Reiniciar navegacion
    state.nav_stack.clear();
    state.cam_memory.clear();
    if (root_msc) state.push(root_msc);

    TraceLog(LOG_INFO, "Assets recargados desde: %s", state.toolbar.assets_path);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "Arkhe");
    SetTargetFPS(60);

    Camera2D cam = {};
    cam.zoom = 1.0f;

    AppState state;

    // ── Carga inicial ─────────────────────────────────────────────────────────
    // La ruta base de assets viene del toolbar (por defecto "assets")
    auto root_msc     = load_msc2020(asset_path(state, "msc2020_tree.json"));
    auto root_mathlib = load_mathlib(asset_path(state, "mathlib_layout.json"));
    std::shared_ptr<MathNode> root_std;

    auto crossrefs = load_crossref(asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root  = root_mathlib;
    state.msc_root      = root_msc;
    state.standard_root = root_std;
    state.crossref_map  = crossrefs;

    // Precargar texturas desde assets/graphics/
    state.textures.set_root(asset_path(state, "graphics"));
    state.textures.preload_all();

    if (root_msc) state.push(root_msc);

    auto current_root = [&]() -> std::shared_ptr<MathNode> {
        switch (state.mode) {
        case ViewMode::Mathlib:  return root_mathlib;
        case ViewMode::Standard: return root_std;
        default:                 return root_msc;
        }
    };

    ViewMode prev_mode  = state.mode;
    int      prev_depth = (int)state.nav_stack.size();
    bool     dragging_split = false;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // ── Recarga de assets si el toolbar lo solicito ───────────────────────
        if (state.toolbar.assets_changed) {
            state.toolbar.assets_changed = false;
            reload_all_assets(state, root_msc, root_mathlib, root_std);
            prev_mode  = state.mode;
            prev_depth = (int)state.nav_stack.size();
        }

        // ── Consumir navegacion diferida ──────────────────────────────────────
        if (state.pending_nav.active) {
            state.pending_nav.active = false;
            state.mode = state.pending_nav.mode;
            state.nav_stack.clear();
            state.nav_stack.push_back(state.pending_nav.root);
            state.push(state.pending_nav.node);
            state.selected_code  = state.pending_nav.code;
            state.selected_label = state.pending_nav.label;
            prev_mode  = state.mode;
            prev_depth = (int)state.nav_stack.size();
            state.restore_cam(cam);
        }

        // ── Divisor arrastrable ───────────────────────────────────────────────
        bool near_split = mouse.y >= g_split_y - 4 && mouse.y <= g_split_y + 4;
        SetMouseCursor((near_split || dragging_split)
            ? MOUSE_CURSOR_RESIZE_NS : MOUSE_CURSOR_DEFAULT);
        if (near_split && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) dragging_split = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))               dragging_split = false;
        if (dragging_split)
            g_split_y = Clamp((int)mouse.y, 160, SH() - 120);

        if (std::abs(mouse.y - g_split_y) < 40) {
            float wh = GetMouseWheelMove();
            if (wh != 0.0f)
                g_split_y = Clamp(g_split_y - (int)(wh * 30.f), 160, SH() - 120);
        }

        // ── Cambio de modo ────────────────────────────────────────────────────
        if (state.mode != prev_mode) {
            MathNode* old_node = state.current();
            std::string old_key = state.cam_key_for(
                prev_mode, old_node ? old_node->code : "ROOT");
            state.save_cam(cam, old_key);

            state.nav_stack.clear();
            auto root = current_root();
            if (root) state.push(root);

            state.restore_cam(cam);
            prev_mode = state.mode;
        }

        // ── Restaurar camara al cambiar profundidad ───────────────────────────
        int cur_depth = (int)state.nav_stack.size();
        if (cur_depth != prev_depth) {
            state.restore_cam(cam);
            prev_depth = cur_depth;
        }

        // ESC: volver un nivel
        if (IsKeyPressed(KEY_ESCAPE)) {
            state.save_cam(cam);
            state.pop();
        }

        BeginDrawing();
        ClearBackground({ 11, 11, 18, 255 });

        draw_bubble_view(state, cam, mouse);
        draw_mode_switcher(state, mouse);

        DrawLineEx({ (float)CANVAS_W(), 0 }, { (float)CANVAS_W(), (float)TOP_H() },
            1.f, { 50, 50, 70, 255 });

        draw_search_panel(state, current_root().get(), mouse);
        draw_info_panel(state, mouse);

        // Linea divisora
        Color sc = (near_split || dragging_split)
            ? Color{ 100, 120, 200, 255 } : Color{ 45, 45, 70, 255 };
        DrawLineEx({ 0, (float)g_split_y }, { (float)SW(), (float)g_split_y },
            dragging_split ? 2.f : 1.5f, sc);
        int hx = SW() / 2;
        DrawRectangle(hx - 20, g_split_y - 3, 40, 6, sc);
        for (int d : {-10, 0, 10})
            DrawRectangle(hx + d - 2, g_split_y - 1, 4, 2, { 180, 180, 220, 160 });

        // ── Toolbar (se dibuja encima de todo) ────────────────────────────────
        draw_toolbar(state, mouse);

        // ── Paneles flotantes del toolbar ─────────────────────────────────────
        draw_docs_panel(state, mouse);
        draw_entry_editor(state, mouse);

        DrawFPS(10, SH() - 20);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
