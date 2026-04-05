#pragma once
// ── docs_panel.hpp ────────────────────────────────────────────────────────────
// Panel de documentación paginada de Arkhe.
//
// Reemplaza al DocsPanel original de una sola página.
// Muestra una barra de navegación de pestañas y delega el contenido
// a las funciones draw de cada DocsPage definida en ui/docs_panel/.
//
// Tamaño del panel: 520 × 560 (ajustable con las constantes PW / PH).
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/widgets/panel_widget.hpp"
#include "ui/constants.hpp"
#include <vector>

class DocsPanel : public PanelWidget {
public:
    static constexpr int PW = 520;
    static constexpr int PH = 560;

    explicit DocsPanel(AppState& s) : PanelWidget(s) {
        pos_x = 460; pos_y = TOOLBAR_H;
    }

    void draw(Vector2 mouse) override;

private:
    int   current_page  = 0;
    float page_scroll   = 0.f;

    // Drag state para scrollbar interna
    bool  sb_drag  = false;
    float sb_dy    = 0.f;
    float sb_doff  = 0.f;
};

void draw_docs_panel(AppState& state, Vector2 mouse);
