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
    const char*    label;
    int            field_id;
};

static ToolbarNavItem s_items[MAX_ITEMS];
static int            s_count          = 0;
static int            s_focused_idx    = -1;
static int            s_activated      = -1;
static int            s_prev_activated = -1;

// ── API de registro ───────────────────────────────────────────────────────────

void kbnav_toolbar_begin_frame() {
    s_prev_activated = s_activated;
    s_activated      = -1;
    s_count          = 0;
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
        return ToolbarNavKind::TextField;
    return s_items[s_focused_idx].kind;
}

int kbnav_toolbar_focused_idx() {
    return (s_count > 0) ? s_focused_idx : -1;
}

// ── apply_bridge ─────────────────────────────────────────────────────────────
// Sincroniza foco con active_field.
//
// Para corregir el problema de "Tab da apariencia de foco pero requiere clic
// adicional para capturar texto":
//   · Al enfocar un TextField vía Tab se activa force_field_activate = true.
//   · draw_text_field en PanelWidget debe leer este flag y, cuando sea true,
//     activar la captura de chars sin esperar un clic, consumiéndolo
//     (force_field_activate = false) inmediatamente después de procesarlo.
//
// ⚠  PENDIENTE: modificar PanelWidget::draw_text_field para consumir este flag.
//    Leer state.toolbar.force_field_activate al inicio del método y, si es true
//    y (aid == field_id), saltar el requisito del clic y poner force_field_activate = false.

static void apply_bridge(AppState& state) {
    if (s_focused_idx < 0 || s_focused_idx >= s_count) return;
    const ToolbarNavItem& it = s_items[s_focused_idx];

    if (it.kind == ToolbarNavKind::TextField && it.field_id >= 0) {
        state.toolbar.active_field       = it.field_id;
        state.toolbar.force_field_activate = true;  // ← fix Tab
    }
    else if (it.kind != ToolbarNavKind::Textarea) {
        if (state.toolbar.active_field >= 0)
            state.toolbar.active_field = -1;
        state.toolbar.force_field_activate = false;
    }
}

// ── kbnav_toolbar_handle ──────────────────────────────────────────────────────

bool kbnav_toolbar_handle(AppState& state, bool textarea_active) {
    if (s_count == 0) return false;

    if (s_focused_idx >= s_count) s_focused_idx = s_count - 1;

    bool consumed = false;
    bool shift    = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // ── Tab / Shift+Tab ───────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_TAB)) {
        if (s_focused_idx < 0) {
            s_focused_idx = 0;
        } else if (shift) {
            s_focused_idx = (s_focused_idx - 1 + s_count) % s_count;
        } else {
            s_focused_idx = (s_focused_idx + 1) % s_count;
        }
        apply_bridge(state);
        consumed = true;
    }

    // ── Enter ─────────────────────────────────────────────────────────────────
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

    // ── Escape: liberar foco ──────────────────────────────────────────────────
    if (IsKeyPressed(KEY_ESCAPE) && s_focused_idx >= 0) {
        if (state.toolbar.active_field >= 0)
            state.toolbar.active_field = -1;
        state.toolbar.force_field_activate = false;
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

    DrawRectangleLinesEx(it.rect, 2.f, ColorAlpha(th.accent, alpha));
    DrawRectangleLinesEx(
        { it.rect.x - 3.f, it.rect.y - 3.f,
          it.rect.width  + 6.f, it.rect.height + 6.f },
        1.f, ColorAlpha(th.accent, alpha * 0.25f));

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

    const char* hint = action_hint(it.kind);
    if (hint && hint[0]) {
        int hfs = 9;
        DrawTextF(hint,
            (int)(it.rect.x + it.rect.width - MeasureTextF(hint, hfs)),
            (int)(it.rect.y + it.rect.height + 3),
            hfs, ColorAlpha(th.text_dim, 0.55f));
    }
}
