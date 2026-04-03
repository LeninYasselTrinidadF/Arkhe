// bubble_controls.hpp — facade de conveniencia.
// No implementa nada; solo re-exporta los módulos reales para que los
// callers existentes (bubble_view, dep_view, …) no necesiten cambiarse.
#pragma once

#include "../controls/mode_switcher.hpp"
#include "../controls/zoom_controls.hpp"
#include "../controls/ext_buttons.hpp"
#include "../controls/canvas_buttons.hpp"
#include "../controls/canvas_btn.hpp"
#include "../controls/nav_sync.hpp"
// nav_sync es un detalle interno de canvas_buttons; no se expone aquí.