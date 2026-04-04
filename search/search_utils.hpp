#pragma once
#include "../data/math_node.hpp"
#include <memory>
#include <string>

// Búsqueda exacta por código dentro del árbol.
std::shared_ptr<MathNode> find_node_by_code(
    const std::shared_ptr<MathNode>& root,
    const std::string& target);

// Búsqueda por nombre de módulo Mathlib.
// Primero intenta coincidencia exacta de código; si falla, busca
// el nodo cuyo código sea el prefijo más largo contenido en `module`.
std::shared_ptr<MathNode> find_node_by_module(
    const std::shared_ptr<MathNode>& root,
    const std::string& module);
