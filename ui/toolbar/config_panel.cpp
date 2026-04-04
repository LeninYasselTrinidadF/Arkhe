#include "ui/toolbar/config_panel.hpp"
#include "ui/constants.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;

static const char* CONFIG_PATH = "assets/config.json";

// ── Constructor ───────────────────────────────────────────────────────────────

ConfigPanel::ConfigPanel(AppState& s) : PanelWidget(s) {
    // Posición inicial: esquina superior izquierda bajo el toolbar,
    // pero el usuario la puede mover libremente.
    pos_x = 0;
    pos_y = TOOLBAR_H;
}

// ── IO ────────────────────────────────────────────────────────────────────────

void ConfigPanel::load_config() {
    std::ifstream f(CONFIG_PATH);
    if (!f.is_open()) return;
    try {
        json j = json::parse(f);
        if (j.contains("font_path"))
            strncpy(font_path_buf, j["font_path"].get<std::string>().c_str(), 511);
        if (j.contains("font_base_size")) {
            float sz = j["font_base_size"].get<float>();
            g_fonts.base_size = std::clamp(sz, 24.0f, 64.0f);
        }
    }
    catch (...) {}
}

void ConfigPanel::save_config() {
    json j;
    j["font_path"] = font_path_buf;
    j["font_base_size"] = g_fonts.base_size;
    try {
        std::ofstream f(CONFIG_PATH);
        if (f.is_open()) f << j.dump(2);
    }
    catch (...) {}
}

// ── Sección Fuente ────────────────────────────────────────────────────────────

void ConfigPanel::draw_font_section(int lx, int fw, int& y, Vector2 mouse) {
    const Theme& th = g_theme;

    // ── Cabecera de sección ───────────────────────────────────────────────────
    DrawRectangle(lx - 4, y - 2, fw + 8, 18, th_alpha(th.bg_button));
    DrawTextF("FUENTE", lx, y + 2, 10, th_alpha(th.accent));
    y += 22;

    // ── Ruta .ttf ─────────────────────────────────────────────────────────────
    DrawTextF("Archivo .ttf", lx, y, 10, th_alpha(th.text_dim));
    y += 13;
    draw_text_field(font_path_buf, 512,
        lx, y, fw - 78, 22, 11,
        active_field, 0, mouse);

    Rectangle load_r = { (float)(lx + fw - 74), (float)y, 70.0f, 22.0f };
    bool load_hov = CheckCollisionPointRec(mouse, load_r);
    DrawRectangleRec(load_r, load_hov
        ? Color{ 80,50,180,255 } : Color{ 50,30,120,255 });
    DrawRectangleLinesEx(load_r, 1.0f, { 120, 80, 255, 200 });
    DrawTextF("Cargar", (int)load_r.x + 11, (int)load_r.y + 6, 10, WHITE);
    if (load_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_fonts.unload();
        g_fonts.load(font_path_buf);
        save_config();
    }
    y += 28;

    // ── Slider tamaño base ────────────────────────────────────────────────────
    DrawTextF("Tamaño base", lx, y, 10, th_alpha(th.text_dim));
    y += 13;

    constexpr float F_MIN = 24.0f, F_MAX = 64.0f;
    float& sz = g_fonts.base_size;

    int track_w = fw - 56;
    DrawRectangle(lx, y + 7, track_w, 4, th_alpha(th.bg_button));

    float t = (sz - F_MIN) / (F_MAX - F_MIN);
    int   fill_w = (int)(t * track_w);
    DrawRectangle(lx, y + 7, fill_w, 4, th.accent);

    // Knob
    DrawCircle(lx + fill_w, y + 9, 7.0f, th.accent);
    DrawCircleLines(lx + fill_w, y + 9, 7.0f, th.accent_hover);

    Rectangle drag_zone = { (float)(lx - 8), (float)(y + 1),
                             (float)(track_w + 16), 18.0f };

    // Iniciar drag solo si el click cae dentro del drag_zone
    if (CheckCollisionPointRec(mouse, drag_zone) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        slider_dragging = true;

    if (slider_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float new_t = std::clamp((mouse.x - lx) / (float)track_w, 0.0f, 1.0f);
        sz = (float)(int)(F_MIN + new_t * (F_MAX - F_MIN) + 0.5f);
    }

    // Guardar al soltar, sin importar dónde esté el cursor
    if (slider_dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        slider_dragging = false;
        save_config();
    }

    char sz_str[16];
    snprintf(sz_str, sizeof(sz_str), "%.0fpx", sz);
    DrawTextF(sz_str, lx + track_w + 8, y + 3, 12, th.text_primary);
    y += 26;

    // ── Preview ───────────────────────────────────────────────────────────────
    DrawLine(lx, y, lx + fw, y, th_alpha(th.border));
    y += 7;
    DrawTextF("AaBbCc 0123 — areas matematicas · Mathlib4", lx, y, 11, th.text_primary);
    y += 18;
    DrawTextF("Referencias cruzadas · MSC2020 · Estandar", lx, y, 9,
        th_alpha(th.text_secondary));
    y += 16;
}

// ── draw ─────────────────────────────────────────────────────────────────────

void ConfigPanel::draw(Vector2 mouse) {
    if (!state.toolbar.config_open) return;

    // Carga lazy de config.json la primera vez
    if (!config_loaded) {
        load_config();
        config_loaded = true;
    }

    // draw_window_frame usa pos_x/pos_y internamente
    if (draw_window_frame(PW, PH,
        "CONFIG", { 160, 130, 255, 255 },
        { 70, 45, 160, 220 }, mouse))
    {
        save_config();
        state.toolbar.config_open = false;
        state.toolbar.active_tab = ToolbarTab::None;
        return;
    }

    // Coordenadas relativas a la posición actual del panel
    const int lx = pos_x + 14;
    const int fw = PW - 28;
    int y = pos_y + 38;

    draw_font_section(lx, fw, y, mouse);
}

// ── Función libre (para compatibilidad) ──────────────────────────────────────

void draw_config_panel(AppState& state, Vector2 mouse) {
    static ConfigPanel panel(state);
    panel.draw(mouse);
}