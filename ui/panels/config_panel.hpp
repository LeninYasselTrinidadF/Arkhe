#pragma once
#include "../widgets/panel_widget.hpp"
#include "../constants.hpp"
#include <string>

// ── ConfigPanel ───────────────────────────────────────────────────────────────
// Panel flotante "Config" abierto desde la tab homónima del toolbar.
// Agrupa sub-secciones colapsables; de momento: Fuente / Apariencia.
// Lee y escribe assets/config.json al abrir/cerrar.
// ─────────────────────────────────────────────────────────────────────────────
class ConfigPanel : public PanelWidget {
    static constexpr int PW = 460;
    static constexpr int PH = 280;

    // ── Sección Fuente ────────────────────────────────────────────────────────
    char font_path_buf[512] = "assets/fonts/font.ttf";
    int  active_field = -1;
    bool slider_dragging = false;   // ← track drag fuera del hit zone

    // ── Estado IO ─────────────────────────────────────────────────────────────
    bool config_loaded = false;   // true después de la primera carga

    void load_config();
    void save_config();

    // ── Secciones de dibujo ───────────────────────────────────────────────────
    void draw_font_section(int lx, int fw, int& y, Vector2 mouse);

public:
    explicit ConfigPanel(AppState& s);
    void draw(Vector2 mouse) override;
};

void draw_config_panel(AppState& state, Vector2 mouse);