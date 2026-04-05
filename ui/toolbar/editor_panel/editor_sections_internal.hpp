#pragma once
// ── editor_sections_internal.hpp ─────────────────────────────────────────────
// Incluido SOLO por los .cpp de implementación de editor_sections.
// No exponer al resto del proyecto.
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/toolbar/editor_panel/editor_sections.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/constants.hpp"
#include "ui/widgets/panel_widget.hpp"
#include "ui/key_controls/kbnav_toolbar/kbnav_toolbar.hpp"
#include "raylib.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// ── BodyCursor ──────────────────────────────────────────────────────────────
// Minimal cursor/selection helper used by the inline editor modules.
struct BodyCursor {
    int cursor_pos = 0;
    int sel_from = 0;
    int sel_to = 0;

    bool has_selection() const { return sel_from != sel_to; }
    int sel_min() const { return std::min(sel_from, sel_to); }
    int sel_max() const { return std::max(sel_from, sel_to); }
    void set_cursor(int p) { cursor_pos = p; sel_from = sel_to = p; }
    void extend_sel(int p) { sel_to = p; cursor_pos = p; }
};

using RL = ::Color;

// ── IDs de campos de texto ────────────────────────────────────────────────────

enum FieldId {
    F_NOTE      =  0,
    F_TEX       =  1,
    F_MSC       =  2,
    F_KIND      = 10,
    F_TITLE     = 11,
    F_CONTENT   = 12,
    F_XREF_CODE = 20,   // campo de código en el formulario de cross-refs
};

// ── Constantes de draw_labeled_field ─────────────────────────────────────────

static constexpr int LABELED_FIELD_H      = 20;
static constexpr int LABELED_FIELD_OFFSET = 16;

// ── draw_section_bar ──────────────────────────────────────────────────────────
// Dibuja una barra de sección colapsable con indicador ▶/▼, título, dirty (*),
// y un botón «Guardar» opcional en el extremo derecho.
//
// Retorna true si la sección está expandida.
// Si show_save && save_clicked != nullptr, pone *save_clicked = true al pulsarlo.
// El clic en el área de la barra (fuera del botón Guardar) alterna expanded.
// ─────────────────────────────────────────────────────────────────────────────

static inline bool draw_section_bar(
    const char* title,
    bool&       expanded,
    int lx, int lw, int& y,
    Vector2 mouse,
    bool  dirty      = false,
    bool  show_save  = false,
    bool* save_clicked = nullptr)
{
    static constexpr int BAR_H  = 22;
    static constexpr int SAVE_W = 54;

    // Rectángulo del botón Guardar
    Rectangle save_r = {};
    bool has_save = show_save && save_clicked;
    if (has_save)
        save_r = { (float)(lx + lw - SAVE_W - 4), (float)(y + 2),
                   (float)SAVE_W, (float)(BAR_H - 4) };

    Rectangle bar_r = { (float)lx, (float)y, (float)lw, (float)BAR_H };

    // Hover solo en zona de toggle (fuera del botón guardar)
    bool bar_hov = CheckCollisionPointRec(mouse, bar_r)
                && !CheckCollisionPointRec(mouse, save_r);

    Color bar_bg = expanded ? RL{ 32, 38, 68, 255 } : RL{ 22, 27, 48, 255 };
    if (bar_hov) bar_bg = RL{ 42, 50, 85, 255 };

    DrawRectangleRec(bar_r, bar_bg);
    DrawRectangleLinesEx(bar_r, 1.f, { 55, 68, 125, 190 });

    // Indicador de expansión
    DrawTextF(expanded ? "v" : ">",
        lx + 6, y + (BAR_H - 11) / 2, 11, { 120, 148, 222, 225 });

    // Título + dirty marker
    char full[128];
    snprintf(full, sizeof(full), "%s%s", title, dirty ? "  *" : "");
    DrawTextF(full, lx + 22, y + (BAR_H - 11) / 2, 11, { 175, 198, 255, 238 });

    // Botón Guardar
    if (has_save) {
        bool shov = CheckCollisionPointRec(mouse, save_r);
        DrawRectangleRec(save_r, shov ? RL{ 55, 122, 65, 255 } : RL{ 32, 74, 40, 255 });
        DrawRectangleLinesEx(save_r, 1.f, { 65, 152, 76, 205 });
        DrawTextF("Guardar", (int)save_r.x + 4, (int)save_r.y + 3, 10, WHITE);
        if (shov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            *save_clicked = true;
    }

    // Toggle
    if (bar_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        expanded = !expanded;

    y += BAR_H + 2;
    return expanded;
}
