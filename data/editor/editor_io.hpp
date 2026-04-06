#pragma once
// ── editor_io.hpp ─────────────────────────────────────────────────────────────
// Persistencia del EntryEditor:
//   · entries_index.json       (code → basename)
//   · <basename>.tex           (cuerpo LaTeX)
//   · <basename>.json          (basic_info + cross_refs + resources)
//
// Acople: itera el grafo completo y aplica secciones JSON a los MathNodes.
// ─────────────────────────────────────────────────────────────────────────────

#include "app.hpp"
#include "data/math_node.hpp"
#include "data/editor/edit_state.hpp"
#include "data/editor/section_data.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace editor_io {

    // ── Rutas ─────────────────────────────────────────────────────────────────────

    inline std::string safe_basename(const std::string& code) {
        std::string s = code;
        for (char& c : s)
            if (c == '/' || c == '\\' || c == ':' || c == ' ' || c == '.') c = '_';
        return s;
    }

    inline std::string safe_filename(const std::string& code) {
        return safe_basename(code) + ".tex";
    }

    inline std::string index_path(const AppState& state) {
        return std::string(state.toolbar.entries_path) + "entries_index.json";
    }

    inline std::string full_tex_path(const AppState& state, const std::string& fname) {
        return std::string(state.toolbar.entries_path) + fname;
    }

    inline std::string node_json_path(const AppState& state, const std::string& code) {
        return std::string(state.toolbar.entries_path) + safe_basename(code) + ".json";
    }

    // ── Índice ────────────────────────────────────────────────────────────────────

    void load_index(const AppState& state,
                    std::unordered_map<std::string, std::string>& index);

    void save_index(const AppState& state,
                    const std::unordered_map<std::string, std::string>& index);

    // ── Archivos .tex ─────────────────────────────────────────────────────────────

    std::vector<std::string> list_tex_files(const AppState& state);
    std::string read_tex(const AppState& state, const std::string& filename);
    bool        write_tex(const AppState& state,
                          const std::string& filename, const std::string& content);

    // ── JSON de nodo (<basename>.json) ────────────────────────────────────────────

    void load_node_json(const AppState& state, MathNode* sel, EditState& edit);

    void save_node_json(const AppState& state,
                        const MathNode* sel,
                        const EditState& edit);

    // ── load_cross_refs_only ──────────────────────────────────────────────────────
    // Lee SOLO la sección "cross_refs" del JSON de `code` sin tocar ningún MathNode.
    // Usado por entry_editor para construir sugerencias de nodos hermanos sin
    // efectos secundarios sobre los datos en memoria.
    // Devuelve vector vacío si el archivo no existe o no tiene cross_refs.
    std::vector<EditorCrossRef> load_cross_refs_only(const AppState& state,
                                                     const std::string& code);

    // ── Acople al grafo principal ─────────────────────────────────────────────────

    void acople_basic_info (AppState& state, MathNode* root);
    void acople_cross_refs (AppState& state, MathNode* root);
    void acople_resources  (AppState& state, MathNode* root);

} // namespace editor_io
