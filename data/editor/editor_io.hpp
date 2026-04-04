#pragma once
// ── editor_io.hpp ─────────────────────────────────────────────────────────────
// Gestión de persistencia del EntryEditor:
//  · Índice entries_index.json  (code → filename.tex)
//  · Lectura / escritura de archivos .tex
//  · Listado del directorio de entradas
// ─────────────────────────────────────────────────────────────────────────────

#include "app.hpp"
#include "data/math_node.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace editor_io {

    // ── Rutas ─────────────────────────────────────────────────────────────────────

    inline std::string index_path(const AppState& state) {
        return std::string(state.toolbar.entries_path) + "entries_index.json";
    }

    inline std::string full_tex_path(const AppState& state, const std::string& filename) {
        return std::string(state.toolbar.entries_path) + filename;
    }

    /// Genera un nombre de archivo .tex seguro a partir del code de un nodo.
    /// Reemplaza '/', '\\', ':', ' ' y '.' por '_'.
    /// Ejemplos:
    ///   "11-XX"                                  → "11-XX.tex"
    ///   "Mathlib.Order.CompactlyGenerated.Intervals" → "Mathlib_Order_CompactlyGenerated_Intervals.tex"
    inline std::string safe_filename(const std::string& code) {
        std::string safe = code;
        for (char& c : safe)
            if (c == '/' || c == '\\' || c == ':' || c == ' ' || c == '.') c = '_';
        return safe + ".tex";
    }

    // ── Índice en disco ───────────────────────────────────────────────────────────

    /// Carga entries_index.json en el mapa dado. No lanza excepción.
    void load_index(const AppState& state,
        std::unordered_map<std::string, std::string>& index);

    /// Serializa el mapa a entries_index.json. Crea el directorio si no existe.
    void save_index(const AppState& state,
        const std::unordered_map<std::string, std::string>& index);

    // ── Archivos .tex ─────────────────────────────────────────────────────────────

    /// Devuelve la lista de archivos .tex del directorio de entradas, ordenada.
    std::vector<std::string> list_tex_files(const AppState& state);

    /// Lee el contenido de `filename` y lo devuelve como string (vacío si falla).
    std::string read_tex(const AppState& state, const std::string& filename);

    /// Escribe `content` en `filename` (relativo a entries_path).
    /// Crea el directorio si no existe. Devuelve true si tuvo éxito.
    bool write_tex(const AppState& state,
        const std::string& filename, const std::string& content);

} // namespace editor_io