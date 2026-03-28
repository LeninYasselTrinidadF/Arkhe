#pragma once
#include "app.hpp"
#include "raylib.h"

// Clase base para todos los paneles flotantes de la UI.
// Centraliza helpers de dibujo de widgets (campo de texto, chip, texto wrapped, etc.)
// para que los paneles hijos no dupliquen lógica.
class PanelWidget {
protected:
    AppState& state;

    // ── Input ──────────────────────────────────────────────────────────────
    // Procesa teclado sobre un buffer: caracteres imprimibles + backspace.
    static void handle_text_input(char* buf, int buf_len);

    // Campo de texto editable. Devuelve true si se presionó Enter.
    // active_id se compara contra my_id para saber si este campo está activo.
    static bool draw_text_field(char* buf, int buf_len,
                                int x, int y, int w, int h, int font,
                                int& active_id, int my_id, Vector2 mouse);

    // Campo con etiqueta encima (shortcut para formularios).
    static void draw_labeled_field(const char* label, char* buf, int buf_len,
                                   int x, int y, int w, int font,
                                   int& active_id, int my_id, Vector2 mouse);

    // ── Primitivos de dibujo ───────────────────────────────────────────────
    static void draw_chip(const char* text, int x, int y, Color bg, Color fg);
    static int  draw_wrapped_text(const char* text, int x, int y,
                                  int max_w, int font_size, Color color);
    static void draw_h_line(int x1, int y, int x2, Color color = { 30, 30, 50, 200 });

    // ── Ventana flotante ───────────────────────────────────────────────────
    // Dibuja fondo + sombra + cabecera y el botón de cerrar.
    // Devuelve true si se hizo click en cerrar.
    bool draw_window_frame(int px, int py, int pw, int ph,
                           const char* title, Color title_col,
                           Color border_col, Vector2 mouse) const;

public:
    explicit PanelWidget(AppState& s) : state(s) {}
    virtual ~PanelWidget() = default;

    // Cada panel implementa su propio draw().
    virtual void draw(Vector2 mouse) = 0;
};
