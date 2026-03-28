#pragma once
#include "panel_widget.hpp"
#include "constants.hpp" 

// Panel flotante "Apariencia": slider de tamaño de fuente + ruta de .ttf.
// Se abre desde el toolbar igual que Ubicaciones/Docs/Editor.
class FontPanel : public PanelWidget {
    static constexpr int PW = 420, PH = 200;
    static constexpr int PX = 0,  PY = TOOLBAR_H;

    // Campo de ruta de fuente
    char font_path_buf[512] = "assets/fonts/font.ttf";
    int  active_field = -1;

public:
    explicit FontPanel(AppState& s) : PanelWidget(s) {}
    void draw(Vector2 mouse) override;
};

void draw_font_panel(AppState& state, Vector2 mouse);
