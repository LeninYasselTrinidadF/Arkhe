#include "toolbar.hpp"
#include "ubicaciones_panel.hpp"
#include "docs_panel.hpp"
#include "entry_editor.hpp"
#include "font_panel.hpp"      // ← nuevo
#include "font_manager.hpp"    // ← para DrawTextF en toolbar
#include "theme.hpp"
#include "nine_patch.hpp"
#include "skin.hpp"
#include "constants.hpp"
#include "raylib.h"

static void draw_separator(int x) {
    DrawLineEx({(float)x,5.0f},{(float)x,(float)(TOOLBAR_H-5)},1.0f,th_alpha(g_theme.ctrl_border));
}

static bool draw_tab_button(const char* label, int x, int y, int w, int h,
    bool active, Vector2 mouse)
{
    const Theme& th = g_theme;
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    bool hov = CheckCollisionPointRec(mouse, r);
    Color bg = active ? th.bg_button_hover : hov ? th_alpha(th.bg_button) : Color{0,0,0,0};

    if (g_skin.button.valid() && (active || hov))
        g_skin.button.draw(r, bg);
    else
        DrawRectangleRec(r, bg);

    if (active)      DrawRectangle(x, y+h-2, w, 2, th.border_active);
    else if (hov)    DrawRectangle(x, y+h-2, w, 2, th_alpha(th.accent_dim));

    int tw = MeasureTextF(label, 12);
    DrawTextF(label, x+(w-tw)/2, y+(h-12)/2-1, 12,
              (active || hov) ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool draw_theme_button(int x, int y, int h, Vector2 mouse) {
    const Theme& th = g_theme;
    const char* name = th.name;
    int bw = MeasureTextF(name, 11) + 28;
    Rectangle r = {(float)x,(float)y,(float)bw,(float)h};
    bool hov = CheckCollisionPointRec(mouse, r);
    DrawRectangleRec(r, hov ? th_alpha(th.bg_button) : Color{0,0,0,0});
    DrawCircle(x+8, y+h/2, 4.0f, th.accent);
    DrawTextF(name, x+16, y+(h-11)/2, 11, hov ? th.ctrl_text : th_alpha(th.ctrl_text_dim));
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static UbicacionesPanel* s_ubicaciones = nullptr;
static DocsPanel*        s_docs        = nullptr;
static EntryEditor*      s_editor      = nullptr;
static FontPanel*        s_font        = nullptr;

static void ensure_panels(AppState& state) {
    if (!s_ubicaciones) s_ubicaciones = new UbicacionesPanel(state);
    if (!s_docs)        s_docs        = new DocsPanel(state);
    if (!s_editor)      s_editor      = new EntryEditor(state);
    if (!s_font)        s_font        = new FontPanel(state);
}

bool draw_toolbar(AppState& state, Vector2 mouse) {
    ensure_panels(state);
    const Theme& th = g_theme;
    int w = SW();

    if (g_skin.toolbar_bg.valid())
        g_skin.toolbar_bg.draw(0.0f, 0.0f, (float)w, (float)TOOLBAR_H, th.bg_toolbar);
    else
        DrawRectangle(0, 0, w, TOOLBAR_H, th.bg_toolbar);

    DrawLineEx({0.0f,(float)TOOLBAR_H},{(float)w,(float)TOOLBAR_H},
               1.0f, th_alpha(th.border_panel));

    int x = 10;
    DrawTextF("ARKHE", x, (TOOLBAR_H-16)/2, 16, th.text_logo);
    x += MeasureTextF("ARKHE", 16) + 14;
    draw_separator(x); x += 10;

    // Tabs (ahora con Apariencia añadida)
    struct Tab { const char* label; ToolbarTab tab; bool* flag; };
    Tab tabs[] = {
        { "Ubicaciones", ToolbarTab::Ubicaciones, &state.toolbar.ubicaciones_open },
        { "Docs",        ToolbarTab::Docs,        &state.toolbar.docs_open        },
        { "Editor",      ToolbarTab::Editor,      &state.toolbar.editor_open      },
        { "Apariencia",  ToolbarTab::Font,        &state.toolbar.font_panel_open  },
    };
    for (auto& t : tabs) {
        int bw     = MeasureTextF(t.label, 12) + 24;
        bool active = (state.toolbar.active_tab == t.tab) && *t.flag;
        if (draw_tab_button(t.label, x, 0, bw, TOOLBAR_H, active, mouse)) {
            bool was = *t.flag;
            // Cerrar todos
            state.toolbar.ubicaciones_open = state.toolbar.docs_open =
            state.toolbar.editor_open      = state.toolbar.font_panel_open = false;
            if (!was) { *t.flag = true; state.toolbar.active_tab = t.tab; }
            else       state.toolbar.active_tab = ToolbarTab::None;
        }
        x += bw + 2;
    }
    draw_separator(x); x += 10;

    auto loaded = state.textures.loaded_keys();
    char tex_info[48];
    snprintf(tex_info, sizeof(tex_info), "%d img", (int)loaded.size());
    DrawTextF(tex_info, x, (TOOLBAR_H-10)/2, 10, th_alpha(th.success));
    x += MeasureTextF(tex_info, 10) + 12;
    draw_separator(x); x += 10;

    const char* mode_str = state.mode == ViewMode::Mathlib  ? "Mathlib"  :
                           state.mode == ViewMode::Standard ? "Estandar" : "MSC2020";
    DrawTextF(mode_str, x, (TOOLBAR_H-11)/2, 11, th_alpha(th.success));
    x += MeasureTextF(mode_str, 11) + 10;
    draw_separator(x); x += 10;

    if (draw_theme_button(x, 0, TOOLBAR_H, mouse))
        state.toolbar.theme_id = (state.toolbar.theme_id + 1) % THEME_COUNT;
    x += MeasureTextF(th.name, 11) + 32;
    draw_separator(x);

    // Ruta de assets (derecha)
    std::string info = state.toolbar.assets_path;
    if ((int)info.size() > 40) info = ".." + info.substr(info.size() - 38);
    DrawTextF(info.c_str(),
              w - MeasureTextF(info.c_str(), 10) - 12,
              (TOOLBAR_H-10)/2, 10, th_alpha(th.text_dim));

    // Paneles flotantes (se dibujan al final → encima de todo)
    s_ubicaciones->draw(mouse);
    s_docs->draw(mouse);
    s_editor->draw(mouse);
    s_font->draw(mouse);

    return state.toolbar.assets_changed;
}
