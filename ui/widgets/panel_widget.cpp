#include "ui/widgets/panel_widget.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/constants.hpp"
#include <cstring>
#include <algorithm>
#include <string>
#include <unordered_map>

// ── Constantes de layout ──────────────────────────────────────────────────────
static constexpr int HEADER_H = 30;
static constexpr int MARGIN = 48;
static constexpr int RESIZE_HIT = 7;
static constexpr int CORNER_LEN = 14;

// ── Estado global de campos de texto ─────────────────────────────────────────
TextFieldState g_tf;
int            g_tf_active_id = -1;
std::string    g_tf_clipboard;
BodyAreaState  g_body;

// ── Helpers de medición de texto ──────────────────────────────────────────────

// Devuelve el índice de carácter más cercano a la coordenada x dentro de buf.
static int char_at_x(const char* buf, int font, int field_x, int mouse_x) {
    int len = (int)strlen(buf);
    int best = 0;
    int best_dist = std::abs(field_x - mouse_x);
    for (int i = 1; i <= len; i++) {
        int cx = field_x + MeasureTextF(std::string(buf, i).c_str(), font);
        int dist = std::abs(cx - mouse_x);
        if (dist < best_dist) { best_dist = dist; best = i; }
    }
    return best;
}

// ── helpers de edición de buf ─────────────────────────────────────────────────

// Borra el rango [from, to) en buf (in-place). Devuelve nueva longitud.
static int buf_erase(char* buf, int from, int to, int buf_len) {
    int len = (int)strlen(buf);
    from = std::clamp(from, 0, len);
    to = std::clamp(to, 0, len);
    if (from >= to) return len;
    memmove(buf + from, buf + to, len - to + 1);
    return len - (to - from);
}

// Inserta str en buf en la posición pos. Respeta buf_len.
// Devuelve nueva longitud.
static int buf_insert(char* buf, int pos, const char* str, int str_len, int buf_len) {
    int len = (int)strlen(buf);
    int space = buf_len - 1 - len;
    int to_ins = std::min(str_len, space);
    if (to_ins <= 0) return len;
    memmove(buf + pos + to_ins, buf + pos, len - pos + 1);
    memcpy(buf + pos, str, to_ins);
    return len + to_ins;
}

// ── handle_text_input (legacy, sin cursor) ────────────────────────────────────
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
// Campo de texto enriquecido: cursor, selección, Ctrl+C/V/A, Shift+flechas,
// selección por mouse (click+drag), clipboard SO via raylib.

