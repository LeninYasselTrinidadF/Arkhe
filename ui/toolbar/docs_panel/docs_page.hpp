#pragma once
// ── docs_page.hpp ─────────────────────────────────────────────────────────────
// Tipos base del sistema de documentación paginada del DocsPanel.
//
// Cada página es un struct DocsPage con su función draw(). El panel
// central (docs_panel_v2.hpp) mantiene un índice de página y llama a
// la función draw() correspondiente pasando el área disponible.
//
// Convenciones de layout:
//   · lx, lw   — x de inicio y ancho del contenido (con padding lateral)
//   · y        — cursor vertical mutable; las funciones lo incrementan
//   · y_bottom — límite inferior estricto; no dibujar por debajo
// ─────────────────────────────────────────────────────────────────────────────

#include "raylib.h"
#include <string>
#include <vector>

// ── Colores semánticos para la documentación ──────────────────────────────────

// Paleta fija independiente del tema global (el DocsPanel tiene fondo propio).
namespace docs_col {
    constexpr Color bg          = {  14,  16,  28, 255 };  // fondo del panel
    constexpr Color bg_card     = {  20,  24,  42, 255 };  // tarjeta / bloque
    constexpr Color bg_code     = {  12,  14,  26, 255 };  // bloque de código
    constexpr Color border      = {  40,  50,  90, 180 };  // borde sutil
    constexpr Color border_acc  = {  70, 110, 210, 220 };  // borde acento
    constexpr Color accent      = {  80, 140, 255, 255 };  // azul principal
    constexpr Color accent2     = { 120, 210, 160, 255 };  // verde secundario
    constexpr Color accent3     = { 220, 170,  80, 255 };  // ámbar / warning
    constexpr Color text_head   = { 200, 215, 255, 240 };  // título de sección
    constexpr Color text_body   = { 160, 170, 200, 220 };  // cuerpo normal
    constexpr Color text_dim    = { 100, 110, 150, 180 };  // texto secundario
    constexpr Color text_code   = { 140, 210, 255, 230 };  // inline code
    constexpr Color text_key    = { 110, 190, 140, 230 };  // tecla / shortcut
    constexpr Color sep         = {  30,  36,  68, 200 };  // separador
}

// ── DocsPage ──────────────────────────────────────────────────────────────────

struct DocsPage {
    std::string title;    ///< Mostrado en la tab de navegación
    std::string icon;     ///< Carácter unicode corto (emoji o símbolo)

    /// Dibuja el contenido de la página.
    /// @param lx       x de inicio del área de contenido
    /// @param lw       ancho del área de contenido
    /// @param y        cursor vertical (modificado por la función)
    /// @param y_bottom límite inferior del área visible
    /// @param mouse    posición del ratón
    /// @param scroll   offset de scroll actual (pixels hacia abajo)
    /// @param dt       delta time del frame (para animaciones)
    using DrawFn = void(*)(int lx, int lw,
                            int& y, int y_bottom,
                            Vector2 mouse,
                            float& scroll,
                            float dt);

    DrawFn draw = nullptr;
};

// ── Helpers de dibujo comunes ─────────────────────────────────────────────────
// Implementados en docs_draw_helpers.cpp (incluido por docs_panel.cpp).

/// Separador horizontal tenue
void docs_draw_sep(int lx, int lw, int y);

/// Título de sección con subrayado de acento
void docs_draw_section_title(const char* text, int lx, int& y);

/// Párrafo de texto con word-wrap automático. Devuelve la altura ocupada.
int docs_draw_paragraph(const char* text, int lx, int lw, int& y,
                         Color col = docs_col::text_body, int font_sz = 10);

/// Bloque de código monospace con fondo oscuro
void docs_draw_code_block(const char* code, int lx, int lw, int& y);

/// Fila clave-valor estilo tabla (key en acento, value en body)
void docs_draw_kv_row(const char* key, const char* value,
                       int lx, int lw, int& y,
                       Color key_col = docs_col::accent);

/// Tarjeta con título, descripción opcional y color lateral
void docs_draw_card(const char* title, const char* desc,
                     int lx, int lw, int& y,
                     Color accent_col = docs_col::accent);

/// Indicador de tecla (chip redondeado)
void docs_draw_key_chip(const char* key, int x, int y, int& x_out);

/// Fila de acceso rápido: chips de teclas + descripción
void docs_draw_shortcut_row(const std::vector<const char*>& keys,
                              const char* desc,
                              int lx, int lw, int& y);
