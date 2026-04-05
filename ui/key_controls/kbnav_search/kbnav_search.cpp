#include "ui/key_controls/kbnav_search/kbnav_search.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "data/search/loogle.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// ── Registro de ítems por frame ───────────────────────────────────────────────

static constexpr int MAX_ITEMS = 64;

struct SearchNavItem {
    SearchNavKind kind;
    Rectangle     rect;
};

static SearchNavItem s_items[MAX_ITEMS];
static int           s_count        = 0;  // ítems registrados este frame
static int           s_activated    = -1; // idx activado este frame (por handle)
static int           s_prev_activated = -1; // idx activado el frame anterior (visible en draw)

// ── API de registro ───────────────────────────────────────────────────────────

void kbnav_search_begin_frame() {
    s_prev_activated = s_activated;   // exponer activación del frame anterior
    s_activated      = -1;
    s_count          = 0;
}

int kbnav_search_register(SearchNavKind kind, Rectangle rect) {
    if (s_count >= MAX_ITEMS) return s_count - 1;
    int idx = s_count++;
    s_items[idx] = { kind, rect };
    return idx;
}

// ── API de consulta ───────────────────────────────────────────────────────────

bool kbnav_search_is_focused(int idx) {
    return idx >= 0 && g_kbnav.in(FocusZone::Search) && g_kbnav.search_idx == idx;
}

bool kbnav_search_is_activated(int idx) {
    return idx >= 0 && s_prev_activated == idx;
}

bool kbnav_search_focused_is(SearchNavKind kind) {
    if (!g_kbnav.in(FocusZone::Search)) return false;
    int idx = g_kbnav.search_idx;
    if (idx < 0 || idx >= s_count) return false;
    return s_items[idx].kind == kind;
}

// ── kbnav_search_handle ───────────────────────────────────────────────────────
// Llamado al FINAL de search_panel::draw, cuando s_count ya es definitivo.

bool kbnav_search_handle(AppState& state, const MathNode* /*search_root*/) {
    if (!g_kbnav.in(FocusZone::Search)) return false;
    if (s_count == 0) return false;

    // Clamp (los ítems pueden cambiar frame a frame)
    g_kbnav.search_idx = std::clamp(g_kbnav.search_idx, 0, s_count - 1);

    bool consumed = false;

    // ── Navegación Up/Down/Left/Right ─────────────────────────────────────────
    bool go_prev = IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_LEFT);
    bool go_next = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_RIGHT);

    if (go_prev) {
        g_kbnav.search_idx = (g_kbnav.search_idx - 1 + s_count) % s_count;
        consumed = true;
    }
    if (go_next) {
        g_kbnav.search_idx = (g_kbnav.search_idx + 1) % s_count;
        consumed = true;
    }

    // ── Enter: activar ítem ───────────────────────────────────────────────────
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        SearchNavKind k = s_items[g_kbnav.search_idx].kind;
        switch (k) {
        // Los campos de texto manejan su propio Enter (loogle_search_async, etc.)
        // Solo activamos los ítems no-campo.
        case SearchNavKind::LocalField:
        case SearchNavKind::LoogleField:
            break;
        default:
            s_activated = g_kbnav.search_idx;
            break;
        }
        consumed = true;
    }

    return consumed;
}

// ── kbnav_search_draw ─────────────────────────────────────────────────────────

static const char* kind_label(SearchNavKind k) {
    switch (k) {
    case SearchNavKind::LocalField:    return "Local";
    case SearchNavKind::LocalButton:   return "Buscar";
    case SearchNavKind::LocalResult:   return "Resultado";
    case SearchNavKind::LocalPagerPrev:return "Anterior";
    case SearchNavKind::LocalPagerNext:return "Siguiente";
    case SearchNavKind::LoogleField:   return "Loogle";
    case SearchNavKind::LoogleButton:  return "Buscar";
    case SearchNavKind::LoogleResult:  return "Resultado";
    case SearchNavKind::LooglePagerPrev:return "Anterior";
    case SearchNavKind::LooglePagerNext:return "Siguiente";
    }
    return "";
}

void kbnav_search_draw() {
    if (!g_kbnav.in(FocusZone::Search)) return;
    if (s_count == 0) return;

    int idx = std::clamp(g_kbnav.search_idx, 0, s_count - 1);
    const SearchNavItem& item = s_items[idx];
    const Theme& th = g_theme;

    // ── Borde pulsante sobre el ítem activo ───────────────────────────────────
    const float t     = (float)GetTime();
    const float alpha = 0.55f + 0.45f * sinf(t * 5.f);
    DrawRectangleLinesEx(item.rect, 2.f, ColorAlpha(th.accent, alpha));
    DrawRectangleLinesEx(
        { item.rect.x - 2.f, item.rect.y - 2.f,
          item.rect.width + 4.f, item.rect.height + 4.f },
        1.f, ColorAlpha(th.accent, alpha * 0.35f));

    // ── Label flotante encima del borde superior ──────────────────────────────
    const char* lbl  = kind_label(item.kind);
    const int   lfs  = 9;
    const int   lw   = MeasureTextF(lbl, lfs) + 10;
    const int   lh   = lfs + 6;
    Rectangle label_r = { item.rect.x, item.rect.y - (float)lh - 2.f,
                           (float)lw,  (float)lh };
    DrawRectangleRec(label_r, ColorAlpha(th.accent, 0.20f));
    DrawRectangleLinesEx(label_r, 1.f, ColorAlpha(th.accent, 0.55f));
    DrawTextF(lbl, (int)(label_r.x + 5), (int)(label_r.y + (lh - lfs) / 2),
              lfs, th.accent);

    // ── Hint de navegación ────────────────────────────────────────────────────
    const char* hint = "\xE2\x86\x91\xE2\x86\x93 navegar";   // ↑↓
    const int   hfs  = 9;
    DrawTextF(hint,
              (int)(item.rect.x + item.rect.width - MeasureTextF(hint, hfs)),
              (int)(item.rect.y + item.rect.height + 3),
              hfs, ColorAlpha(th.text_dim, 0.55f));
}
