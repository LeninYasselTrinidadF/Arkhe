#pragma once
// ── dep_camera.hpp ────────────────────────────────────────────────────────────
// Pan y zoom de la cámara dep_view con ratón y rueda.
// ─────────────────────────────────────────────────────────────────────────────
#include "raylib.h"

// Actualiza dep_cam con pan (LMB/MMB) y zoom (rueda).
// over_node: si el cursor está encima de algún nodo (inhibe el pan con LMB).
// dragging:  si hay un nodo siendo arrastrado (inhibe el pan).
void dep_camera_update(Camera2D& dep_cam, bool in_canvas, bool canvas_blocked,
                       bool over_node, bool dragging);
