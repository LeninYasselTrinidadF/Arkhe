#include "theme.hpp"

Theme g_theme = {};   // se inicializa en main con apply_theme(0)

// ── Dark ──────────────────────────────────────────────────────────────────────
Theme theme_dark() {
    Theme t;
    t.name = "Dark";
    t.bg_app = { 11,  11,  18, 255 };
    t.bg_toolbar = { 15,  15,  25, 255 };
    t.bg_panel = { 14,  14,  24, 252 };
    t.bg_panel_header = { 18,  22,  42, 255 };
    t.bg_field = { 20,  20,  35, 255 };
    t.bg_card = { 17,  17,  30, 255 };
    t.bg_card_hover = { 22,  22,  40, 255 };
    t.bg_button = { 28,  28,  48, 220 };
    t.bg_button_hover = { 55,  55,  95, 255 };

    t.text_primary = { 220, 225, 240, 255 };
    t.text_secondary = { 170, 180, 165, 220 };
    t.text_dim = { 100, 100, 150, 200 };
    t.text_code = { 140, 200, 255, 255 };
    t.text_logo = { 100, 140, 255, 255 };

    t.accent = { 70, 130, 255, 255 };
    t.accent_hover = { 100, 160, 255, 255 };
    t.accent_dim = { 80,  80, 120, 200 };

    t.success = { 80, 200, 120, 255 };
    t.success_dim = { 35,  75,  40, 200 };
    t.warning = { 200, 180, 100, 255 };

    t.border = { 45,  45,  72, 200 };
    t.border_hover = { 70,  70, 120, 255 };
    t.border_active = { 70, 130, 255, 255 };
    t.border_panel = { 80, 100, 180, 255 };

    t.bubble_center_flat = { 228, 228, 240, 255 };
    t.bubble_ring = { 170, 170, 200, 255 };
    t.bubble_hover_ring = { 255, 255, 255, 255 };
    t.bubble_label_center = { 25,  25,  50, 255 };
    t.bubble_label_child = { 0,   0,   0, 255 };
    t.bubble_empty_msg = { 90,  90, 110, 255 };
    t.bubble_line_alpha = 0.30f;
    t.bubble_center_alpha = 0.20f;
    t.bubble_glow_alpha = 0.18f;

    t.ctrl_bg = { 28,  28,  48, 220 };
    t.ctrl_bg_hover = { 60,  60, 100, 255 };
    t.ctrl_border = { 80,  80, 130, 255 };
    t.ctrl_text = { 255, 255, 255, 255 };
    t.ctrl_text_dim = { 180, 180, 210, 200 };

    t.split_normal = { 45,  45,  70, 255 };
    t.split_active = { 100, 120, 200, 255 };
    t.split_dots = { 180, 180, 220, 160 };
    t.split_vline = { 50,  50,  70, 255 };

    t.shadow = { 0,   0,   0, 120 };
    t.close_bg = { 50,  40,  60, 255 };
    t.close_bg_hover = { 180,  50,  50, 255 };

    t.scrollbar_bg = { 18,  18,  32, 200 };
    t.scrollbar_thumb = { 70,  70, 120, 255 };

    t.use_transparency = true;
    t.use_fade_lines = true;
    return t;
}