bool PanelWidget::draw_text_field(char* buf, int buf_len,
    int x, int y, int w, int h, int font,
    int& active_id, int my_id, Vector2 mouse)
{
    const Theme& th = g_theme;
    bool is_active = (active_id == my_id);
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = CheckCollisionPointRec(mouse, r);

    // ── Auto-activación vía Tab (sin clic) ────────────────────────────────────
    // Si active_id fue puesto externamente (kbnav Tab) pero g_tf_active_id
    // no fue sincronizado, sincronizar aquí: el campo toma posesión del input
    // sin requerir un clic. Esto corrige el bug donde Tab daba apariencia de
    // selección pero el teclado seguía escribiendo en el campo anterior.
    if (is_active && g_tf_active_id != my_id) {
        g_tf_active_id = my_id;
        int len = (int)strlen(buf);
        g_tf.set_cursor(len, len);   // cursor al final del contenido actual
    }

    // ── Activar / desactivar con click ────────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hov) {
            if (active_id != my_id) {
                // Cambio de campo: resetear estado de cursor
                active_id = my_id;
                g_tf_active_id = my_id;
                int len = (int)strlen(buf);
                g_tf.set_cursor(len, len);  // cursor al final
            }
            // Clic dentro del campo activo: posicionar cursor
            int text_x = x + 5;
            int clicked = char_at_x(buf, font, text_x, (int)mouse.x);
            g_tf.set_cursor(clicked, (int)strlen(buf));
        }
        else if (is_active) {
            active_id = -1;
            g_tf_active_id = -1;
        }
        is_active = (active_id == my_id);
    }

    // ── Drag del mouse para selección ─────────────────────────────────────────
    static bool s_dragging = false;
    static int  s_drag_id = -1;
    if (is_active) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hov) {
            s_dragging = true;
            s_drag_id = my_id;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            s_dragging = false;
        }
        if (s_dragging && s_drag_id == my_id && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            int text_x = x + 5;
            int drag_pos = char_at_x(buf, font, text_x, (int)mouse.x);
            g_tf.extend_sel(drag_pos, (int)strlen(buf));
        }
    }

    // ── Procesar teclado ──────────────────────────────────────────────────────
    bool confirmed = false;
    if (is_active && g_tf_active_id == my_id) {
        int len = (int)strlen(buf);
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // ── Ctrl+A: seleccionar todo ──────────────────────────────────────────
        if (ctrl && IsKeyPressed(KEY_A)) {
            g_tf.sel_from = 0;
            g_tf.sel_to = len;
            g_tf.cursor = len;
        }

        // ── Ctrl+C: copiar ────────────────────────────────────────────────────
        if (ctrl && IsKeyPressed(KEY_C) && g_tf.has_selection()) {
            g_tf_clipboard = std::string(buf + g_tf.sel_min(),
                g_tf.sel_max() - g_tf.sel_min());
            SetClipboardText(g_tf_clipboard.c_str());
        }

        // ── Ctrl+X: cortar ────────────────────────────────────────────────────
        if (ctrl && IsKeyPressed(KEY_X) && g_tf.has_selection()) {
            g_tf_clipboard = std::string(buf + g_tf.sel_min(),
                g_tf.sel_max() - g_tf.sel_min());
            SetClipboardText(g_tf_clipboard.c_str());
            int new_pos = g_tf.sel_min();
            buf_erase(buf, g_tf.sel_min(), g_tf.sel_max(), buf_len);
            len = (int)strlen(buf);
            g_tf.collapse(new_pos, len);
        }

        // ── Ctrl+V: pegar ─────────────────────────────────────────────────────
        if (ctrl && IsKeyPressed(KEY_V)) {
            // Preferir clipboard del SO
            const char* so_clip = GetClipboardText();
            std::string clip = (so_clip && so_clip[0]) ? so_clip : g_tf_clipboard;
            // Filtrar solo imprimibles (sin \n en campos de línea única)
            std::string filtered;
            for (char c : clip)
                if (c >= 32 && c <= 126) filtered += c;
            if (!filtered.empty()) {
                if (g_tf.has_selection()) {
                    buf_erase(buf, g_tf.sel_min(), g_tf.sel_max(), buf_len);
                    g_tf.collapse(g_tf.sel_min(), (int)strlen(buf));
                }
                int ins = (int)filtered.size();
                buf_insert(buf, g_tf.cursor, filtered.c_str(), ins, buf_len);
                len = (int)strlen(buf);
                g_tf.collapse(g_tf.cursor + ins, len);
            }
        }

        // ── Flecha izquierda ──────────────────────────────────────────────────
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            if (shift) {
                g_tf.extend_sel(std::max(0, g_tf.cursor - 1), len);
            }
            else if (g_tf.has_selection()) {
                g_tf.set_cursor(g_tf.sel_min(), len);
            }
            else {
                g_tf.set_cursor(std::max(0, g_tf.cursor - 1), len);
            }
        }

        // ── Flecha derecha ────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            if (shift) {
                g_tf.extend_sel(std::min(len, g_tf.cursor + 1), len);
            }
            else if (g_tf.has_selection()) {
                g_tf.set_cursor(g_tf.sel_max(), len);
            }
            else {
                g_tf.set_cursor(std::min(len, g_tf.cursor + 1), len);
            }
        }

        // ── Home / Ctrl+flecha izq ────────────────────────────────────────────
        if (IsKeyPressed(KEY_HOME) || (ctrl && IsKeyPressed(KEY_LEFT))) {
            shift ? g_tf.extend_sel(0, len) : g_tf.set_cursor(0, len);
        }
        // ── End / Ctrl+flecha der ─────────────────────────────────────────────
        if (IsKeyPressed(KEY_END) || (ctrl && IsKeyPressed(KEY_RIGHT))) {
            shift ? g_tf.extend_sel(len, len) : g_tf.set_cursor(len, len);
        }

        // ── Backspace ─────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (g_tf.has_selection()) {
                int new_pos = g_tf.sel_min();
                buf_erase(buf, g_tf.sel_min(), g_tf.sel_max(), buf_len);
                g_tf.collapse(new_pos, (int)strlen(buf));
            }
            else if (g_tf.cursor > 0) {
                buf_erase(buf, g_tf.cursor - 1, g_tf.cursor, buf_len);
                g_tf.collapse(g_tf.cursor - 1, (int)strlen(buf));
            }
        }

        // ── Delete ────────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
            if (g_tf.has_selection()) {
                int new_pos = g_tf.sel_min();
                buf_erase(buf, g_tf.sel_min(), g_tf.sel_max(), buf_len);
                g_tf.collapse(new_pos, (int)strlen(buf));
            }
            else {
                len = (int)strlen(buf);
                if (g_tf.cursor < len) {
                    buf_erase(buf, g_tf.cursor, g_tf.cursor + 1, buf_len);
                    g_tf.collapse(g_tf.cursor, (int)strlen(buf));
                }
            }
        }

        // ── Enter: confirmar ──────────────────────────────────────────────────
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            active_id = -1;
            g_tf_active_id = -1;
            confirmed = true;
        }

        // ── Escape: cancelar foco ─────────────────────────────────────────────
        if (IsKeyPressed(KEY_ESCAPE)) {
            active_id = -1;
            g_tf_active_id = -1;
        }

        // ── Caracteres imprimibles ────────────────────────────────────────────
        if (!ctrl) {
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 126) {
                    if (g_tf.has_selection()) {
                        int new_pos = g_tf.sel_min();
                        buf_erase(buf, g_tf.sel_min(), g_tf.sel_max(), buf_len);
                        g_tf.collapse(new_pos, (int)strlen(buf));
                    }
                    char ch = (char)key;
                    buf_insert(buf, g_tf.cursor, &ch, 1, buf_len);
                    g_tf.collapse(g_tf.cursor + 1, (int)strlen(buf));
                }
                key = GetCharPressed();
            }
        }
    }

    // ── Dibujo del campo ──────────────────────────────────────────────────────
    Color border = is_active ? th.border_active
        : hov ? th.border_hover
        : th_alpha(th.border);
    DrawRectangleRec(r, th.bg_field);
    DrawRectangleLinesEx(r, 1.0f, border);

    // Texto truncado (scroll horizontal básico: mostrar sufijo si no cabe)
    const int pad = 5;
    int len = (int)strlen(buf);
    int text_x = x + pad;

    // Calcular offset de scroll: mantener cursor visible
    int cursor_px = MeasureTextF(std::string(buf, std::min(g_tf.cursor, len)).c_str(), font);
    int available = w - 2 * pad;
    static std::unordered_map<int, int>* s_scroll_map = nullptr;
    if (!s_scroll_map) s_scroll_map = new std::unordered_map<int, int>();
    int& scroll_off = (*s_scroll_map)[my_id];
    if (is_active) {
        if (cursor_px - scroll_off > available) scroll_off = cursor_px - available;
        if (cursor_px - scroll_off < 0)         scroll_off = cursor_px;
    }
    else {
        scroll_off = 0;
    }

    // ── Highlight de selección ────────────────────────────────────────────────
    if (is_active && g_tf.has_selection()) {
        int sx0 = MeasureTextF(std::string(buf, g_tf.sel_min()).c_str(), font) - scroll_off;
        int sx1 = MeasureTextF(std::string(buf, g_tf.sel_max()).c_str(), font) - scroll_off;
        sx0 = std::clamp(sx0, 0, available);
        sx1 = std::clamp(sx1, 0, available);
        DrawRectangle(text_x + sx0, y + 2, sx1 - sx0, h - 4,
            ColorAlpha(th.accent, 0.35f));
    }

    // ── Texto ─────────────────────────────────────────────────────────────────
    BeginScissorMode(x + 1, y + 1, w - 2, h - 2);
    DrawTextF(buf, text_x - scroll_off, y + (h - font) / 2, font, th.text_primary);
    EndScissorMode();

    // ── Cursor parpadeante ────────────────────────────────────────────────────
    if (is_active && !g_tf.has_selection() && (int)(GetTime() * 2) % 2 == 0) {
        int cx = text_x + cursor_px - scroll_off;
        if (cx >= x && cx <= x + w - 2)
            DrawLine(cx, y + 3, cx, y + h - 3, th.text_primary);
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

    if (cur_pw <= 0 || cur_ph <= 0) {
        cur_pw = std::max(min_pw, sw - 2 * MARGIN);
        cur_ph = std::max(min_ph, sh - TOOLBAR_H - 2 * MARGIN);
        pos_x = (sw - cur_pw) / 2;
        pos_y = TOOLBAR_H + (sh - TOOLBAR_H - cur_ph) / 2;
    }

    pos_x = std::clamp(pos_x, 0, std::max(0, sw - cur_pw));
    pos_y = std::clamp(pos_y, TOOLBAR_H, std::max(TOOLBAR_H, sh - cur_ph));
    int px = pos_x, py = pos_y;
    int pw = cur_pw, ph = cur_ph;

    DrawRectangle(0, 0, sw, sh, { 0, 0, 0, 120 });
    overlay::push_rect({ 0.0f, 0.0f, (float)sw, (float)sh });

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

    if (!resizing && !dragging && edge != 0 && !over_title
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        resizing = true;
        resize_edge = edge;
        resize_sx = (int)mouse.x;  resize_sy = (int)mouse.y;
        resize_ox = pos_x;         resize_oy = pos_y;
        resize_opw = cur_pw;        resize_oph = cur_ph;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        resizing = false;
        dragging = false;
        resize_edge = 0;
    }

    if (resizing) {
        int dx = (int)mouse.x - resize_sx;
        int dy = (int)mouse.y - resize_sy;

        if (resize_edge & 2)
            cur_pw = std::clamp(resize_opw + dx, min_pw, sw - pos_x);
        if (resize_edge & 8)
            cur_ph = std::clamp(resize_oph + dy, min_ph, sh - pos_y);
        if (resize_edge & 1) {
            int new_pw = std::clamp(resize_opw - dx, min_pw, resize_ox + resize_opw);
            pos_x = std::clamp(resize_ox + (resize_opw - new_pw), 0, sw - new_pw);
            cur_pw = new_pw;
        }
        if (resize_edge & 4) {
            int new_ph = std::clamp(resize_oph - dy, min_ph, resize_oy + resize_oph - TOOLBAR_H);
            pos_y = std::clamp(resize_oy + (resize_oph - new_ph), TOOLBAR_H, sh - new_ph);
            cur_ph = new_ph;
        }
        pw = cur_pw; ph = cur_ph;
        px = pos_x;  py = pos_y;
    }

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

    // ── Dibujo ────────────────────────────────────────────────────────────────
    if (th.use_transparency)
        DrawRectangle(px + 6, py + 6, pw, ph, { 0, 0, 0, 90 });

    DrawRectangle(px, py, pw, ph, th.bg_panel);
    DrawRectangleLinesEx({ (float)px,(float)py,(float)pw,(float)ph }, 1.5f, border_col);

    DrawRectangle(px, py, pw, HEADER_H, th.bg_panel_header);
    DrawTextF(title, px + 12, py + 9, 12, title_col);

    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 2; col++)
            DrawRectangle(px + pw / 2 - 4 + col * 5, py + 10 + row * 4, 2, 2,
                (over_title || dragging) ? th_alpha(th.ctrl_text_dim) : Color{ 0,0,0,0 });

    Color cc = { 80, 105, 200, 150 };
    DrawLine(px, py, px + CORNER_LEN, py, cc);
    DrawLine(px, py, px, py + CORNER_LEN, cc);
    DrawLine(px + pw, py, px + pw - CORNER_LEN, py, cc);
    DrawLine(px + pw, py, px + pw, py + CORNER_LEN, cc);
    DrawLine(px, py + ph, px + CORNER_LEN, py + ph, cc);
    DrawLine(px, py + ph, px, py + ph - CORNER_LEN, cc);
    DrawLine(px + pw, py + ph, px + pw - CORNER_LEN, py + ph, cc);
    DrawLine(px + pw, py + ph, px + pw, py + ph - CORNER_LEN, cc);

    if ((edge || resizing) && !over_title) {
        int ae = resizing ? resize_edge : edge;
        Color hc = resizing ? Color{ 120,160,255,200 } : Color{ 80,120,220,130 };
        if (ae & 1) DrawLine(px, py, px, py + ph, hc);
        if (ae & 2) DrawLine(px + pw, py, px + pw, py + ph, hc);
        if (ae & 4) DrawLine(px, py, px + pw, py, hc);
        if (ae & 8) DrawLine(px, py + ph, px + pw, py + ph, hc);
    }

    Rectangle cr = { (float)(px + pw - 28), (float)(py + 5), 22.0f, 22.0f };
    bool chov = CheckCollisionPointRec(mouse, cr);
    DrawRectangleRec(cr, chov ? th.close_bg_hover : th.close_bg);
    DrawTextF("x", (int)cr.x + 7, (int)cr.y + 4, 13, th.text_primary);

    return chov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}