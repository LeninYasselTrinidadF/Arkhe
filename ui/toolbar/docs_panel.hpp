#pragma once
#include "ui/widgets/panel_widget.hpp"
#include "ui/constants.hpp"

class DocsPanel : public PanelWidget {
    static constexpr int PW = 480, PH = 380;
public:
    explicit DocsPanel(AppState& s) : PanelWidget(s) {
        // Centrado horizontalmente al abrir (se actualiza en draw si SW es 0)
        pos_x = 460; pos_y = TOOLBAR_H;
    }
    void draw(Vector2 mouse) override;
};

void draw_docs_panel(AppState& state, Vector2 mouse);
