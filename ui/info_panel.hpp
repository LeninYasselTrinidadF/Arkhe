#pragma once
#include "../app.hpp"
#include "constants.hpp"
#include "raylib.h"

// Panel info (breadcrumb + selección) + panel recursos con scroll
void draw_info_panel(AppState& state, Vector2 mouse);
