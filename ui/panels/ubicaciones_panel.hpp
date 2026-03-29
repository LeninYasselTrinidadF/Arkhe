#pragma once
#include "../widgets/panel_widget.hpp"
#include "../constants.hpp"

class UbicacionesPanel : public PanelWidget {
    static constexpr int PW = 600, PH = 360;

public:
    explicit UbicacionesPanel(AppState& s) : PanelWidget(s) {
        pos_x = 0; pos_y = TOOLBAR_H;
    }
    void draw(Vector2 mouse) override;
};

void draw_ubicaciones_panel(AppState& state, Vector2 mouse);
