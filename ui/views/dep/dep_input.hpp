#pragma once
// ── dep_input.hpp ─────────────────────────────────────────────────────────────
// Gestión de teclado en dep_view:
//   - Pan de cámara con Shift+flechas
//   - Modo pivote con Ctrl (activación/desactivación y navegación por anillos)
//   - Navegación geográfica normal (sin modificadores)
//
// Devuelve true si el frame debe reiniciarse (se llamó dep_view_init).
// En ese caso el caller debe hacer return inmediatamente.
// ─────────────────────────────────────────────────────────────────────────────
#include "app.hpp"
#include "raylib.h"

bool dep_input_handle(AppState& state, Camera2D& dep_cam);
