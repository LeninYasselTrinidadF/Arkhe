#include "ui/key_controls/kbnav_toolbar/kbnav_toolbar.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

// ── Tabla de ítems registrados por frame ──────────────────────────────────────

static constexpr int MAX_ITEMS = 96;

struct ToolbarNavItem {
    ToolbarNavKind kind;
    Rectangle      rect;
    const char*    label;    // puntero a literal; válido durante el frame
    int            field_id; // para TextField: ID de active_field; -1 si n/a
};

static ToolbarNavItem s_items[MAX_ITEMS];
static int            s_count          = 0;

// Foco: persiste entre frames para que el usuario no lo pierda al redibujar.
static int            s_focused_idx    = -1;

// Activación: patrón de doble buffer (mismo mecanismo que kbnav_search).
static int            s_activated      = -1;  // activado ESTE frame
static int            s_prev_activated = -1;  // activado el frame ANTERIOR

// ── API de registro ───────────────────────────────────────────────────────────

void kbnav_toolbar_begin_frame() {
    s_prev_activated = s_activated;
    s_activated      = -1;
    s_count          = 0;
    // s_focused_idx NO se resetea: el foco debe persistir frame a frame.
}

int kbnav_toolbar_register(ToolbarNavKind kind, Rectangle rect,
                            const char* label, int field_id)
{
    if (s_count >= MAX_ITEMS) return MAX_ITEMS - 1;
    int idx   = s_count++;
    s_items[idx] = { kind, rect, label ? label : "", field_id };
    return idx;
}

// ── API de consulta ───────────────────────────────────────────────────────────

bool kbnav_toolbar_is_focused(int idx) {
    return idx >= 0 && s_count > 0 && s_focused_idx == idx;
}

bool kbnav_toolbar_is_activated(int idx) {
    return idx >= 0 && s_prev_activated == idx;
}

ToolbarNavKind kbnav_toolbar_focused_kind() {
    if (s_focused_idx < 0 || s_focused_idx >= s_count)
        return ToolbarNavKind::TextField; // valor por defecto; no usado si idx==-1
    return s_items[s_focused_idx].kind;
}

int kbnav_toolbar_focused_idx() {
    return (s_count > 0) ? s_focused_idx : -1;
}

// ── Bridge interno: sincroniza foco con active_field ─────────────────────────
//
// Llamado desde handle() después de cada cambio de foco por Tab.
// · TextField  → pone state.toolbar.active_field = field_id
//                (permite que draw_labeled_field / draw_text_field lo vean
//                 en el frame SIGUIENTE; el draw del frame actual ya tiene
//                 kbnav_toolbar_is_focused() para el highlight visual).
// · Otros      → limpia active_field para soltar cualquier campo de texto.

static void apply_bridge(AppState& state) {
    if (s_focused_idx < 0 || s_focused_idx >= s_count) return;
    const ToolbarNavItem& it = s_items[s_focused_idx];

    if (it.kind == ToolbarNavKind::TextField && it.field_id >= 0) {
        state.toolbar.active_field = it.field_id;
    }
    else if (it.kind != ToolbarNavKind::Textarea) {
        // Al llegar a un botón, quitar el foco de los campos de texto.
        // La Textarea gestiona body_active en draw_body_section.
        if (state.toolbar.active_field >= 0)
            state.toolbar.active_field = -1;
    }
}

// ── kbnav_toolbar_handle ──────────────────────────────────────────────────────

