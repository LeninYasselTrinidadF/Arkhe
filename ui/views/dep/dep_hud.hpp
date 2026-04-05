#pragma once
// ── dep_hud.hpp ───────────────────────────────────────────────────────────────
// HUD superpuesto en dep_view (fuera de BeginMode2D):
//   - Barra de título con foco y pivote activo
//   - Leyenda de colores (upstream / downstream / otro / pivote)
//   - Barra de progreso de simulación
//   - Contador de nodos
//   - Hint de controles según modo activo
// ─────────────────────────────────────────────────────────────────────────────
#include "app.hpp"

void dep_hud_draw(const AppState& state, bool shift_held);
