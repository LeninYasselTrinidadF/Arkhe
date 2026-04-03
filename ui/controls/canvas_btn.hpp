#pragma once
#include "../../app.hpp"
#include "../core/theme.hpp"
#include "raylib.h"

// ── canvas_btn_impl ───────────────────────────────────────────────────────────
// Primitivo de botón canvas: dibuja un botón con estado enabled/active/hover
// y devuelve true si fue pulsado. Usado por draw_canvas_buttons y
// draw_dep_canvas_buttons.

static constexpr int BTN_FONT = 14;
static constexpr int BTN_PAD_X = 10;
static constexpr int BTN_PAD_Y = 7;

void  btn_dims(const char* label, float& bw, float& bh);
float btn_gap();

bool canvas_btn_impl(const Theme& th, Vector2 mouse, bool canvas_blocked,
    float bx, float by, const char* label,
    bool enabled, bool active = false,
    float* out_bw = nullptr, float* out_bh = nullptr);