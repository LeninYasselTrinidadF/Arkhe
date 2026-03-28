#pragma once
#include "panel_widget.hpp"
#include "constants.hpp"

class UbicacionesPanel : public PanelWidget {
    static constexpr int PW = 600, PH = 360;   // un poco más ancho para el botón "..."
    static constexpr int PX = 0, PY = TOOLBAR_H;

public:
    explicit UbicacionesPanel(AppState& s) : PanelWidget(s) {}
    void draw(Vector2 mouse) override;
};

void draw_ubicaciones_panel(AppState& state, Vector2 mouse);