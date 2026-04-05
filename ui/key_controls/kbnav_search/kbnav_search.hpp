#pragma once
#include "app.hpp"
#include "data/math_node.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "raylib.h"

// ── SearchNavKind ─────────────────────────────────────────────────────────────
// Tipos de ítem interactivo dentro del panel de búsqueda.
// Se registran en orden visual (top → bottom) durante cada draw().
// ─────────────────────────────────────────────────────────────────────────────

enum class SearchNavKind : uint8_t {
    LocalField,
    LocalButton,
    LocalResult,
    LocalPagerPrev,
    LocalPagerNext,
    LoogleField,
    LoogleButton,
    LoogleResult,
    LooglePagerPrev,
    LooglePagerNext,
};

// ── API de registro ───────────────────────────────────────────────────────────
// Llamar al inicio de SearchPanel::draw() antes de los sub-componentes.
void kbnav_search_begin_frame();

// Registrar un ítem interactivo tal como se dibuja. Devuelve su índice.
int  kbnav_search_register(SearchNavKind kind, Rectangle rect);

// ── API de consulta (durante draw de sub-componentes) ─────────────────────────

// True si el ítem en idx tiene el foco teclado este frame.
bool kbnav_search_is_focused(int idx);

// True si el ítem en idx fue activado (Enter) el frame anterior.
bool kbnav_search_is_activated(int idx);

// True si el ítem actualmente enfocado es del tipo dado.
// Usar para decidir si un campo de texto acepta input.
bool kbnav_search_focused_is(SearchNavKind kind);

// ── Procesamiento y dibujo ────────────────────────────────────────────────────
// Llamar al FINAL de SearchPanel::draw() tras registrar todos los ítems.
bool kbnav_search_handle(AppState& state, const MathNode* search_root);

// Dibujar indicador encima de todo (llamar tras handle).
void kbnav_search_draw();
