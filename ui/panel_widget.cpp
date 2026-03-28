#include "panel_widget.hpp"
#include "theme.hpp"
#include <cstring>

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

bool PanelWidget::draw_text_field(char* buf, int buf_len,
    int x, int y, int w, int h, int font,
    int& active_id, int my_id, Vector2 mouse)
{
    const Theme& th = g_theme;
    bool is_active = (active_id == my_id);
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = CheckCollisionPointRec(mouse, r);

    Color border = is_active ? th.border_active
                 : hov       ? th.border_hover
                             : th_alpha(th.border);
    DrawRectangleRec(r, th.bg_field);
    DrawRectangleLinesEx(r, 1.0f, border);

    std::string shown = buf;
    while (MeasureText(shown.c_str(), font) > w - 10 && shown.size() > 3)
        shown = ".." + shown.substr(shown.size() - shown.size() / 2);
    DrawText(shown.c_str(), x + 5, y + (h - font) / 2, font, th.text_primary);

    if (is_active && (int)(GetTime() * 2) % 2 == 0) {
        int cw = MeasureText(buf, font);
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

void PanelWidget::draw_labeled_field(const char* label, char* buf, int buf_len,
    int x, int y, int w, int font,
    int& active_id, int my_id, Vector2 mouse)
{
    DrawText(label, x, y, 10, th_alpha(g_theme.text_dim));
    draw_text_field(buf, buf_len, x, y + 14, w, font + 4, font, active_id, my_id, mouse);
}

void PanelWidget::draw_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureText(text, 11);
    DrawRectangle(x, y, tw + 14, 20, th_alpha(bg));
    DrawText(text, x + 7, y + 5, 11, fg);
}

int PanelWidget::draw_wrapped_text(const char* text, int x, int y,
    int max_w, int font_size, Color color)
{
    std::string s(text);
    int line_y = y;
    while (!s.empty()) {
        int chars = 1;
        while (chars < (int)s.size() &&
            MeasureText(s.substr(0, chars + 1).c_str(), font_size) < max_w)
            chars++;
        if (chars < (int)s.size() && s[chars] != ' ') {
            int sp = (int)s.rfind(' ', chars);
            if (sp > 0) chars = sp;
        }
        DrawText(s.substr(0, chars).c_str(), x, line_y, font_size, color);
        s = s.substr(chars);
        if (!s.empty() && s[0] == ' ') s = s.substr(1);
        line_y += font_size + 4;
    }
    return line_y;
}

void PanelWidget::draw_h_line(int x1, int y, int x2, Color color) {
    DrawLine(x1, y, x2, y, color);
}

bool PanelWidget::draw_window_frame(int px, int py, int pw, int ph,
    const char* title, Color title_col, Color border_col, Vector2 mouse) const
{
    const Theme& th = g_theme;
    if (th.use_transparency)
        DrawRectangle(px + 4, py + 4, pw, ph, th.shadow);
    DrawRectangle(px, py, pw, ph, th.bg_panel);
    DrawRectangleLinesEx({ (float)px,(float)py,(float)pw,(float)ph }, 1.5f, border_col);
    DrawRectangle(px, py, pw, 30, th.bg_panel_header);
    DrawText(title, px + 12, py + 9, 12, title_col);
    Rectangle cr = { (float)(px + pw - 28),(float)(py + 5),22.0f,22.0f };
    bool chov = CheckCollisionPointRec(mouse, cr);
    DrawRectangleRec(cr, chov ? th.close_bg_hover : th.close_bg);
    DrawText("x", (int)cr.x + 7, (int)cr.y + 4, 13, th.text_primary);
    return chov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}