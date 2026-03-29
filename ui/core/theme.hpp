#pragma once
#include "raylib.h"

// ── Theme ─────────────────────────────────────────────────────────────────────
// Toda la paleta de colores de la UI en un struct.
// Acceder siempre a través de g_theme.
// ─────────────────────────────────────────────────────────────────────────────

struct Theme {
    const char* name;

    // ── Fondos principales ────────────────────────────────────────────────────
    Color bg_app;           // ClearBackground + info panel fondo
    Color bg_toolbar;       // barra de herramientas
    Color bg_panel;         // ventanas flotantes (fondo)
    Color bg_panel_header;  // cabecera de ventana flotante
    Color bg_field;         // campos de texto
    Color bg_card;          // cards de recursos / crossrefs
    Color bg_card_hover;
    Color bg_button;        // botones genéricos (zoom, atrás)
    Color bg_button_hover;

    // ── Texto ─────────────────────────────────────────────────────────────────
    Color text_primary;     // contenido principal
    Color text_secondary;   // subtítulos, previews .tex
    Color text_dim;         // hints, metadatos de baja prioridad
    Color text_code;        // códigos MSC, nombres de módulo
    Color text_logo;        // "ARKHE" en toolbar

    // ── Acento principal (azul en dark, rosa en bocchi, rojo en CSM) ──────────
    Color accent;
    Color accent_hover;
    Color accent_dim;       // indicador de tab hover

    // ── Familia success (verde → Mathlib / LEAN) ──────────────────────────────
    Color success;
    Color success_dim;      // bordes de cards ML

    // ── Familia warning (ámbar → Estándar / STD) ─────────────────────────────
    Color warning;

    // ── Bordes ────────────────────────────────────────────────────────────────
    Color border;           // borde de campo normal
    Color border_hover;     // borde de campo con hover
    Color border_active;    // borde de campo activo / tab activo
    Color border_panel;     // borde de ventana flotante

    // ── Burbujas ──────────────────────────────────────────────────────────────
    Color bubble_center_flat;   // burbuja central cuando el nodo no tiene color
    Color bubble_ring;          // contorno de burbuja central
    Color bubble_hover_ring;    // anillo blanco al hacer hover en hijo
    Color bubble_label_center;  // texto sobre burbuja central
    Color bubble_label_child;   // texto sobre burbuja hija
    Color bubble_empty_msg;     // "Sin sub-areas"
    float bubble_line_alpha;    // alpha para Fade() en líneas de conexión
    float bubble_center_alpha;  // alpha para Fade() en burbuja central con color
    float bubble_glow_alpha;    // alpha para Fade() en glow de hijos

    // ── Controles de UI (switcher de modo, zoom buttons) ─────────────────────
    Color ctrl_bg;
    Color ctrl_bg_hover;
    Color ctrl_border;
    Color ctrl_text;
    Color ctrl_text_dim;

    // ── Divisor horizontal ────────────────────────────────────────────────────
    Color split_normal;
    Color split_active;     // al arrastrar o hacer hover
    Color split_dots;       // puntitos del handle
    Color split_vline;      // línea vertical entre canvas y panel lateral

    // ── Ventana flotante ──────────────────────────────────────────────────────
    Color shadow;           // sombra offset
    Color close_bg;         // botón X normal
    Color close_bg_hover;   // botón X hover

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    Color scrollbar_bg;
    Color scrollbar_thumb;

    // ── Flags de rendimiento ──────────────────────────────────────────────────
    bool use_transparency;  // false → todos los alphas < 255 se ignoran
    bool use_fade_lines;    // false → líneas de burbuja sin Fade()
};

// ── Global activo ─────────────────────────────────────────────────────────────
extern Theme g_theme;

// ── Temas predefinidos ────────────────────────────────────────────────────────
Theme theme_dark();
Theme theme_light();
Theme theme_bocchi();
Theme theme_chainsawman();
Theme theme_generic();

// Aplica el tema por índice (0=dark 1=light 2=bocchi 3=csm 4=generic)
void apply_theme(int id);

constexpr int THEME_COUNT = 5;
const char* theme_name(int id);   // nombre corto para el botón

// ── Helpers inline ────────────────────────────────────────────────────────────
// Usa siempre estos wrappers en lugar de Fade() directo.

// Aplica Fade si el tema lo permite; si no, devuelve el color sin alpha.
inline Color th_fade(Color c, float a) {
    return g_theme.use_fade_lines ? Fade(c, a) : c;
}

// Hace un color semitransparente según el tema.
// Para solid colors (alpha ya = 255) devuelve tal cual.
inline Color th_alpha(Color c) {
    if (!g_theme.use_transparency && c.a < 255) c.a = 255;
    return c;
}