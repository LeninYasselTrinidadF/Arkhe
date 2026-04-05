// ── docs_draw_helpers.cpp ────────────────────────────────────────────────────
// Implementa los helpers de dibujo declarados en docs_page.hpp.
// Incluido directamente por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/toolbar/docs_panel/docs_page.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "raylib.h"
#include <cstring>
#include <string>
#include <vector>

// ── Utilidad: word-wrap ───────────────────────────────────────────────────────

static std::vector<std::string> docs_wrap(const char* text, int max_w, int fs)
{
    std::vector<std::string> lines;
    std::string s(text);
    while (!s.empty()) {
        // Avanzar mientras quepan caracteres
        int chars = 1;
        while (chars < (int)s.size() &&
               MeasureTextF(s.substr(0, chars + 1).c_str(), fs) <= max_w)
            chars++;
        // Retroceder al último espacio si no llegamos al final
        if (chars < (int)s.size()) {
            int sp = (int)s.rfind(' ', chars);
            if (sp > 0) chars = sp;
        }
        lines.push_back(s.substr(0, chars));
        s = s.substr(chars);
        if (!s.empty() && s[0] == ' ') s = s.substr(1);
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

// ── Implementaciones ──────────────────────────────────────────────────────────

void docs_draw_sep(int lx, int lw, int y)
{
    DrawRectangle(lx, y, lw, 1, docs_col::sep);
}

void docs_draw_section_title(const char* text, int lx, int& y)
{
    const int fs = 11;
    int tw = MeasureTextF(text, fs);
    DrawTextF(text, lx, y, fs, docs_col::text_head);
    // Subrayado de acento
    DrawRectangle(lx, y + fs + 3, tw, 2, docs_col::accent);
    y += fs + 10;
}

int docs_draw_paragraph(const char* text, int lx, int lw, int& y,
                         Color col, int font_sz)
{
    auto lines = docs_wrap(text, lw, font_sz);
    int  lh    = font_sz + 3;
    for (auto& l : lines) {
        DrawTextF(l.c_str(), lx, y, font_sz, col);
        y += lh;
    }
    y += 4;
    return (int)lines.size() * lh + 4;
}

void docs_draw_code_block(const char* code, int lx, int lw, int& y)
{
    const int PAD = 8, fs = 10, lh = fs + 4;
    // Dividir por '\n'
    std::vector<std::string> lines;
    std::string s(code);
    size_t pos;
    while ((pos = s.find('\n')) != std::string::npos) {
        lines.push_back(s.substr(0, pos));
        s = s.substr(pos + 1);
    }
    lines.push_back(s);

    int total_h = (int)lines.size() * lh + PAD * 2;
    DrawRectangle(lx, y, lw, total_h, docs_col::bg_code);
    DrawRectangleLinesEx({ (float)lx, (float)y, (float)lw, (float)total_h },
                         1.f, docs_col::border);
    // Franja lateral izquierda
    DrawRectangle(lx, y, 3, total_h, docs_col::accent);

    int ty = y + PAD;
    for (auto& l : lines) {
        DrawTextF(l.c_str(), lx + PAD + 4, ty, fs, docs_col::text_code);
        ty += lh;
    }
    y += total_h + 6;
}

void docs_draw_kv_row(const char* key, const char* value,
                       int lx, int lw, int& y, Color key_col)
{
    const int fs = 10, row_h = fs + 8;
    DrawRectangle(lx, y, lw, row_h, docs_col::bg_card);
    DrawRectangleLinesEx({ (float)lx, (float)y, (float)lw, (float)row_h },
                         1.f, docs_col::border);
    DrawTextF(key,   lx + 8,  y + 4, fs, key_col);
    int kw = MeasureTextF(key, fs);
    DrawTextF(value, lx + kw + 16, y + 4, fs, docs_col::text_body);
    y += row_h + 2;
}

void docs_draw_card(const char* title, const char* desc,
                     int lx, int lw, int& y, Color accent_col)
{
    const int fs_t = 10, fs_d = 10, PAD = 8;
    int desc_lines = desc ? (int)docs_wrap(desc, lw - PAD * 2 - 6, fs_d).size() : 0;
    int card_h = PAD + fs_t + (desc_lines > 0 ? 4 + desc_lines * (fs_d + 3) : 0) + PAD;

    DrawRectangle(lx, y, lw, card_h, docs_col::bg_card);
    DrawRectangleLinesEx({ (float)lx, (float)y, (float)lw, (float)card_h },
                         1.f, docs_col::border);
    // Franja de acento
    DrawRectangle(lx, y, 3, card_h, accent_col);

    DrawTextF(title, lx + PAD + 4, y + PAD, fs_t, docs_col::text_head);
    if (desc && desc[0]) {
        int dy = y + PAD + fs_t + 4;
        docs_draw_paragraph(desc, lx + PAD + 4, lw - PAD * 2 - 6, dy, docs_col::text_body, fs_d);
    }
    y += card_h + 4;
}

void docs_draw_key_chip(const char* key, int x, int y_base, int& x_out)
{
    const int fs = 9, PAD_X = 6, PAD_Y = 3;
    int tw = MeasureTextF(key, fs);
    int cw = tw + PAD_X * 2, ch = fs + PAD_Y * 2;
    DrawRectangle(x, y_base, cw, ch, { 30, 38, 70, 255 });
    DrawRectangleLinesEx({ (float)x, (float)y_base, (float)cw, (float)ch },
                         1.f, docs_col::border_acc);
    DrawTextF(key, x + PAD_X, y_base + PAD_Y, fs, docs_col::text_key);
    x_out = x + cw + 4;
}

void docs_draw_shortcut_row(const std::vector<const char*>& keys,
                              const char* desc,
                              int lx, int lw, int& y)
{
    const int row_h = 20, fs_d = 10;
    // Calcular x final de los chips
    int cx = lx;
    for (auto* k : keys) {
        const int fs = 9, PAD_X = 6;
        cx += MeasureTextF(k, fs) + PAD_X * 2 + 4;
    }
    cx += 6; // separador antes de la descripción

    // Fondo de fila
    DrawRectangle(lx, y, lw, row_h, docs_col::bg_card);
    DrawRectangleLinesEx({ (float)lx, (float)y, (float)lw, (float)row_h },
                         1.f, docs_col::border);

    int chip_y = y + (row_h - (9 + 6)) / 2;
    int x = lx + 6;
    for (auto* k : keys) {
        docs_draw_key_chip(k, x, chip_y, x);
    }
    DrawTextF(desc, x + 2, y + (row_h - fs_d) / 2, fs_d, docs_col::text_body);
    y += row_h + 2;
}
