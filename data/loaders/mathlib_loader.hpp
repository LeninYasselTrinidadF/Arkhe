#pragma once
#include "data/math_node.hpp"
#include <string>
#include <memory>

// Carga mathlib_layout.json y construye el arbol de navegacion
// Estructura: root -> folders -> subfolders -> files -> declarations
std::shared_ptr<MathNode> load_mathlib(const std::string& path);
