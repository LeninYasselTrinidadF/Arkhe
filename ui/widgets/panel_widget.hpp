#pragma once
#include "app.hpp"
#include "../core/theme.hpp"
#include "../constants.hpp"
#include "raylib.h"

// ── PanelWidget ───────────────────────────────────────────────────────────────
// Base para paneles flotantes modales.
//
// Comportamiento:
//  · Primera apertura: tamaño = ventana - MARGIN (al menos min_pw × min_ph).
//  · Posición inicial: centrada.
//  · Arrastrable desde la barra de título.
//  · Redimensionable desde los 4 bordes y las 4 esquinas.
//  · Dibuja un backdrop semitransparente que bloquea el canvas completo.
//
// Subclases:
//  · Heredan cur_pw / cur_ph para adaptar su layout al tamaño real.
//  · Llaman draw_window_frame(min_pw, min_ph, ...) al inicio de draw().
//  · El panel se resetea a cur_pw = cur_ph = 0 SOLO en la primera
//    construcción; las reaperturas conservan el tamaño anterior.
// ─────────────────────────────────────────────────────────────────────────────

class PanelWidget {
protected:
    AppState& state;

    // ── Posición / tamaño ──────────────────────────────────────────────────
    int pos_x = 0, pos_y = 0;
    int cur_pw = 0, cur_ph = 0;   // 0 → inicializar en el primer draw

    // ── Drag ──────────────────────────────────────────────────────────────
    bool dragging = false;
    int  drag_off_x = 0, drag_off_y = 0;

    // ── Resize ────────────────────────────────────────────────────────────
    bool resizing = false;
    int  resize_edge = 0;        // bitmask: 1=L 2=R 4=T 8=B
    int  resize_sx = 0, resize_sy = 0;   // mouse al empezar
    int  resize_ox = 0, resize_oy = 0;   // pos al empezar
    int  resize_opw = 0, resize_oph = 0;   // tamaño al empezar

    // ── Ventana flotante modal ─────────────────────────────────────────────
    // min_pw/min_ph: tamaño mínimo (los constexpr de la subclase).
    // En el primer draw el tamaño real se calcula como SW()-2*MARGIN,
    // clampeado a al menos min_pw×min_ph.
    // Devuelve true si se hizo click en el botón cerrar.
    bool draw_window_frame(int min_pw, int min_ph,
        const char* title, Color title_col,
        Color border_col, Vector2 mouse);

public:
    // ── Helpers de input ──────────────────────────────────────────────────
    static void handle_text_input(char* buf, int buf_len);

    static bool draw_text_field(char* buf, int buf_len,
        int x, int y, int w, int h, int font,
        int& active_id, int my_id, Vector2 mouse);

    static void draw_labeled_field(const char* label, char* buf, int buf_len,
        int x, int y, int w, int font,
        int& active_id, int my_id, Vector2 mouse);

    // ── Primitivos de dibujo ───────────────────────────────────────────────
    static void draw_chip(const char* text, int x, int y, Color bg, Color fg);
    static int  draw_wrapped_text(const char* text, int x, int y,
        int max_w, int font_size, Color color);
    static void draw_h_line(int x1, int y, int x2,
        Color color = { 30, 30, 50, 200 });


    explicit PanelWidget(AppState& s) : state(s) {}
    virtual ~PanelWidget() = default;
    virtual void draw(Vector2 mouse) = 0;
};