// ── Light ─────────────────────────────────────────────────────────────────────
Theme theme_light() {
    Theme t;
    t.name = "Light";
    t.bg_app = { 238, 241, 250, 255 };
    t.bg_toolbar = { 250, 251, 255, 255 };
    t.bg_panel = { 255, 255, 255, 252 };
    t.bg_panel_header = { 225, 230, 248, 255 };
    t.bg_field = { 245, 247, 253, 255 };
    t.bg_card = { 228, 232, 248, 255 };
    t.bg_card_hover = { 212, 220, 242, 255 };
    t.bg_button = { 208, 216, 238, 255 };
    t.bg_button_hover = { 185, 198, 230, 255 };

    t.text_primary = { 22,  26,  48, 255 };
    t.text_secondary = { 65,  78, 115, 220 };
    t.text_dim = { 120, 132, 168, 200 };
    t.text_code = { 30,  80, 185, 255 };
    t.text_logo = { 45,  88, 218, 255 };

    t.accent = { 55, 108, 218, 255 };
    t.accent_hover = { 35,  88, 198, 255 };
    t.accent_dim = { 100, 130, 205, 180 };

    t.success = { 28, 138,  68, 255 };
    t.success_dim = { 55, 160,  95, 200 };
    t.warning = { 158, 118,  18, 255 };

    t.border = { 178, 185, 212, 200 };
    t.border_hover = { 100, 122, 202, 200 };
    t.border_active = { 55, 108, 218, 255 };
    t.border_panel = { 155, 170, 212, 255 };

    t.bubble_center_flat = { 195, 205, 232, 255 };
    t.bubble_ring = { 135, 148, 192, 255 };
    t.bubble_hover_ring = { 55, 108, 218, 255 };
    t.bubble_label_center = { 18,  26,  62, 255 };
    t.bubble_label_child = { 12,  20,  52, 255 };
    t.bubble_empty_msg = { 98, 110, 152, 255 };
    t.bubble_line_alpha = 0.45f;
    t.bubble_center_alpha = 0.28f;
    t.bubble_glow_alpha = 0.22f;

    t.ctrl_bg = { 215, 222, 242, 255 };
    t.ctrl_bg_hover = { 188, 200, 228, 255 };
    t.ctrl_border = { 155, 168, 212, 255 };
    t.ctrl_text = { 22,  26,  48, 255 };
    t.ctrl_text_dim = { 100, 112, 158, 200 };

    t.split_normal = { 175, 182, 212, 255 };
    t.split_active = { 55, 108, 218, 255 };
    t.split_dots = { 95, 115, 172, 160 };
    t.split_vline = { 158, 168, 202, 255 };

    t.shadow = { 0,   0,   0,  55 };
    t.close_bg = { 200, 205, 228, 255 };
    t.close_bg_hover = { 218,  55,  55, 255 };

    t.scrollbar_bg = { 208, 215, 235, 200 };
    t.scrollbar_thumb = { 138, 150, 198, 255 };

    t.use_transparency = true;
    t.use_fade_lines = true;
    return t;
}

// ── Bocchi ────────────────────────────────────────────────────────────────────
// Inspirado en Bocchi the Rock: negro morado, rosa coral, rojo frambuesa.
Theme theme_bocchi() {
    Theme t;
    t.name = "Bocchi";
    t.bg_app = { 10,   7,  18, 255 };
    t.bg_toolbar = { 14,   9,  25, 255 };
    t.bg_panel = { 20,  12,  35, 252 };
    t.bg_panel_header = { 30,  16,  50, 255 };
    t.bg_field = { 25,  14,  42, 255 };
    t.bg_card = { 22,  12,  38, 255 };
    t.bg_card_hover = { 34,  18,  55, 255 };
    t.bg_button = { 35,  18,  58, 220 };
    t.bg_button_hover = { 62,  28,  88, 255 };

    t.text_primary = { 255, 198, 218, 255 };
    t.text_secondary = { 198, 155, 182, 220 };
    t.text_dim = { 138,  95, 128, 200 };
    t.text_code = { 255, 135, 168, 255 };
    t.text_logo = { 255,  95, 138, 255 };

    t.accent = { 228,  55,  98, 255 };
    t.accent_hover = { 255,  88, 130, 255 };
    t.accent_dim = { 148,  38,  68, 200 };

    t.success = { 158,  95, 205, 255 };  // violeta (en vez de verde)
    t.success_dim = { 98,  55, 138, 200 };
    t.warning = { 255, 175,  75, 255 };

    t.border = { 78,  38,  78, 200 };
    t.border_hover = { 158,  58,  98, 200 };
    t.border_active = { 228,  55,  98, 255 };
    t.border_panel = { 118,  48,  98, 255 };

    t.bubble_center_flat = { 55,  25,  78, 255 };
    t.bubble_ring = { 178,  75, 128, 255 };
    t.bubble_hover_ring = { 255, 148, 188, 255 };
    t.bubble_label_center = { 255, 198, 218, 255 };
    t.bubble_label_child = { 255, 228, 238, 255 };
    t.bubble_empty_msg = { 158,  98, 128, 255 };
    t.bubble_line_alpha = 0.38f;
    t.bubble_center_alpha = 0.28f;
    t.bubble_glow_alpha = 0.22f;

    t.ctrl_bg = { 35,  18,  58, 220 };
    t.ctrl_bg_hover = { 65,  28,  92, 255 };
    t.ctrl_border = { 118,  48,  98, 255 };
    t.ctrl_text = { 255, 198, 218, 255 };
    t.ctrl_text_dim = { 178, 118, 158, 200 };

    t.split_normal = { 98,  38,  78, 255 };
    t.split_active = { 228,  55,  98, 255 };
    t.split_dots = { 198,  98, 138, 160 };
    t.split_vline = { 78,  32,  68, 255 };

    t.shadow = { 0,   0,   0, 145 };
    t.close_bg = { 58,  28,  78, 255 };
    t.close_bg_hover = { 200,  38,  68, 255 };

    t.scrollbar_bg = { 28,  14,  45, 200 };
    t.scrollbar_thumb = { 148,  48,  98, 255 };

    t.use_transparency = true;
    t.use_fade_lines = true;
    return t;
}

