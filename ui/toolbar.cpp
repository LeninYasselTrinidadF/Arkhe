#include "toolbar.hpp"
#include "ubicaciones_panel.hpp"
#include "docs_panel.hpp"
#include "entry_editor.hpp"
#include "constants.hpp"
#include "raylib.h"

// ── Helpers locales ───────────────────────────────────────────────────────────

static void draw_separator(int x) {
    DrawLineEx({ (float)x, 5.0f }, { (float)x, (float)(TOOLBAR_H - 5) }, 1.0f, { 50, 50, 75, 180 });
}

static bool draw_tab_button(const char* label, int x, int y, int w, int h,
    bool active, Vector2 mouse)
{
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color bg = active ? Color{ 30,30,50,255 } : hov ? Color{ 35,35,58,255 } : Color{ 0,0,0,0 };
    DrawRectangleRec(r, bg);
    if (active)   DrawRectangle(x, y + h - 2, w, 2, { 70, 130, 255, 255 });
    else if (hov) DrawRectangle(x, y + h - 2, w, 2, { 80,  80, 120, 200 });
    int tw = MeasureText(label, 12);
    DrawText(label, x + (w - tw) / 2, y + (h - 12) / 2 - 1, 12,
        (active || hov) ? WHITE : Color{ 170, 170, 200, 200 });
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── Instancias de paneles (singleton por sesion) ──────────────────────────────

static UbicacionesPanel* s_ubicaciones = nullptr;
static DocsPanel*        s_docs        = nullptr;
static EntryEditor*      s_editor      = nullptr;

static void ensure_panels(AppState& state) {
    if (!s_ubicaciones) s_ubicaciones = new UbicacionesPanel(state);
    if (!s_docs)        s_docs        = new DocsPanel(state);
    if (!s_editor)      s_editor      = new EntryEditor(state);
}

// ── draw_toolbar ──────────────────────────────────────────────────────────────

bool draw_toolbar(AppState& state, Vector2 mouse) {
    ensure_panels(state);

    int w = SW();
    DrawRectangle(0, 0, w, TOOLBAR_H, { 15, 15, 25, 255 });
    DrawLineEx({ 0.0f, (float)TOOLBAR_H }, { (float)w, (float)TOOLBAR_H }, 1.0f, { 40, 40, 65, 255 });

    int x = 10;
    DrawText("ARKHE", x, (TOOLBAR_H - 16) / 2, 16, { 100, 140, 255, 255 });
    x += MeasureText("ARKHE", 16) + 14;
    draw_separator(x); x += 10;

    struct Tab { const char* label; ToolbarTab tab; bool* flag; };
    Tab tabs[] = {
        { "Ubicaciones",   ToolbarTab::Ubicaciones, &state.toolbar.ubicaciones_open },
        { "Documentacion", ToolbarTab::Docs,        &state.toolbar.docs_open        },
        { "Editor",        ToolbarTab::Editor,      &state.toolbar.editor_open      },
    };
    for (auto& t : tabs) {
        int bw = MeasureText(t.label, 12) + 24;
        bool active = (state.toolbar.active_tab == t.tab) && *t.flag;
        if (draw_tab_button(t.label, x, 0, bw, TOOLBAR_H, active, mouse)) {
            bool was_open = *t.flag;
            state.toolbar.ubicaciones_open = state.toolbar.docs_open = state.toolbar.editor_open = false;
            if (!was_open) { *t.flag = true; state.toolbar.active_tab = t.tab; }
            else             state.toolbar.active_tab = ToolbarTab::None;
        }
        x += bw + 2;
    }
    draw_separator(x); x += 10;

    auto loaded = state.textures.loaded_keys();
    char tex_info[48]; snprintf(tex_info, sizeof(tex_info), "%d img", (int)loaded.size());
    DrawText(tex_info, x, (TOOLBAR_H - 10) / 2, 10, { 70, 120, 70, 200 });
    x += MeasureText(tex_info, 10) + 12;
    draw_separator(x); x += 10;

    const char* mode_str = state.mode == ViewMode::Mathlib  ? "Mathlib"  :
                           state.mode == ViewMode::Standard ? "Estandar" : "MSC2020";
    DrawText(mode_str, x, (TOOLBAR_H - 11) / 2, 11, { 100, 180, 100, 200 });

    std::string info = state.toolbar.assets_path;
    if ((int)info.size() > 40) info = ".." + info.substr(info.size() - 38);
    DrawText(info.c_str(), w - MeasureText(info.c_str(), 10) - 12,
        (TOOLBAR_H - 10) / 2, 10, { 60, 65, 100, 180 });

    // Delegar a los paneles hijos
    s_ubicaciones->draw(mouse);
    s_docs->draw(mouse);
    s_editor->draw(mouse);

    return state.toolbar.assets_changed;
}
