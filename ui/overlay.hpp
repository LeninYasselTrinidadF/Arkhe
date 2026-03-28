#pragma once
#include "raylib.h"
#include <vector>

// ── Overlay / Panel blocker ────────────────────────────────────────────────────
// Cada frame, los paneles flotantes registran su rectángulo con
// overlay_push_rect(). Al final del frame se limpia con overlay_clear().
//
// Cualquier subsistema (bubble_view, search_panel, divisor) debe consultar
// overlay_blocks_mouse(mouse) antes de procesar clicks o arrastre.
//
// Uso en panel:
//   overlay_push_rect({px, py, pw, ph});   // dentro de draw(), tras dibujar
//
// Uso en consumidores:
//   if (overlay_blocks_mouse(mouse)) return;  // al inicio de su sección de input
//
// overlay_clear() debe llamarse una vez por frame, antes de dibujar paneles.
// ─────────────────────────────────────────────────────────────────────────────

namespace overlay {

inline std::vector<Rectangle>& _rects() {
    static std::vector<Rectangle> v;
    return v;
}

// Registra un rectángulo que "bloquea" el mouse para el frame actual.
inline void push_rect(Rectangle r) {
    _rects().push_back(r);
}

// Devuelve true si el mouse está sobre algún panel flotante activo.
inline bool blocks_mouse(Vector2 mouse) {
    for (auto& r : _rects())
        if (CheckCollisionPointRec(mouse, r)) return true;
    return false;
}

// Limpia todos los rects registrados. Llamar una vez al inicio del frame
// (antes de draw_toolbar y paneles flotantes).
inline void clear() {
    _rects().clear();
}

} // namespace overlay
