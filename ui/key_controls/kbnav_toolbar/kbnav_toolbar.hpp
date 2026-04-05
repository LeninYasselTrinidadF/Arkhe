#pragma once
// ── kbnav_toolbar.hpp ─────────────────────────────────────────────────────────
// Navegación por teclado para los paneles modales del toolbar
// (UbicacionesPanel, DocsPanel, EntryEditor, ConfigPanel).
//
// Patrón idéntico a kbnav_search / kbnav_info:
//   1. begin_frame()         → al INICIO del draw del panel activo
//   2. register(...)         → mientras se dibuja cada widget interactivo
//   3. is_focused / is_activated → consultas durante el draw del mismo frame
//   4. handle(state)         → al FINAL del draw, tras registrar todos los ítems
//   5. draw()                → dibuja el borde de foco (encima de todo)
//
// Teclas:
//   Tab          → siguiente ítem
//   Shift+Tab    → ítem anterior
//   Enter        → activa Button / BrowseButton / FileItem
//                  (no activa TextField ni Textarea — éstos usan su propio Enter)
//   Escape       → limpia el foco (sin cerrar el panel)
//
// Integración con el sistema de active_field:
//   · Cuando kbnav enfoca un TextField, apply_bridge() pone
//     state.toolbar.active_field = field_id en el mismo frame.
//   · Cuando kbnav enfoca un Button / BrowseButton, apply_bridge() limpia
//     active_field para que ningún campo quede "pegado".
//   · La Textarea (body LaTeX) se maneja con body_active (bool&) en
//     EntryEditor: kbnav_toolbar_is_focused(textarea_idx) activa/desactiva
//     body_active directamente desde draw_body_section.
// ─────────────────────────────────────────────────────────────────────────────

#include "app.hpp"
#include "raylib.h"

// ── Tipos de widget ───────────────────────────────────────────────────────────

enum class ToolbarNavKind : uint8_t {
    TextField,     ///< campo de texto de una línea (usa active_field)
    Textarea,      ///< área de texto multilínea (body LaTeX)
    Button,        ///< botón genérico — Enter lo activa
    BrowseButton,  ///< botón "..." de selección de ruta/archivo
    FileItem,      ///< fila del file manager flotante
};

// ── API de registro ───────────────────────────────────────────────────────────

/// Llamar al INICIO del draw del panel activo (antes de dibujar ningún widget).
void kbnav_toolbar_begin_frame();

/// Registrar un widget interactivo tal como se dibuja.
/// · kind     : tipo de widget
/// · rect     : rectángulo exacto del widget en pantalla
/// · label    : texto corto para el indicador visual (puede ser "")
/// · field_id : solo para TextField — el ID de active_field que debe activarse
/// Devuelve el índice asignado al widget en este frame.
int kbnav_toolbar_register(ToolbarNavKind kind, Rectangle rect,
                            const char* label  = "",
                            int         field_id = -1);

// ── API de consulta ───────────────────────────────────────────────────────────

/// True si el widget en `idx` tiene el foco de teclado este frame.
bool kbnav_toolbar_is_focused(int idx);

/// True si el widget en `idx` fue activado (Enter) el frame ANTERIOR.
/// (mismo frame-delay que kbnav_search_is_activated)
bool kbnav_toolbar_is_activated(int idx);

/// Tipo del ítem enfocado actualmente.
ToolbarNavKind kbnav_toolbar_focused_kind();

/// Índice del ítem enfocado (-1 si ninguno).
int kbnav_toolbar_focused_idx();

// ── Procesamiento y dibujo ────────────────────────────────────────────────────

/// Procesar Tab / Shift+Tab / Enter / Escape.
///   textarea_active : true cuando el body editor está capturando Enter
///                     (evita que Enter active botones mientras se escribe).
/// Llamar AL FINAL del draw del panel, tras haber registrado todos los ítems.
/// Devuelve true si alguna tecla fue consumida.
bool kbnav_toolbar_handle(AppState& state, bool textarea_active = false);

/// Dibujar borde de foco sobre el ítem activo.
/// Llamar al final del draw del panel, después de handle().
void kbnav_toolbar_draw();
