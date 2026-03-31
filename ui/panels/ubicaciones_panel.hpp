#pragma once
#include "../widgets/panel_widget.hpp"
#include "../constants.hpp"

class UbicacionesPanel : public PanelWidget {
    // PH ampliado para incluir la sección de generadores Mathlib
    static constexpr int PW = 620, PH = 520;

public:
    explicit UbicacionesPanel(AppState& s) : PanelWidget(s) {
        pos_x = 0; pos_y = TOOLBAR_H;
    }
    void draw(Vector2 mouse) override;
};

void draw_ubicaciones_panel(AppState& state, Vector2 mouse);
