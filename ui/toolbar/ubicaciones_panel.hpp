#pragma once
#include "ui/widgets/panel_widget.hpp"
#include "ui/constants.hpp"

class UbicacionesPanel : public PanelWidget {
    // 6 filas de ruta (46px c/u) + apply + generadores + acople (3 btns + status) + padding
    static constexpr int PW = 620, PH = 640;

public:
    explicit UbicacionesPanel(AppState& s) : PanelWidget(s) {
        pos_x = 0; pos_y = TOOLBAR_H;
    }
    void draw(Vector2 mouse) override;
};

void draw_ubicaciones_panel(AppState& state, Vector2 mouse);
