#pragma once
// ── editor_sections.hpp ───────────────────────────────────────────────────────
// Funciones de dibujo reutilizables para las distintas secciones del
// EntryEditor. Cada función recibe exactamente lo que necesita: no hay
// dependencia de la clase completa, lo que facilita:
//  · Testear cada sección visualmente de forma aislada.
//  · Reusar secciones en otros paneles (p.ej. un futuro BatchEditor).
//  · Mantener entry_editor.cpp limpio como «orquestador».
//
// Convención de parámetros:
//  · lx / lw    → x inicial y ancho del área de contenido
//  · py / ph    → y inicial y alto del panel completo (para calcular límites)
//  · y          → cursor vertical, modificado por referencia
//  · mouse      → posición del ratón en pantalla
// ─────────────────────────────────────────────────────────────────────────────

#include "edit_state.hpp"
#include "../../app.hpp"
#include "../../data/math_node.hpp"
#include "raylib.h"
#include <vector>
#include <string>

namespace editor_sections {

// ── draw_node_header ──────────────────────────────────────────────────────────
/// Muestra «Nodo: <code>  |  <label>» y una línea separadora.
/// Avanza `y`.
void draw_node_header(MathNode* sel, int lx, int lw, int& y);

// ── draw_node_fields ──────────────────────────────────────────────────────────
/// Campos de texto editables: nota, texture_key y MSC refs.
/// Escribe de vuelta en `sel` mientras el usuario escribe.
/// Usa `state.toolbar.active_field` como ID de campo activo.
void draw_node_fields(MathNode* sel, EditState& edit,
                      AppState& state,
                      int lx, int lw, int& y, Vector2 mouse);

// ── draw_body_section ─────────────────────────────────────────────────────────
/// Textarea de LaTeX con scroll, cursor parpadeante y botones
/// «Importar» (abre el file-manager) y «Guardar» (escribe al disco).
///
/// `show_file_manager` y `body_scroll`/`body_active` son estado
/// del EntryEditor; se pasan por referencia para que la sección los
/// actualice sin necesitar acceso a la clase completa.
void draw_body_section(MathNode* sel, EditState& edit,
                       AppState& state,
                       int lx, int lw,
                       int py, int ph,
                       int& y, Vector2 mouse,
                       float& body_scroll,
                       bool&  body_active,
                       bool&  show_file_manager,
                       /* callbacks para IO */
                       void (*on_save)(MathNode*, EditState&, AppState&, bool&));

// ── draw_file_manager ─────────────────────────────────────────────────────────
/// Popup flotante de selección de archivos .tex.
/// `tex_files` y `file_list_scroll` son estado del EntryEditor.
/// `files_stale` se pone a true cuando se selecciona un archivo.
void draw_file_manager(MathNode* sel, EditState& edit,
                       AppState& state,
                       int panel_x, int panel_y,
                       Vector2 mouse,
                       bool& show_file_manager,
                       std::vector<std::string>& tex_files,
                       float& file_list_scroll,
                       bool& files_stale,
                       std::unordered_map<std::string,std::string>& entries_index);

// ── draw_resource_list ────────────────────────────────────────────────────────
/// Lista los recursos del nodo con botón «x» para eliminar cada uno.
void draw_resource_list(MathNode* sel,
                        int lx, int lw,
                        int py, int ph,
                        int& y, Vector2 mouse);

// ── draw_add_resource_form ────────────────────────────────────────────────────
/// Formulario de una línea para añadir un nuevo Resource al nodo.
void draw_add_resource_form(MathNode* sel, EditState& edit,
                            AppState& state,
                            int lx, int lw,
                            int py, int ph,
                            int& y, Vector2 mouse);

} // namespace editor_sections
