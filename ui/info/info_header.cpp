#include "ui/info/info_header.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/skin.hpp"
#include "ui/constants.hpp"
#include "data/math_node.hpp"

// ── Chip helper local ─────────────────────────────────────────────────────────
// Mismo comportamiento que draw_chip en panel_widget, pero accesible aquí
// sin tener un PanelWidget instanciado.
static void header_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureTextF(text, 11);
    int cw = tw + 14, ch = 20;
    if (g_skin.card.valid())
        g_skin.card.draw((float)x, (float)y, (float)cw, (float)ch, bg);
    else
        DrawRectangle(x, y, cw, ch, th_alpha(bg));
    DrawTextF(text, x + 7, y + 5, 11, fg);
}

// ── draw_info_header ──────────────────────────────────────────────────────────

void draw_info_header(AppState& state, int top, int w) {
    const Theme& th = g_theme;
    const int px = 18;

    // ── Breadcrumb ────────────────────────────────────────────────────────────
    int tx = px;
    for (int i = 1; i < (int)state.nav_stack.size(); i++) {
        std::string crumb = state.nav_stack[i]->label;
        if ((int)crumb.size() > 22) crumb = crumb.substr(0, 21) + ".";

        Color cc = (i == (int)state.nav_stack.size() - 1)
                 ? th.accent_hover
                 : th_alpha(th.text_dim);

        DrawTextF(crumb.c_str(), tx, top + 8, 11, cc);
        tx += MeasureTextF(crumb.c_str(), 11);

        if (i < (int)state.nav_stack.size() - 1) {
            DrawTextF("  >  ", tx, top + 8, 11, th_alpha(th.text_dim));
            tx += MeasureTextF("  >  ", 11);
        }
        if (tx > w - 200) {
            DrawTextF("...", tx, top + 8, 11, th_alpha(th.text_dim));
            break;
        }
    }

    // ── Sin selección ─────────────────────────────────────────────────────────
    if (state.selected_code.empty()) {
        DrawTextF("Selecciona una burbuja", px, top + 28, 18, th_alpha(th.text_dim));
        DrawTextF("para ver su informacion detallada.", px, top + 52, 12,
                 th_alpha(th.text_dim));
        return;
    }

    // ── Título ────────────────────────────────────────────────────────────────
    std::string title = state.selected_label;
    if ((int)title.size() > 55) title = title.substr(0, 54) + "...";
    DrawTextF(title.c_str(), px, top + 24, 20, th.text_primary);

    // ── Chips: código / nivel / modo ──────────────────────────────────────────
    // Chip de código
    header_chip(state.selected_code.c_str(), px, top + 52,
                { th.accent.r, th.accent.g, th.accent.b, 60 },
                th.accent_hover);

    // Chip de nivel
    const char* level_str = "-";
    MathNode* cur = state.current();
    if (cur) {
        for (auto& child : cur->children) {
            if (child->code == state.selected_code) {
                switch (child->level) {
                case NodeLevel::Macro:      level_str = "Macro-area"; break;
                case NodeLevel::Section:    level_str = "Seccion";    break;
                case NodeLevel::Subsection: level_str = "Sub-area";   break;
                case NodeLevel::Topic:      level_str = "Tema";       break;
                default: break;
                }
                break;
            }
        }
    }
    int chip_w = MeasureTextF(state.selected_code.c_str(), 11) + 14 + 14;
    header_chip(level_str, px + chip_w, top + 52,
                th_alpha(th.bg_button),
                th_alpha(th.ctrl_text_dim));

    // Chip de modo
    const char* mode_badge =
        state.mode == ViewMode::Mathlib  ? "MATHLIB"   :
        state.mode == ViewMode::Standard ? "ESTANDAR"  : "MSC2020";

    Color mode_col =
        state.mode == ViewMode::Mathlib  ? th.success  :
        state.mode == ViewMode::Standard ? th.warning   : th.accent;

    int chip2_w = MeasureTextF(level_str, 11) + 14 + 14;
    header_chip(mode_badge,
                px + chip_w + chip2_w, top + 52,
                { mode_col.r, mode_col.g, mode_col.b, 40 },
                mode_col);
}
