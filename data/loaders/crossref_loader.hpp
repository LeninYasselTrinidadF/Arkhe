#pragma once
#include "data/math_node.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Entrada del mapa de referencias cruzadas
struct CrossRef {
    std::vector<std::string> msc;       // codigos MSC: "34A12", "37Cxx"
    std::vector<std::string> standard;  // temas del temario Estandar
};

// Carga assets/crossref.json y devuelve el mapa
// key: modulo Mathlib exacto ("Mathlib.Data.Real.Basic")
std::unordered_map<std::string, CrossRef> load_crossref(const std::string& path);

// Inyecta las refs en los nodos Mathlib del arbol
// Llama esto despues de load_mathlib() y load_crossref()
void inject_crossrefs(
    MathNode* mathlib_root,
    const std::unordered_map<std::string, CrossRef>& refs);
