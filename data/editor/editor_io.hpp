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

    /// Nombre base seguro (sin extensión) a partir del code de un nodo.
    inline std::string safe_basename(const std::string& code) {
        std::string s = code;
        for (char& c : s)
            if (c == '/' || c == '\\' || c == ':' || c == ' ' || c == '.') c = '_';
        return s;
    }

    /// .tex compatible con versiones anteriores.
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
    //
    // Estructura:
    //   {
    //     "basic_info": { "note": "...", "texture_key": "...", "msc_refs": [...] },
    //     "cross_refs":  [ { "target_code": "...", "relation": "..." } ],
    //     "resources":   [ { "kind": "...", "title": "...", "content": "..." } ]
    //   }

    /// Carga el JSON del nodo y actualiza sel + edit.cross_refs.
    /// No-op si el archivo no existe (conserva los datos actuales del nodo).
    void load_node_json(const AppState& state, MathNode* sel, EditState& edit);

    /// Serializa el estado actual de sel + edit.cross_refs al JSON del nodo.
    void save_node_json(const AppState& state,
                        const MathNode* sel,
                        const EditState& edit);

    // ── Acople al grafo principal ─────────────────────────────────────────────────
    //
    // Cada función recorre el grafo desde `root`, lee el JSON de cada nodo
    // que tenga archivo y aplica la sección correspondiente al MathNode en vivo.
    //
    // ¡NOTA! MathNode debe tener el campo:
    //   std::vector<CrossRef> cross_refs;
    // para que acople_cross_refs funcione.
    //
    // Sustituye root con tu accessor real: state.root_node, state.graph.root(), etc.

    void acople_basic_info (AppState& state, MathNode* root);
    void acople_cross_refs (AppState& state, MathNode* root);
    void acople_resources  (AppState& state, MathNode* root);

} // namespace editor_io
