#pragma once
#include "app.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include <string>

// ── TextFieldState ────────────────────────────────────────────────────────────
// Estado de edición para un campo de texto de línea única.
// Un único estado global (g_tf) sirve al campo activo del frame.
// ─────────────────────────────────────────────────────────────────────────────
struct TextFieldState {
    int cursor = 0;
    int sel_from = 0;
    int sel_to = 0;

    bool has_selection() const { return sel_from != sel_to; }
    int  sel_min()       const { return sel_from < sel_to ? sel_from : sel_to; }
    int  sel_max()       const { return sel_from < sel_to ? sel_to : sel_from; }

    void set_cursor(int pos, int len) {
        cursor = sel_from = sel_to = clamp(pos, 0, len);
    }
    void extend_sel(int pos, int len) {
        sel_to = cursor = clamp(pos, 0, len);
    }
    void collapse(int pos, int len) {
        cursor = sel_from = sel_to = clamp(pos, 0, len);
    }

    static int clamp(int v, int lo, int hi) {
        return v < lo ? lo : v > hi ? hi : v;
    }
};

// Estado global del campo de texto activo
extern TextFieldState g_tf;
extern int            g_tf_active_id;  // ID del campo dueño de g_tf (-1 = ninguno)
extern std::string    g_tf_clipboard;  // clipboard interno

// ── BodyAreaState ─────────────────────────────────────────────────────────────
// Estado de edición para el textarea multilinea del body del editor.
// ─────────────────────────────────────────────────────────────────────────────
struct BodyAreaState {
    int cursor_pos = 0;  // posición absoluta en bytes dentro del buf
    int sel_from = 0;
    int sel_to = 0;

    bool has_selection() const { return sel_from != sel_to; }
    int  sel_min()       const { return sel_from < sel_to ? sel_from : sel_to; }
    int  sel_max()       const { return sel_from < sel_to ? sel_to : sel_from; }
    void set_cursor(int pos) { cursor_pos = sel_from = sel_to = pos; }
    void extend_sel(int pos) { sel_to = cursor_pos = pos; }
};

extern BodyAreaState g_body;

// ── PanelWidget ───────────────────────────────────────────────────────────────
class PanelWidget {
protected:
    AppState& state;

    int pos_x = 0, pos_y = 0;
    int cur_pw = 0, cur_ph = 0;

    bool dragging = false;
    int  drag_off_x = 0, drag_off_y = 0;

    bool resizing = false;
    int  resize_edge = 0;
    int  resize_sx = 0, resize_sy = 0;
    int  resize_ox = 0, resize_oy = 0;
    int  resize_opw = 0, resize_oph = 0;

    bool draw_window_frame(int min_pw, int min_ph,
        const char* title, Color title_col,
        Color border_col, Vector2 mouse);

public:
    // handle_text_input: versión legacy sin cursor (usada internamente).
    static void handle_text_input(char* buf, int buf_len);

    // draw_text_field: campo completo con cursor, selección, Ctrl+C/V/A,
    // Shift+flechas, selección por mouse y clipboard del SO.
    // Devuelve true si el usuario confirma con Enter.
    static bool draw_text_field(char* buf, int buf_len,
        int x, int y, int w, int h, int font,
        int& active_id, int my_id, Vector2 mouse);

    static void draw_labeled_field(const char* label, char* buf, int buf_len,
        int x, int y, int w, int font,
        int& active_id, int my_id, Vector2 mouse);

    static void draw_chip(const char* text, int x, int y, Color bg, Color fg);
    static int  draw_wrapped_text(const char* text, int x, int y,
        int max_w, int font_size, Color color);
    static void draw_h_line(int x1, int y, int x2,
        Color color = { 30, 30, 50, 200 });

    explicit PanelWidget(AppState& s) : state(s) {}
    virtual ~PanelWidget() = default;
    virtual void draw(Vector2 mouse) = 0;
};