#pragma once
#include "app.hpp"
#include "raylib.h"

// ── kbnav_views ───────────────────────────────────────────────────────────────
// Gestiona todos los hotkeys que actúan sobre las vistas (burbuja y dependencias)
// cuando la zona activa es Canvas.
//
// Hotkeys:
//   +/−          zoom
//   P            modo posición
//   M / N        ciclar modo de vista (MSC2020 / Mathlib / Estandar)
//   S            alternar burbuja ↔ dependencias
//   E            abrir en VS Code (nodo seleccionado)
//   F            abrir en Mathlib (VS Code)
//   C            casa — resetear cámara y volver al nodo raíz
//
// Devuelve true si alguna tecla fue consumida.
// Solo actúa cuando la zona activa es Canvas (o el sistema kb no está activo).
// ─────────────────────────────────────────────────────────────────────────────

bool kbnav_views_handle(AppState& state, Camera2D& cam, Camera2D& dep_cam);
