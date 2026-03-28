#pragma once
#include "panel_widget.hpp"

class DocsPanel : public PanelWidget {
public:
    explicit DocsPanel(AppState& s) : PanelWidget(s) {}
    void draw(Vector2 mouse) override;
};

void draw_docs_panel(AppState& state, Vector2 mouse);
