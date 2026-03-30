#include "panel_widget.hpp"
#include "../core/font_manager.hpp"
#include "../core/overlay.hpp"
#include "../core/theme.hpp"
#include "../constants.hpp"
#include <cstring>
#include <algorithm>

// ── Constantes ────────────────────────────────────────────────────────────────

static constexpr int HEADER_H = 30;
static constexpr int MARGIN = 48;   // margen entre ventana principal y panel
static constexpr int RESIZE_HIT = 7;    // px cerca del borde para activar resize
static constexpr int CORNER_LEN = 14;   // longitud de las marcas de esquina

// ── handle_text_input ─────────────────────────────────────────────────────────

void PanelWidget::handle_text_input(char* buf, int buf_len) {
    int key = GetCharPressed();
    while (key > 0) {
        int len = (int)strlen(buf);
        if (key >= 32 && key <= 126 && len < buf_len - 1) {
            buf[len] = (char)key; buf[len + 1] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) buf[len - 1] = '\0';
    }
}

// ── draw_text_field ───────────────────────────────────────────────────────────

bool PanelWidget::draw_text_field(char* buf, int buf_len,
    int x, int y, int w, int h, int font,
    int& active_id, int my_id, Vector2 mouse)
{
    const Theme& th = g_theme;
    bool is_active = (active_id == my_id);
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = CheckCollisionPointRec(mouse, r);

    Color border = is_active ? th.border_active
        : hov ? th.border_hover
        : th_alpha(th.border);
    DrawRectangleRec(r, th.bg_field);
    DrawRectangleLinesEx(r, 1.0f, border);

    std::string shown = buf;
    while (MeasureTextF(shown.c_str(), font) > w - 10 && shown.size() > 3)
        shown = ".." + shown.substr(shown.size() - shown.size() / 2);
    DrawTextF(shown.c_str(), x + 5, y + (h - font) / 2, font, th.text_primary);

    if (is_active && (int)(GetTime() * 2) % 2 == 0) {
        int cw = MeasureTextF(buf, font);
        int cx = x + 5 + cw;
        if (cx < x + w - 4) DrawLine(cx, y + 3, cx, y + h - 3, th.text_primary);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        active_id = hov ? my_id : (is_active ? -1 : active_id);

    bool confirmed = false;
    if (is_active) {
        handle_text_input(buf, buf_len);
        if (IsKeyPressed(KEY_ENTER)) { active_id = -1; confirmed = true; }
    }
    return confirmed;
}

// ── draw_labeled_field ────────────────────────────────────────────────────────

void PanelWidget::draw_labeled_field(const char* label, char* buf, int buf_len,
    int x, int y, int w, int font,
    int& active_id, int my_id, Vector2 mouse)
{
    DrawTextF(label, x, y, 10, th_alpha(g_theme.text_dim));
    draw_text_field(buf, buf_len, x, y + 14, w, font + 4, font, active_id, my_id, mouse);
}

// ── draw_chip ─────────────────────────────────────────────────────────────────

void PanelWidget::draw_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureTextF(text, 11);
    DrawRectangle(x, y, tw + 14, 20, th_alpha(bg));
    DrawTextF(text, x + 7, y + 5, 11, fg);
}

// ── draw_wrapped_text ─────────────────────────────────────────────────────────

int PanelWidget::draw_wrapped_text(const char* text, int x, int y,
    int max_w, int font_size, Color color)
{
    std::string s(text);
    int line_y = y;
    while (!s.empty()) {
        int chars = 1;
        while (chars < (int)s.size() &&
            MeasureTextF(s.substr(0, chars + 1).c_str(), font_size) < max_w)
            chars++;
        if (chars < (int)s.size() && s[chars] != ' ') {
            int sp = (int)s.rfind(' ', chars);
            if (sp > 0) chars = sp;
        }
        DrawTextF(s.substr(0, chars).c_str(), x, line_y, font_size, color);
        s = s.substr(chars);
        if (!s.empty() && s[0] == ' ') s = s.substr(1);
        line_y += font_size + 4;
    }
    return line_y;
}

