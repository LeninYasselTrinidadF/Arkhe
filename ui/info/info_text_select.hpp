#pragma once
#include "raylib.h"
#include "ui/aesthetic/font_manager.hpp"  // MeasureTextF, DrawTextF
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// InfoTextSelect
//
// Permite seleccionar texto del info-panel arrastrando el mouse (LMB drag) y
// copiarlo al portapapeles con Ctrl+C.  El drag solo se activa cuando el mouse
// se mueve > 8 px desde el punto de clic, de modo que los clicks normales en
// cards/botones no se interfieren.
//
// Uso en draw_info_panel:
//   1. Al inicio del frame (antes de BeginScissorMode):
//          g_info_sel.begin_frame();
//   2. Dentro del bloque scissor, en lugar de DrawTextF usa DrawTextFS para
//      texto que quieras que sea seleccionable.
//   3. Antes de EndScissorMode:
//          g_info_sel.draw();
//   4. Después de EndScissorMode:
//          g_info_sel.handle(mouse, scroll_top, info_blocked);
// ─────────────────────────────────────────────────────────────────────────────

struct InfoTextEntry {
    std::string text;
    Rectangle   rect     = {};
    bool        selected = false;
};

struct InfoTextSelect {
    std::vector<InfoTextEntry> entries;

    bool    drag_started = false; // LMB presionado en el área
    bool    dragging     = false; // umbral de 8 px superado
    bool    has_sel      = false; // hay selección activa
    int     copy_flash   = 0;     // frames de feedback "copiado"
    Vector2 drag_start   = {};
    Vector2 drag_cur     = {};

    // ── API ────────────────────────────────────────────────────────────────────

    // Llama al INICIO del frame (antes de BeginScissorMode).
    void begin_frame();

    // Registra una entrada de texto (rect = bounding box del texto).
    // Llamado internamente por DrawTextFS.
    void register_entry(const char* text, int x, int y, int w, int h);

    // Procesa mouse + teclado.  Llamar DESPUÉS de EndScissorMode.
    // scroll_top : coordenada Y donde empieza la zona scrollable.
    // blocked    : true si hay un modal encima (overlay::blocks_mouse).
    void handle(Vector2 mouse, int scroll_top, bool blocked);

    // Dibuja highlights de selección y rectángulo de arrastre.
    // Llamar DENTRO de BeginScissorMode, al final del bloque.
    void draw() const;

    // ── Helpers ────────────────────────────────────────────────────────────────
    Rectangle  sel_rect()       const;
    std::string sel_text()      const;   // solo items seleccionados
    std::string all_text()      const;   // todos los items registrados
};

extern InfoTextSelect g_info_sel;

// ── DrawTextFS ────────────────────────────────────────────────────────────────
// Drop-in de DrawTextF: dibuja el texto Y lo registra en g_info_sel.
// Usa solo dentro de la zona scrollable del info-panel.
inline void DrawTextFS(const char* text, int x, int y, int font_size, Color color) {
    DrawTextF(text, x, y, font_size, color);
    int tw = MeasureTextF(text, font_size);
    g_info_sel.register_entry(text, x, y, tw, font_size + 2);
}