// ── Chainsaw Man ──────────────────────────────────────────────────────────────
// Negro casi puro, rojo sangre, naranja quemado.
Theme theme_chainsawman() {
    Theme t;
    t.name = "ChainsawMan";
    t.bg_app = { 6,   3,   3, 255 };
    t.bg_toolbar = { 10,   4,   4, 255 };
    t.bg_panel = { 15,   6,   6, 252 };
    t.bg_panel_header = { 22,   8,   8, 255 };
    t.bg_field = { 18,   7,   7, 255 };
    t.bg_card = { 15,   6,   6, 255 };
    t.bg_card_hover = { 26,  10,  10, 255 };
    t.bg_button = { 30,  10,  10, 220 };
    t.bg_button_hover = { 62,  16,  14, 255 };

    t.text_primary = { 242, 215, 198, 255 };
    t.text_secondary = { 188, 158, 142, 220 };
    t.text_dim = { 128,  88,  78, 200 };
    t.text_code = { 255,  98,  55, 255 };
    t.text_logo = { 222,  38,  28, 255 };

    t.accent = { 198,  22,  18, 255 };
    t.accent_hover = { 255,  58,  28, 255 };
    t.accent_dim = { 128,  18,  14, 200 };

    t.success = { 202,  98,  18, 255 };  // naranja quemado como "positivo"
    t.success_dim = { 138,  58,  10, 200 };
    t.warning = { 255, 178,  18, 255 };

    t.border = { 78,  18,  14, 200 };
    t.border_hover = { 138,  28,  18, 200 };
    t.border_active = { 198,  22,  18, 255 };
    t.border_panel = { 118,  22,  18, 255 };

    t.bubble_center_flat = { 38,   9,   8, 255 };
    t.bubble_ring = { 158,  28,  18, 255 };
    t.bubble_hover_ring = { 255,  78,  38, 255 };
    t.bubble_label_center = { 242, 215, 198, 255 };
    t.bubble_label_child = { 255, 228, 212, 255 };
    t.bubble_empty_msg = { 138,  78,  68, 255 };
    t.bubble_line_alpha = 0.42f;
    t.bubble_center_alpha = 0.28f;
    t.bubble_glow_alpha = 0.24f;

    t.ctrl_bg = { 30,  10,  10, 220 };
    t.ctrl_bg_hover = { 65,  18,  14, 255 };
    t.ctrl_border = { 118,  22,  18, 255 };
    t.ctrl_text = { 242, 215, 198, 255 };
    t.ctrl_text_dim = { 168, 118,  98, 200 };

    t.split_normal = { 88,  18,  14, 255 };
    t.split_active = { 198,  22,  18, 255 };
    t.split_dots = { 178,  78,  55, 160 };
    t.split_vline = { 68,  14,  12, 255 };

    t.shadow = { 0,   0,   0, 162 };
    t.close_bg = { 48,  14,  14, 255 };
    t.close_bg_hover = { 198,  18,  18, 255 };

    t.scrollbar_bg = { 18,   7,   7, 200 };
    t.scrollbar_thumb = { 138,  22,  18, 255 };

    t.use_transparency = true;
    t.use_fade_lines = true;
    return t;
}

