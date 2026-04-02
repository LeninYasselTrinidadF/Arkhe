#include "app.hpp"
#include "data/msc_loader.hpp"
#include "data/mathlib_loader.hpp"
#include "data/crossref_loader.hpp"
#include "data/nav_state.hpp"
#include "data/position_state.hpp"

// UI Core
#include "ui/core/font_manager.hpp"
#include "ui/core/nine_patch.hpp"
#include "ui/core/skin.hpp"
#include "ui/core/theme.hpp"

// UI Info
#include "ui/info/info_panel.hpp"

// UI Panels
#include "ui/panels/search_panel.hpp"
#include "ui/panels/toolbar.hpp"

// UI Raíz / Otros
#include "ui/bubble/bubble_stats.hpp"
#include "ui/bubble_view.hpp"
#include "ui/dep_view.hpp"
#include "ui/constants.hpp" // Asumiendo que está en core
#include "ui/core/overlay.hpp"   // Asumiendo que está en core

#include "raylib.h"
#include "raymath.h"
#include <fstream>  // <--- Esta es la pieza que falta
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
    bubble_stats_clear();
    state.nav_stack.clear(); state.cam_memory.clear();
    if (root_msc) state.push(root_msc);
    // Recargar grafo de dependencias por modo (si existen archivos específicos, se usan)
    state.dep_graph_msc.load(asset_path(state, "deps.json"));
    state.dep_graph_mathlib.load(asset_path(state, "deps_mathlib.json"));
    state.dep_graph_standard.load(asset_path(state, "deps_standard.json"));

    // Cargar posiciones temporales guardadas
    position_state_load(state);
    // Actualizar alias compat
    state.dep_graph = get_dep_graph_for(state);
    state.dep_view_active = false;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "Arkhe");
    SetTargetFPS(60);
    apply_theme(0);
    g_circle_mask.load();

    // Fuente custom — fallback silencioso si no existe el .ttf
    g_fonts.load("assets/fonts/main.ttf");
    g_fonts.base_size = 30.0f;

    Camera2D cam = {}; cam.zoom = 1.0f;
    Camera2D dep_cam = {}; dep_cam.zoom = 1.0f;
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
    // Cargar grafo de dependencias por modo
    state.dep_graph_msc.load(asset_path(state, "deps.json"));
    state.dep_graph_mathlib.load(asset_path(state, "deps_mathlib.json"));
    state.dep_graph_standard.load(asset_path(state, "deps_standard.json"));

    // Cargar posiciones temporales guardadas
    position_state_load(state);
    // Compat: mantener el grafo "legacy" apuntando al actual
    state.dep_graph = get_dep_graph_for(state);

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
        overlay::clear();

        if (state.toolbar.theme_id != prev_theme) {
            apply_theme(state.toolbar.theme_id);
            prev_theme = state.toolbar.theme_id;
        }
        if (state.toolbar.assets_changed) {
            state.toolbar.assets_changed = false;
            reload_all_assets(state, root_msc, root_mathlib, root_std);
            // Al recargar assets el árbol cambia → borrar estado guardado
            // para no intentar restaurar nodos que ya no existen
            { std::ofstream f(NAV_STATE_FILE); if (f.is_open()) f << "{}"; }
            prev_mode = state.mode; prev_depth = (int)state.nav_stack.size();
        }
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

        const int SPLIT_MIN = TOOLBAR_H + 160, SPLIT_MAX = SH() - 120;
        bool near_split = mouse.y >= g_split_y - 4 && mouse.y <= g_split_y + 4;
        bool split_blocked = overlay::blocks_mouse(mouse);
        SetMouseCursor((near_split && !split_blocked) || dragging_split
            ? MOUSE_CURSOR_RESIZE_NS : MOUSE_CURSOR_DEFAULT);
        if (near_split && !split_blocked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) dragging_split = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging_split = false;
        if (dragging_split) g_split_y = Clamp((int)mouse.y, SPLIT_MIN, SPLIT_MAX);
        if (std::abs(mouse.y - g_split_y) < 40 && !split_blocked) {
            float wh = GetMouseWheelMove();
            if (wh != 0.0f) g_split_y = Clamp(g_split_y - (int)(wh * 30.f), SPLIT_MIN, SPLIT_MAX);
        }

        if (state.mode != prev_mode) {
            // Guardar posición del modo que se abandona
            nav_state_save(state, prev_mode);
            MathNode* old = state.current();
            state.save_cam(cam, state.cam_key_for(prev_mode, old ? old->code : "ROOT"));
            // Preparar el nuevo modo
            state.nav_stack.clear();
            auto root = current_root(); if (root)state.push(root);
            // Restaurar posición guardada del nuevo modo (si existe)
            nav_state_load(state, state.mode, current_root());
            state.restore_cam(cam); prev_mode = state.mode;
            // actualizar alias compat al cambiar modo
            state.dep_graph = get_dep_graph_for(state);
        }
        int cur_depth = (int)state.nav_stack.size();
        if (cur_depth != prev_depth) { state.restore_cam(cam); prev_depth = cur_depth; }
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (state.dep_view_active) {
                // ESC en vista de dependencias → volver al modo burbuja anterior
                state.dep_view_active = false;
            }
            else {
                state.save_cam(cam);
                state.pop();
            }
        }

        const Theme& th = g_theme;
        BeginDrawing();
        ClearBackground(th.bg_app);

        if (state.dep_view_active) {
            draw_dep_view(state, dep_cam, mouse);
        }
        else {
            draw_bubble_view(state, cam, mouse);
        }
        if (!state.dep_view_active)
            draw_mode_switcher(state, mouse);   // solo en bubble_view
        DrawLineEx({ (float)CANVAS_W(),(float)UI_TOP() }, { (float)CANVAS_W(),(float)g_split_y }, 1.f, th.split_vline);
        draw_search_panel(state, current_root().get(), mouse);
        draw_info_panel(state, mouse);

        bool split_active = near_split || dragging_split;
        Color sc = split_active ? th.split_active : th.split_normal;
        DrawLineEx({ 0,(float)g_split_y }, { (float)SW(),(float)g_split_y }, dragging_split ? 2.f : 1.5f, sc);
        int hx = SW() / 2;
        DrawRectangle(hx - 20, g_split_y - 3, 40, 6, sc);
        for (int d : {-10, 0, 10}) DrawRectangle(hx + d - 2, g_split_y - 1, 4, 2, th_alpha(th.split_dots));

        draw_toolbar(state, mouse);   // siempre al final → encima de todo

        DrawFPS(10, SH() - 20);
        EndDrawing();
    }

    if (state.latex_render.tex_loaded) UnloadTexture(state.latex_render.texture);
    g_fonts.unload();
    g_circle_mask.unload();
    CloseWindow();
    return 0;
}