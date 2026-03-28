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

// g_split_y: coordenada Y absoluta del divisor (>= TOOLBAR_H).
int g_split_y = TOOLBAR_H + 640;

// ── Helpers de rutas ──────────────────────────────────────────────────────────

static std::string ensure_slash(const std::string& p) {
    if (p.empty()) return p;
    if (p.back() != '/' && p.back() != '\\') return p + '/';
    return p;
}

static std::string asset_path(AppState& state, const char* rel) {
    return ensure_slash(state.toolbar.assets_path) + rel;
}

// ── Recarga de assets ─────────────────────────────────────────────────────────

static void reload_all_assets(AppState& state,
    std::shared_ptr<MathNode>& root_msc,
    std::shared_ptr<MathNode>& root_mathlib,
    std::shared_ptr<MathNode>& root_std)
{
    root_msc     = load_msc2020    (asset_path(state, "msc2020_tree.json"));
    root_mathlib = load_mathlib    (asset_path(state, "mathlib_layout.json"));
    root_std     = nullptr;

    auto crossrefs = load_crossref (asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root  = root_mathlib;
    state.msc_root      = root_msc;
    state.standard_root = root_std;
    state.crossref_map  = crossrefs;

    // Las graficas usan graphics_path si esta configurado; si no, assets/graphics/
    std::string gfx = state.toolbar.graphics_path;
    if (gfx[0] == '\0')
        gfx = asset_path(state, "graphics");
    state.textures.set_root(gfx);
    state.textures.preload_all();

    state.nav_stack.clear();
    state.cam_memory.clear();
    if (root_msc) state.push(root_msc);

    TraceLog(LOG_INFO, "Assets recargados desde: %s | entries: %s | graphics: %s",
        state.toolbar.assets_path,
        state.toolbar.entries_path,
        state.toolbar.graphics_path);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "Arkhe");
    SetTargetFPS(60);

    Camera2D cam = {};
    cam.zoom = 1.0f;

    AppState state;

    // Valores iniciales: entries y graphics como sub-rutas de assets/
    strncpy(state.toolbar.entries_path,  "assets/entries/",  511);
    strncpy(state.toolbar.graphics_path, "assets/graphics/", 511);

    auto root_msc     = load_msc2020  (asset_path(state, "msc2020_tree.json"));
    auto root_mathlib = load_mathlib  (asset_path(state, "mathlib_layout.json"));
    std::shared_ptr<MathNode> root_std;

    auto crossrefs = load_crossref(asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root  = root_mathlib;
    state.msc_root      = root_msc;
    state.standard_root = root_std;
    state.crossref_map  = crossrefs;

    state.textures.set_root(ensure_slash(state.toolbar.graphics_path));
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

        // ── Recarga de assets ─────────────────────────────────────────────────
        if (state.toolbar.assets_changed) {
            state.toolbar.assets_changed = false;
            reload_all_assets(state, root_msc, root_mathlib, root_std);
            prev_mode  = state.mode;
            prev_depth = (int)state.nav_stack.size();
        }

        // ── Navegacion diferida ───────────────────────────────────────────────
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
        const int SPLIT_MIN = TOOLBAR_H + 160;
        const int SPLIT_MAX = SH() - 120;

        bool near_split = mouse.y >= g_split_y - 4 && mouse.y <= g_split_y + 4;
        SetMouseCursor((near_split || dragging_split)
            ? MOUSE_CURSOR_RESIZE_NS : MOUSE_CURSOR_DEFAULT);
        if (near_split && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) dragging_split = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))               dragging_split = false;
        if (dragging_split)
            g_split_y = Clamp((int)mouse.y, SPLIT_MIN, SPLIT_MAX);

        if (std::abs(mouse.y - g_split_y) < 40) {
            float wh = GetMouseWheelMove();
            if (wh != 0.0f)
                g_split_y = Clamp(g_split_y - (int)(wh * 30.f), SPLIT_MIN, SPLIT_MAX);
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

        if (IsKeyPressed(KEY_ESCAPE)) {
            state.save_cam(cam);
            state.pop();
        }

        BeginDrawing();
        ClearBackground({ 11, 11, 18, 255 });

        // ── 1. TOOLBAR (y = 0) ────────────────────────────────────────────────
        draw_toolbar(state, mouse);

        // ── 2. Zona de burbujas (y >= TOOLBAR_H) ──────────────────────────────
        draw_bubble_view(state, cam, mouse);
        draw_mode_switcher(state, mouse);

        DrawLineEx(
            { (float)CANVAS_W(), (float)UI_TOP() },
            { (float)CANVAS_W(), (float)g_split_y },
            1.f, { 50, 50, 70, 255 });

        draw_search_panel(state, current_root().get(), mouse);
        draw_info_panel(state, mouse);

        // ── 3. Divisor horizontal ─────────────────────────────────────────────
        Color sc = (near_split || dragging_split)
            ? Color{ 100, 120, 200, 255 } : Color{ 45, 45, 70, 255 };
        DrawLineEx({ 0, (float)g_split_y }, { (float)SW(), (float)g_split_y },
            dragging_split ? 2.f : 1.5f, sc);
        int hx = SW() / 2;
        DrawRectangle(hx - 20, g_split_y - 3, 40, 6, sc);
        for (int d : {-10, 0, 10})
            DrawRectangle(hx + d - 2, g_split_y - 1, 4, 2, { 180, 180, 220, 160 });

        // ── 4. Paneles flotantes ──────────────────────────────────────────────
        // Orden: ubicaciones primero (alineado izquierda), luego docs y editor
        draw_ubicaciones_panel(state, mouse);
        draw_docs_panel(state, mouse);
        draw_entry_editor(state, mouse);

        DrawFPS(10, SH() - 20);
        EndDrawing();
    }

    // Limpiar textura LaTeX si existe
    if (state.latex_render.tex_loaded)
        UnloadTexture(state.latex_render.texture);

    CloseWindow();
    return 0;
}
