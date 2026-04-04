// bubble_controls.hpp — facade de conveniencia.
// No implementa nada; solo re-exporta los módulos reales para que los
// callers existentes (bubble_view, dep_view, …) no necesiten cambiarse.
#pragma once

#include "ui/views/controls/mode_switcher.hpp"
#include "ui/views/controls/zoom_controls.hpp"
#include "ui/views/controls/ext_buttons.hpp"
#include "ui/views/controls/canvas_buttons.hpp"
#include "ui/views/controls/canvas_btn.hpp"
#include "ui/views/controls/nav_sync.hpp"
// nav_sync es un detalle interno de canvas_buttons; no se expone aquí.