#include "ui/key_controls/kbnav_search/kbnav_search.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "data/search/loogle.hpp"   // loogle_search_async
#include "raylib.h"
#include <cstring>

// ── kbnav_search_handle ───────────────────────────────────────────────────────
//
// Contrato de llamada:
//   • Se invoca ANTES de draw_search_panel, una vez por frame.
//   • Solo actúa cuando g_kbnav.in(FocusZone::Search) == true.
//   • Escribe caracteres en state.search_buf o state.loogle_buf según sub-zona.
//   • Enter en sub-zona 0 → no hace nada especial (búsqueda reactiva).
//   • Enter en sub-zona 1 → lanza loogle_search_async.
//   • Left/Right → cambia entre sub-zonas (espejo de up/down en toolbar).

bool kbnav_search_handle(AppState& state, const MathNode* /*search_root*/) {
    if (!g_kbnav.in(FocusZone::Search)) return false;

    bool consumed = false;

    // ── Cambiar sub-zona con Left/Right (o Up/Down) ───────────────────────────
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) {
        g_kbnav.search_idx = (g_kbnav.search_idx - 1 + 2) % 2;
        consumed = true;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN)) {
        g_kbnav.search_idx = (g_kbnav.search_idx + 1) % 2;
        consumed = true;
    }

    // ── Escritura de caracteres en el buffer activo ───────────────────────────
    char* buf  = (g_kbnav.search_idx == 0) ? state.search_buf  : state.loogle_buf;
    int   cap  = 255;

    // Caracteres imprimibles
    int key = GetCharPressed();
    while (key > 0) {
        int len = (int)strlen(buf);
        if (key >= 32 && key <= 125 && len < cap) {
            buf[len] = (char)key;
            buf[len + 1] = '\0';
            consumed = true;
        }
        key = GetCharPressed();
    }

    // Borrar
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) { buf[len - 1] = '\0'; consumed = true; }
    }

    // Enter
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        if (g_kbnav.search_idx == 1 && strlen(state.loogle_buf) > 0) {
            loogle_search_async(state, state.loogle_buf);
        }
        // sub-zona 0: la búsqueda local es reactiva, no necesita acción extra.
        consumed = true;
    }

    return consumed;
}

// ── kbnav_search_draw ─────────────────────────────────────────────────────────

void kbnav_search_draw(int px, int pw,
                       int field_y_local,
                       int field_y_loogle,
                       int field_h,
                       int btn_x, int btn_w)
{
    if (!g_kbnav.in(FocusZone::Search)) return;
    const Theme& th = g_theme;

    // Rectángulo que se resalta según sub-zona
    Rectangle r;
    if (g_kbnav.search_idx == 0) {
        // Campo local
        r = { (float)px, (float)field_y_local, (float)pw, (float)field_h };
    } else {
        // Campo + botón Loogle (ancho completo)
        r = { (float)px, (float)field_y_loogle,
              (float)(btn_x - px + btn_w), (float)field_h };
    }

    // Borde pulsante
    float t     = (float)GetTime();
    float alpha = 0.55f + 0.45f * sinf(t * 5.f);
    DrawRectangleLinesEx(r, 2.f, ColorAlpha(th.accent, alpha));

    // Segunda capa exterior (glow suave)
    DrawRectangleLinesEx(
        { r.x - 2.f, r.y - 2.f, r.width + 4.f, r.height + 4.f },
        1.f, ColorAlpha(th.accent, alpha * 0.35f));
}
