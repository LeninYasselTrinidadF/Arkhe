#pragma once
#include "app.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "raylib.h"
#include <vector>
#include <string>

// ── Tipo de ítem seleccionable en el info_panel ───────────────────────────────

enum class InfoItemKind {
    None,
    RenderButton,   // botón "Render LaTeX" / "Re-render LaTeX"
    ResourceLink,   // tarjeta de recurso que abre URL externa
    CrossrefCard,   // tarjeta de crossref (navega a nodo MSC/STD/Mathlib)
    MathlibHitCard, // tarjeta de módulo Mathlib relacionado
};

struct InfoNavItem {
    InfoItemKind kind   = InfoItemKind::None;
    Rectangle    rect   = {};
    std::string  data;  // URL, código de nodo, módulo, etc.
};

// ── Estado de la zona Info ────────────────────────────────────────────────────
// Se reconstruye cada frame en kbnav_info_register_*.
// El índice info_idx en KbNavState recorre esta lista.

struct KbInfoState {
    std::vector<InfoNavItem> items;
    bool expanded = false;  // Espacio → info_panel al máximo

    void clear() { items.clear(); }
    void push(InfoNavItem&& it) { items.push_back(std::move(it)); }
    int  count() const { return (int)items.size(); }
    const InfoNavItem* current(int idx) const {
        if (idx < 0 || idx >= count()) return nullptr;
        return &items[idx];
    }
};

extern KbInfoState g_kbinfo;

// ── API ───────────────────────────────────────────────────────────────────────

// Llamar al INICIO de draw_info_panel (reset de ítems del frame anterior).
void kbnav_info_begin_frame();

// Registrar un ítem del botón Render.
void kbnav_info_register_render(Rectangle r);

// Registrar una tarjeta de recurso (puede ser link, nodo local, etc.).
void kbnav_info_register_resource(Rectangle r, const std::string& url_or_code,
                                  bool is_web_link);

// Registrar tarjeta de crossref / mathlib hit.
void kbnav_info_register_crossref(Rectangle r, const std::string& code,
                                  InfoItemKind kind);

// Procesa teclado para la zona Info.
// • Up/Down ciclan por los ítems registrados.
// • Enter activa el ítem actual (render, open URL, navigate).
// • Espacio expande/contrae el panel (modifica g_split_y).
// Devuelve true si alguna tecla fue consumida.
bool kbnav_info_handle(AppState& state, int split_y_min, int split_y_max,
                       int& g_split_y);

// Dibuja el indicador de foco sobre el ítem seleccionado.
void kbnav_info_draw();