bool kbnav_toolbar_handle(AppState& state, bool textarea_active) {
    if (s_count == 0) return false;

    // Clamp por si el recuento de ítems cambió (cambio de nodo, panel, etc.)
    if (s_focused_idx >= s_count) s_focused_idx = s_count - 1;

    bool consumed = false;
    bool shift    = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // ── Tab / Shift+Tab: ciclar ítems ─────────────────────────────────────────
    if (IsKeyPressed(KEY_TAB)) {
        if (s_focused_idx < 0) {
            // Primera pulsación de Tab: ir al primer ítem.
            s_focused_idx = 0;
        } else if (shift) {
            s_focused_idx = (s_focused_idx - 1 + s_count) % s_count;
        } else {
            s_focused_idx = (s_focused_idx + 1) % s_count;
        }
        apply_bridge(state);
        consumed = true;
    }

    // ── Enter: activar Button / BrowseButton / FileItem ───────────────────────
    // No actúa cuando la textarea está capturando (textarea_active == true)
    // porque Enter en ese contexto inserta \n en el buffer.
    if (!textarea_active &&
        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)))
    {
        if (s_focused_idx >= 0 && s_focused_idx < s_count) {
            switch (s_items[s_focused_idx].kind) {
            case ToolbarNavKind::Button:
            case ToolbarNavKind::BrowseButton:
            case ToolbarNavKind::FileItem:
                s_activated = s_focused_idx;
                consumed = true;
                break;
            default:
                break;
            }
        }
    }

    // ── Escape: liberar foco (sin cerrar el panel) ────────────────────────────
    // Escape con paneles modales abiertos ya no navega por el mapa (ver main.cpp),
    // así que podemos interceptarlo aquí para limpiar el foco de campo.
    if (IsKeyPressed(KEY_ESCAPE) && s_focused_idx >= 0) {
        if (state.toolbar.active_field >= 0)
            state.toolbar.active_field = -1;
        s_focused_idx = -1;
        consumed = true;
    }

    return consumed;
}

// ── kbnav_toolbar_draw ────────────────────────────────────────────────────────

static const char* action_hint(ToolbarNavKind k) {
    switch (k) {
    case ToolbarNavKind::TextField:     return "Tab: siguiente campo";
    case ToolbarNavKind::Textarea:      return "Tab: salir del editor";
    case ToolbarNavKind::Button:        return "Enter: activar";
    case ToolbarNavKind::BrowseButton:  return "Enter: explorar";
    case ToolbarNavKind::FileItem:      return "Enter: seleccionar";
    }
    return "";
}

void kbnav_toolbar_draw() {
    if (s_focused_idx < 0 || s_focused_idx >= s_count) return;

    const ToolbarNavItem& it = s_items[s_focused_idx];
    const Theme& th = g_theme;
    float t     = (float)GetTime();
    float alpha = 0.55f + 0.45f * sinf(t * 5.f);

    // ── Borde pulsante ────────────────────────────────────────────────────────
    DrawRectangleLinesEx(it.rect, 2.f, ColorAlpha(th.accent, alpha));

    // ── Glow exterior tenue ───────────────────────────────────────────────────
    DrawRectangleLinesEx(
        { it.rect.x - 3.f, it.rect.y - 3.f,
          it.rect.width  + 6.f, it.rect.height + 6.f },
        1.f, ColorAlpha(th.accent, alpha * 0.25f));

    // ── Etiqueta del ítem (chip encima del borde superior) ────────────────────
    const char* lbl = (it.label && it.label[0]) ? it.label
                                                 : action_hint(it.kind);
    if (lbl && lbl[0]) {
        int lfs = 9;
        int lw  = MeasureTextF(lbl, lfs) + 10;
        int lh  = lfs + 6;
        float lx = it.rect.x;
        float ly = it.rect.y - (float)lh - 2.f;
        DrawRectangleRec({ lx, ly, (float)lw, (float)lh },
                         ColorAlpha(th.accent, 0.22f));
        DrawRectangleLinesEx({ lx, ly, (float)lw, (float)lh },
                             1.f, ColorAlpha(th.accent, 0.55f));
        DrawTextF(lbl, (int)(lx + 5), (int)(ly + (lh - lfs) / 2), lfs, th.accent);
    }

    // ── Hint de acción (debajo del borde inferior) ────────────────────────────
    const char* hint = action_hint(it.kind);
    if (hint && hint[0]) {
        int hfs = 9;
        DrawTextF(hint,
            (int)(it.rect.x + it.rect.width - MeasureTextF(hint, hfs)),
            (int)(it.rect.y + it.rect.height + 3),
            hfs, ColorAlpha(th.text_dim, 0.55f));
    }
}
