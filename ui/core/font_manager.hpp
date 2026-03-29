#pragma once
#include "raylib.h"
#include <string>

// ── FontManager ───────────────────────────────────────────────────────────────
// Gestiona UNA fuente global custom (.ttf) con un tamaño base configurable.
// Si la fuente no carga, hace fallback a la default de raylib transparentemente.
//
// Uso:
//   g_fonts.load("assets/fonts/myfont.ttf");   // en main(), tras InitWindow
//   g_fonts.base_size = 14;                     // ajustable por el usuario
//   DrawTextF("Hola", x, y, 12, WHITE);         // reemplaza DrawText()
//   int w = MeasureTextF("Hola", 12);           // reemplaza MeasureText()
//
// El parámetro `size` en DrawTextF/MeasureTextF es un tamaño RELATIVO:
//   size final = size * (base_size / 14.0f)
// Con base_size=21 (default) toda la UI se renderiza a 1.5× el tamaño original.
// Cambiar base_size en ToolbarState::theme_id o en Config para ajuste global.
// ─────────────────────────────────────────────────────────────────────────────

struct FontManager {
    Font    font = {};
    bool    loaded = false;
    float   base_size = 21.0f;   // tamaño base del usuario (8–32) — default 21 = 1.5× original

    // Carga la fuente desde path. Genera glifos hasta char 1024 para cubrir
    // símbolos matemáticos básicos del rango Latin Extended.
    // Si falla, loaded=false y las funciones usan la default de raylib.
    void load(const std::string& path, int atlas_size = 48);

    // Descarga la fuente de GPU. Llamar antes de CloseWindow().
    void unload();

    // Escala un tamaño relativo al base_size del usuario.
    inline int scale(int size) const {
        if (!loaded) return size;
        return std::max(6, (int)(size * base_size / 14.0f + 0.5f));
    }

    // Equivalente a DrawText() pero usa la fuente custom con escala.
    void draw(const char* text, int x, int y, int size, Color color) const;

    // Equivalente a MeasureText() pero usa la fuente custom con escala.
    int  measure(const char* text, int size) const;
};

extern FontManager g_fonts;

// ── Shortcuts globales (drop-in para DrawText / MeasureText) ─────────────────
// Usar estas macros/funciones en vez de las de raylib para que toda la UI
// respete la fuente custom y el tamaño de base configurado por el usuario.

inline void DrawTextF(const char* text, int x, int y, int size, Color color) {
    g_fonts.draw(text, x, y, size, color);
}
inline int MeasureTextF(const char* text, int size) {
    return g_fonts.measure(text, size);
}