// ── draw_h_line ───────────────────────────────────────────────────────────────

void PanelWidget::draw_h_line(int x1, int y, int x2, Color color) {
    DrawLine(x1, y, x2, y, color);
}

// ── draw_window_frame ─────────────────────────────────────────────────────────

bool PanelWidget::draw_window_frame(int min_pw, int min_ph,
    const char* title, Color title_col, Color border_col, Vector2 mouse)
{
    const Theme& th = g_theme;
    int sw = SW(), sh = SH();

    // ── Inicialización lazy (primera apertura) ────────────────────────────────
    // Tamaño: ventana − MARGIN en cada lado, pero al menos min_pw × min_ph.
    if (cur_pw <= 0 || cur_ph <= 0) {
        cur_pw = std::max(min_pw, sw - 2 * MARGIN);
        cur_ph = std::max(min_ph, sh - TOOLBAR_H - 2 * MARGIN);
        // Centrado bajo el toolbar
        pos_x = (sw - cur_pw) / 2;
        pos_y = TOOLBAR_H + (sh - TOOLBAR_H - cur_ph) / 2;
    }

    // ── Clamp posición (no salir de pantalla) ─────────────────────────────────
    pos_x = std::clamp(pos_x, 0, std::max(0, sw - cur_pw));
    pos_y = std::clamp(pos_y, TOOLBAR_H, std::max(TOOLBAR_H, sh - cur_ph));
    int px = pos_x, py = pos_y;
    int pw = cur_pw, ph = cur_ph;

    // ── Backdrop modal (oscurece y bloquea el canvas) ─────────────────────────
    DrawRectangle(0, 0, sw, sh, { 0, 0, 0, 120 });
    overlay::push_rect({ 0.0f, 0.0f, (float)sw, (float)sh });

    // ── Detección de borde para resize ────────────────────────────────────────
    bool in_x = mouse.x >= px && mouse.x <= px + pw;
    bool in_y = mouse.y >= py && mouse.y <= py + ph;
    bool on_l = mouse.x >= px - RESIZE_HIT && mouse.x <= px + RESIZE_HIT && in_y;
    bool on_r = mouse.x >= px + pw - RESIZE_HIT && mouse.x <= px + pw + RESIZE_HIT && in_y;
    bool on_t = mouse.y >= py - RESIZE_HIT && mouse.y <= py + RESIZE_HIT && in_x;
    bool on_b = mouse.y >= py + ph - RESIZE_HIT && mouse.y <= py + ph + RESIZE_HIT && in_x;

    int edge = 0;
    if (on_l) edge |= 1;
    if (on_r) edge |= 2;
    if (on_t) edge |= 4;
    if (on_b) edge |= 8;

    Rectangle title_bar = { (float)px, (float)py, (float)(pw - 32), (float)HEADER_H };
    bool over_title = CheckCollisionPointRec(mouse, title_bar);

    // ── Iniciar resize (prioridad sobre drag) ─────────────────────────────────
    if (!resizing && !dragging && edge != 0 && !over_title
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        resizing = true;
        resize_edge = edge;
        resize_sx = (int)mouse.x;   resize_sy = (int)mouse.y;
        resize_ox = pos_x;          resize_oy = pos_y;
        resize_opw = cur_pw;         resize_oph = cur_ph;
    }

    // ── Soltar (unifica drag y resize) ────────────────────────────────────────
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        resizing = false;
        dragging = false;
        resize_edge = 0;
    }

    // ── Aplicar resize ────────────────────────────────────────────────────────
    if (resizing) {
        int dx = (int)mouse.x - resize_sx;
        int dy = (int)mouse.y - resize_sy;

        if (resize_edge & 2) {                                          // borde derecho
            cur_pw = std::max(min_pw, resize_opw + dx);
        }
        if (resize_edge & 8) {                                          // borde inferior
            cur_ph = std::max(min_ph, resize_oph + dy);
        }
        if (resize_edge & 1) {                                          // borde izquierdo
            int new_pw = std::max(min_pw, resize_opw - dx);
            pos_x = std::clamp(resize_ox + (resize_opw - new_pw), 0, sw - new_pw);
            cur_pw = new_pw;
        }
        if (resize_edge & 4) {                                          // borde superior
            int new_ph = std::max(min_ph, resize_oph - dy);
            pos_y = std::clamp(resize_oy + (resize_oph - new_ph), TOOLBAR_H, sh - new_ph);
            cur_ph = new_ph;
        }
        // Refrescar locales para el dibujo de este frame
        pw = cur_pw; ph = cur_ph;
        px = pos_x;  py = pos_y;
    }

    // ── Iniciar drag desde title bar ──────────────────────────────────────────
    if (!resizing) {
        if (over_title && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging = true;
            drag_off_x = (int)mouse.x - px;
            drag_off_y = (int)mouse.y - py;
        }
        if (dragging) {
            pos_x = std::clamp((int)mouse.x - drag_off_x, 0, std::max(0, sw - pw));
            pos_y = std::clamp((int)mouse.y - drag_off_y, TOOLBAR_H, std::max(TOOLBAR_H, sh - ph));
            px = pos_x; py = pos_y;
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // DIBUJO
    // ═════════════════════════════════════════════════════════════════════════

    // Sombra
    if (th.use_transparency)
        DrawRectangle(px + 6, py + 6, pw, ph, { 0, 0, 0, 90 });

    // Cuerpo
    DrawRectangle(px, py, pw, ph, th.bg_panel);
    DrawRectangleLinesEx({ (float)px,(float)py,(float)pw,(float)ph }, 1.5f, border_col);

    // Cabecera
    DrawRectangle(px, py, pw, HEADER_H, th.bg_panel_header);
    DrawTextF(title, px + 12, py + 9, 12, title_col);

    // Indicador arrastrable (seis puntitos en el centro de la cabecera)
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 2; col++)
            DrawRectangle(px + pw / 2 - 4 + col * 5,
                py + 10 + row * 4, 2, 2,
                (over_title || dragging)
                ? th_alpha(th.ctrl_text_dim) : Color{ 0,0,0,0 });

    // Marcas de esquina (visual de resize)
    Color cc = { 80, 105, 200, 150 };
    // top-left
    DrawLine(px, py, px + CORNER_LEN, py, cc);
    DrawLine(px, py, px, py + CORNER_LEN, cc);
    // top-right
    DrawLine(px + pw, py, px + pw - CORNER_LEN, py, cc);
    DrawLine(px + pw, py, px + pw, py + CORNER_LEN, cc);
    // bottom-left
    DrawLine(px, py + ph, px + CORNER_LEN, py + ph, cc);
    DrawLine(px, py + ph, px, py + ph - CORNER_LEN, cc);
    // bottom-right
    DrawLine(px + pw, py + ph, px + pw - CORNER_LEN, py + ph, cc);
    DrawLine(px + pw, py + ph, px + pw, py + ph - CORNER_LEN, cc);

    // Highlight del borde activo (hover o resize en curso)
    if ((edge || resizing) && !over_title) {
        int ae = resizing ? resize_edge : edge;   // borde activo
        Color hc = resizing ? Color{ 120, 160, 255, 200 } : Color{ 80, 120, 220, 130 };
        if (ae & 1) DrawLine(px, py, px, py + ph, hc);
        if (ae & 2) DrawLine(px + pw, py, px + pw, py + ph, hc);
        if (ae & 4) DrawLine(px, py, px + pw, py, hc);
        if (ae & 8) DrawLine(px, py + ph, px + pw, py + ph, hc);
    }

    // Botón cerrar
    Rectangle cr = { (float)(px + pw - 28), (float)(py + 5), 22.0f, 22.0f };
    bool chov = CheckCollisionPointRec(mouse, cr);
    DrawRectangleRec(cr, chov ? th.close_bg_hover : th.close_bg);
    DrawTextF("x", (int)cr.x + 7, (int)cr.y + 4, 13, th.text_primary);

    return chov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}