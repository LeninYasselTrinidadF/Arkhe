#pragma once
// ── editor_sections.hpp ───────────────────────────────────────────────────────
// Funciones de dibujo de secciones del EntryEditor.
// Cada sección es colapsable y tiene su propio botón «Guardar» que escribe
// el JSON del nodo (<basename>.json) completo.
// ─────────────────────────────────────────────────────────────────────────────

#include "data/editor/edit_state.hpp"
#include "data/editor/section_data.hpp"
#include "app.hpp"
#include "data/math_node.hpp"
#include "raylib.h"
#include <vector>
#include <string>

namespace editor_sections {

// ── Tipo de callback de guardado ──────────────────────────────────────────────
// Firma: on_save(sel, edit, state)
using SaveFn = void (*)(MathNode*, EditState&, AppState&);

// ── draw_node_header ──────────────────────────────────────────────────────────
/// Cabecera fija (no colapsable): «Nodo: <code>  |  <label>» + separador.
void draw_node_header(MathNode* sel, int lx, int lw, int& y);

// ── draw_basic_info_section ───────────────────────────────────────────────────
/// Sección colapsable: Nota, Texture key, MSC refs.
/// Escribe en vivo sobre sel. «Guardar» → on_save → save_node_json.
void draw_basic_info_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw,
    int& y, Vector2 mouse,
    SaveFn on_save);

// ── draw_body_section ─────────────────────────────────────────────────────────
/// Sección colapsable: textarea LaTeX + preview + file manager.
/// «Guardar» escribe el .tex (on_save_tex) y después llama on_save_json.
void draw_body_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll, bool& body_active, bool& show_file_manager,
    SaveFn on_save_json,
    void (*on_save_tex)(MathNode*, EditState&, AppState&, bool&));

// ── draw_body_preview ─────────────────────────────────────────────────────────
/// (interno, llamado por draw_body_section; declarado aquí para editor_body.cpp)
void draw_body_preview(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll);

// ── draw_cross_refs_section ───────────────────────────────────────────────────
/// Sección colapsable: lista de referencias cruzadas + formulario de añadir.
/// hint_codes: códigos de nodos hermanos para autocompletar.
void draw_cross_refs_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    const std::vector<std::string>& hint_codes,
    SaveFn on_save);

// ── draw_resources_section ────────────────────────────────────────────────────
/// Sección colapsable: lista de recursos + formulario de añadir.
void draw_resources_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    SaveFn on_save);

// ── draw_file_manager ─────────────────────────────────────────────────────────
/// Popup flotante de selección de .tex (sin cambios de firma).
void draw_file_manager(
    MathNode* sel, EditState& edit, AppState& state,
    int panel_x, int panel_y, Vector2 mouse,
    bool& show_file_manager,
    std::vector<std::string>& tex_files,
    float& file_list_scroll,
    bool& files_stale,
    std::unordered_map<std::string, std::string>& entries_index);

} // namespace editor_sections
