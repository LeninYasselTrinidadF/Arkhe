#pragma once
// ── section_data.hpp ──────────────────────────────────────────────────────────
// Tipos de datos para las secciones JSON del EntryEditor.
// Incluido por edit_state.hpp, editor_io.hpp y los .cpp de secciones.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>

// ── CrossRef ──────────────────────────────────────────────────────────────────
// Enlace tipado entre dos nodos del grafo matemático.

struct EditorCrossRef {
    std::string target_code;  // code del nodo destino
    std::string relation;     // ver xref_relation_label()
};

// Etiqueta de visualización para un tipo de relación.
inline const char* xref_relation_label(const std::string& rel) {
    if (rel == "requires")      return "Requiere";
    if (rel == "extends")       return "Extiende";
    if (rel == "example_of")    return "Ej. de";
    if (rel == "generalizes")   return "Generaliza";
    if (rel == "equivalent_to") return "Equivale a";
    return "Ver tambien";   // "see_also" (default)
}

// Tipos de relación disponibles (parallel arrays).
inline const char* XREF_RELATIONS[] = {
    "see_also", "requires", "extends",
    "example_of", "generalizes", "equivalent_to"
};
inline constexpr int XREF_RELATION_COUNT = 6;
