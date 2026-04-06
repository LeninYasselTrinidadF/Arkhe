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
using SaveFn = void (*)(MathNode*, EditState&, AppState&);

// ── CrossRefSuggestion ────────────────────────────────────────────────────────
// Sugerencia de referencia cruzada derivada de los nodos hermanos del nodo
// actual. Se construye en entry_editor.cpp y se pasa a draw_cross_refs_section.
struct CrossRefSuggestion {
    EditorCrossRef ref;         ///< Referencia sugerida (target_code + relation)
    std::string    from_label;  ///< Label del nodo hermano que tiene esta ref
    std::string    from_code;   ///< Code del nodo hermano (para debug/display)
};

// ── draw_node_header ──────────────────────────────────────────────────────────
void draw_node_header(MathNode* sel, int lx, int lw, int& y);

// ── draw_basic_info_section ───────────────────────────────────────────────────
void draw_basic_info_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw,
    int& y, Vector2 mouse,
    SaveFn on_save);

// ── draw_body_section ─────────────────────────────────────────────────────────
void draw_body_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll, bool& body_active, bool& show_file_manager,
    SaveFn on_save_json,
    void (*on_save_tex)(MathNode*, EditState&, AppState&, bool&));

// ── draw_body_preview ─────────────────────────────────────────────────────────
void draw_body_preview(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll);

// ── draw_cross_refs_section ───────────────────────────────────────────────────
// hint_codes   : códigos de hijos del nodo actual para autocompletar en el campo.
// suggestions  : referencias cruzadas de nodos hermanos propuestas automáticamente.
void draw_cross_refs_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    const std::vector<std::string>& hint_codes,
    const std::vector<CrossRefSuggestion>& suggestions,
    SaveFn on_save);

// ── draw_resources_section ────────────────────────────────────────────────────
void draw_resources_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    SaveFn on_save);

// ── draw_file_manager ─────────────────────────────────────────────────────────
void draw_file_manager(
    MathNode* sel, EditState& edit, AppState& state,
    int panel_x, int panel_y, Vector2 mouse,
    bool& show_file_manager,
    std::vector<std::string>& tex_files,
    float& file_list_scroll,
    bool& files_stale,
    std::unordered_map<std::string, std::string>& entries_index);

} // namespace editor_sections
