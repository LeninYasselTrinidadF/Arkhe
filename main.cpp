#include "app.hpp"
#include "data/msc_loader.hpp"
#include "data/mathlib_loader.hpp"
#include "data/crossref_loader.hpp"
#include "ui/bubble_view.hpp"
#include "ui/search_panel.hpp"
#include "ui/info_panel.hpp"
#include "ui/toolbar.hpp"
#include "ui/theme.hpp"
#include "ui/nine_patch.hpp"
#include "ui/skin.hpp"
#include "ui/constants.hpp"
#include "ui/overlay.hpp"   // ← sistema de bloqueo de clicks
#include "raylib.h"
#include "raymath.h"
#include <cstring>

int g_split_y = TOOLBAR_H + 640;

static std::string ensure_slash(const std::string& p) {
    if (p.empty()) return p;
    return (p.back() != '/' && p.back() != '\\') ? p + '/' : p;
}
static std::string asset_path(AppState& state, const char* rel) {
    return ensure_slash(state.toolbar.assets_path) + rel;
}

static void reload_all_assets(AppState& state,
    std::shared_ptr<MathNode>& root_msc,
    std::shared_ptr<MathNode>& root_mathlib,
    std::shared_ptr<MathNode>& root_std)
{
    root_msc = load_msc2020(asset_path(state, "msc2020_tree.json"));
    root_mathlib = load_mathlib(asset_path(state, "mathlib_layout.json"));
    root_std = nullptr;
    auto crossrefs = load_crossref(asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root = root_mathlib; state.msc_root = root_msc;
    state.standard_root = root_std;   state.crossref_map = crossrefs;

    std::string gfx = state.toolbar.graphics_path;
    if (gfx[0] == '\0') gfx = asset_path(state, "graphics");
    state.textures.set_root(gfx);
    state.textures.preload_all();

    reload_skin(ensure_slash(gfx), state.textures);

    state.nav_stack.clear(); state.cam_memory.clear();
    if (root_msc) state.push(root_msc);
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "Arkhe");
    SetTargetFPS(60);

    apply_theme(0);
    g_circle_mask.load();

    Camera2D cam = {}; cam.zoom = 1.0f;

    AppState state;
    strncpy(state.toolbar.entries_path, "assets/entries/", 511);
    strncpy(state.toolbar.graphics_path, "assets/graphics/", 511);

    auto root_msc = load_msc2020(asset_path(state, "msc2020_tree.json"));
    auto root_mathlib = load_mathlib(asset_path(state, "mathlib_layout.json"));
    std::shared_ptr<MathNode> root_std;
    auto crossrefs = load_crossref(asset_path(state, "crossref.json"));
    if (root_mathlib) inject_crossrefs(root_mathlib.get(), crossrefs);

    state.mathlib_root = root_mathlib; state.msc_root = root_msc;
    state.standard_root = root_std;   state.crossref_map = crossrefs;

    state.textures.set_root(ensure_slash(state.toolbar.graphics_path));
    state.textures.preload_all();
    g_skin.load(ensure_slash(state.toolbar.graphics_path), state.textures);

    if (root_msc) state.push(root_msc);

    auto current_root = [&]()->std::shared_ptr<MathNode> {
        switch (state.mode) {
        case ViewMode::Mathlib:  return root_mathlib;
        case ViewMode::Standard: return root_std;
        default:                 return root_msc;
        }
        };

    ViewMode prev_mode = state.mode;
    int prev_depth = (int)state.nav_stack.size();
    int prev_theme = state.toolbar.theme_id;
    bool dragging_split = false;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // ── Limpiar overlay del frame anterior ────────────────────────────────
        // Debe ser lo primero para que bubble_view/search_panel vean los rects
        // registrados en el frame previo (bloqueo con 1 frame de latencia,
        // imperceptible en práctica).
        overlay::clear();

        // Cambio de tema
        if (state.toolbar.theme_id != prev_theme) {
            apply_theme(state.toolbar.theme_id);
            prev_theme = state.toolbar.theme_id;
        }

        // Recarga de assets
        if (state.toolbar.assets_changed) {
            state.toolbar.assets_changed = false;
            reload_all_assets(state, root_msc, root_mathlib, root_std);
            prev_mode = state.mode; prev_depth = (int)state.nav_stack.size();
        }

        // Navegación diferida
        if (state.pending_nav.active) {
            state.pending_nav.active = false;
            state.mode = state.pending_nav.mode;
            state.nav_stack.clear();
            state.nav_stack.push_back(state.pending_nav.root);
            state.push(state.pending_nav.node);
            state.selected_code = state.pending_nav.code;
            state.selected_label = state.pending_nav.label;
            prev_mode = state.mode; prev_depth = (int)state.nav_stack.size();
            state.restore_cam(cam);
        }

        // Divisor horizontal
        const int SPLIT_MIN = TOOLBAR_H + 160, SPLIT_MAX = SH() - 120;
        bool near_split = mouse.y >= g_split_y - 4 && mouse.y <= g_split_y + 4;

        // El divisor NO debe reaccionar si el mouse está sobre un panel flotante
        bool split_blocked = overlay::blocks_mouse(mouse);
        SetMouseCursor((near_split && !split_blocked) || dragging_split
            ? MOUSE_CURSOR_RESIZE_NS : MOUSE_CURSOR_DEFAULT);
        if (near_split && !split_blocked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            dragging_split = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            dragging_split = false;
        if (dragging_split)
            g_split_y = Clamp((int)mouse.y, SPLIT_MIN, SPLIT_MAX);
        if (std::abs(mouse.y - g_split_y) < 40 && !split_blocked) {
            float wh = GetMouseWheelMove();
            if (wh != 0.0f)
                g_split_y = Clamp(g_split_y - (int)(wh * 30.f), SPLIT_MIN, SPLIT_MAX);
        }

        // Cambio de modo
        if (state.mode != prev_mode) {
            MathNode* old = state.current();
            state.save_cam(cam, state.cam_key_for(prev_mode, old ? old->code : "ROOT"));
            state.nav_stack.clear();
            auto root = current_root(); if (root)state.push(root);
            state.restore_cam(cam); prev_mode = state.mode;
        }

        int cur_depth = (int)state.nav_stack.size();
        if (cur_depth != prev_depth) { state.restore_cam(cam); prev_depth = cur_depth; }
        if (IsKeyPressed(KEY_ESCAPE)) { state.save_cam(cam); state.pop(); }

        const Theme& th = g_theme;
        BeginDrawing();
        ClearBackground(th.bg_app);

        // ── Orden de dibujo: fondo → canvas → paneles → flotantes ────────────
        // Los paneles flotantes (toolbar) se dibujan AL FINAL para quedar encima
        // visualmente y registrar sus rects en overlay (disponibles el frame
        // siguiente para bloquear eventos de bubble_view/search_panel).

        // 1. Canvas de burbujas (fondo, procesa clicks con guarda de overlay)
        draw_bubble_view(state, cam, mouse);
        draw_mode_switcher(state, mouse);

        // 2. Separador vertical canvas/panel lateral
        DrawLineEx({ (float)CANVAS_W(),(float)UI_TOP() },
            { (float)CANVAS_W(),(float)g_split_y },
            1.f, th.split_vline);

        // 3. Panel de búsqueda (procesa clicks con guarda de overlay)
        draw_search_panel(state, current_root().get(), mouse);

        // 4. Panel de información inferior
        draw_info_panel(state, mouse);

        // 5. Divisor horizontal (handle visual)
        bool split_active = near_split || dragging_split;
        Color sc = split_active ? th.split_active : th.split_normal;
        DrawLineEx({ 0,(float)g_split_y }, { (float)SW(),(float)g_split_y },
            dragging_split ? 2.f : 1.5f, sc);
        int hx = SW() / 2;
        DrawRectangle(hx - 20, g_split_y - 3, 40, 6, sc);
        for (int d : {-10, 0, 10})
            DrawRectangle(hx + d - 2, g_split_y - 1, 4, 2, th_alpha(th.split_dots));

        // 6. Toolbar + paneles flotantes (SIEMPRE LO ÚLTIMO → encima de todo)
        //    draw_window_frame llama overlay::push_rect() por cada panel abierto.
        draw_toolbar(state, mouse);

        DrawFPS(10, SH() - 20);
        EndDrawing();
    }

    if (state.latex_render.tex_loaded) UnloadTexture(state.latex_render.texture);
    g_circle_mask.unload();
    CloseWindow();
    return 0;
}