// ── Generic ───────────────────────────────────────────────────────────────────
// Colores planos sin transparencias. Máximo rendimiento en hardware limitado.
Theme theme_generic() {
    Theme t;
    t.name = "Generic";
    t.bg_app = { 32,  32,  32, 255 };
    t.bg_toolbar = { 26,  26,  26, 255 };
    t.bg_panel = { 42,  42,  42, 255 };
    t.bg_panel_header = { 50,  50,  50, 255 };
    t.bg_field = { 46,  46,  46, 255 };
    t.bg_card = { 52,  52,  52, 255 };
    t.bg_card_hover = { 60,  60,  60, 255 };
    t.bg_button = { 46,  46,  46, 255 };
    t.bg_button_hover = { 65,  65,  65, 255 };

    t.text_primary = { 222, 222, 222, 255 };
    t.text_secondary = { 182, 182, 182, 255 };
    t.text_dim = { 132, 132, 132, 255 };
    t.text_code = { 148, 198, 255, 255 };
    t.text_logo = { 128, 168, 255, 255 };

    t.accent = { 80, 140, 255, 255 };
    t.accent_hover = { 110, 165, 255, 255 };
    t.accent_dim = { 68, 100, 182, 255 };

    t.success = { 78, 178,  98, 255 };
    t.success_dim = { 58, 138,  78, 255 };
    t.warning = { 198, 168,  78, 255 };

    t.border = { 68,  68,  68, 255 };
    t.border_hover = { 90,  90,  90, 255 };
    t.border_active = { 80, 140, 255, 255 };
    t.border_panel = { 78,  78,  78, 255 };

    t.bubble_center_flat = { 78,  78,  95, 255 };
    t.bubble_ring = { 128, 128, 150, 255 };
    t.bubble_hover_ring = { 218, 218, 218, 255 };
    t.bubble_label_center = { 222, 222, 222, 255 };
    t.bubble_label_child = { 198, 198, 198, 255 };
    t.bubble_empty_msg = { 138, 138, 138, 255 };
    t.bubble_line_alpha = 0.55f;
    t.bubble_center_alpha = 0.35f;
    t.bubble_glow_alpha = 0.28f;

    t.ctrl_bg = { 46,  46,  46, 255 };
    t.ctrl_bg_hover = { 65,  65,  65, 255 };
    t.ctrl_border = { 78,  78,  78, 255 };
    t.ctrl_text = { 222, 222, 222, 255 };
    t.ctrl_text_dim = { 158, 158, 158, 255 };

    t.split_normal = { 68,  68,  78, 255 };
    t.split_active = { 80, 140, 255, 255 };
    t.split_dots = { 148, 148, 168, 255 };
    t.split_vline = { 62,  62,  72, 255 };

    t.shadow = { 0,   0,   0, 255 };  // sombra sólida
    t.close_bg = { 62,  55,  55, 255 };
    t.close_bg_hover = { 178,  48,  48, 255 };

    t.scrollbar_bg = { 40,  40,  40, 255 };
    t.scrollbar_thumb = { 88,  88, 108, 255 };

    t.use_transparency = false;  // ← sin blending de alpha
    t.use_fade_lines = false;  // ← líneas sólidas
    return t;
}

// ── apply_theme / helpers ─────────────────────────────────────────────────────

void apply_theme(int id) {
    switch (id) {
    case 0: g_theme = theme_dark();         break;
    case 1: g_theme = theme_light();        break;
    case 2: g_theme = theme_bocchi();       break;
    case 3: g_theme = theme_chainsawman();  break;
    case 4: g_theme = theme_generic();      break;
    default: g_theme = theme_dark();        break;
    }
}

const char* theme_name(int id) {
    switch (id) {
    case 0: return "Dark";
    case 1: return "Light";
    case 2: return "Bocchi";
    case 3: return "CSM";
    case 4: return "Generic";
    default: return "?";
    }
}