#pragma once
#include "../app.hpp"
#include "raylib.h"

// Altura reservada para el toolbar (pixels)
constexpr int TOOLBAR_H = 34;

// Dibuja el toolbar horizontal en la parte superior de la ventana.
// Gestiona: selector de assets, boton de docs, boton de editor.
// Devuelve true si se produjo un cambio de assets_path que requiere recarga.
bool draw_toolbar(AppState& state, Vector2 mouse);

// Paneles flotantes (se llaman desde main despues de BeginDrawing)
void draw_docs_panel(AppState& state, Vector2 mouse);
void draw_entry_editor(AppState& state, Vector2 mouse);

enum class ToolbarTab { None, Config, Editor, Docs };

struct ToolbarState {
    ToolbarTab active_tab = ToolbarTab::None;
    char latex_path[512] = "C:/miktex/bin/x64/pdflatex.exe";
    char assets_path[512] = "assets/";
    bool assets_changed = false;
};