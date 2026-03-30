#pragma once
#include "../data/math_node.hpp"
#include <vector>

// ── BubblePos ─────────────────────────────────────────────────────────────────
// Posición y radio calculados para un hijo en este frame.
// NO muta MathNode: x/y/radius del JSON no se tocan.
struct BubblePos {
    float x, y, r;   // posición centro y radio de dibujo
    int   node_idx;   // índice en cur->children
};

// Calcula el layout sin superposición para los hijos de `cur`.
//
//   draw_radii : radio de dibujo por hijo (índice = posición en children)
//   center_r   : radio de la burbuja central (zona exclusiva)
//
// Algoritmo:
//   1. Distribuye hijos en anillos concéntricos (mayor→menor radio, adentro→afuera)
//   2. Ángulos equidistantes por anillo, empezando en -90° (arriba)
//   3. Spring-relaxation (20 iter) para separar solapamientos
//   4. Re-proyección suave a la órbita de cada anillo
std::vector<BubblePos> compute_layout(
    const MathNode*        cur,
    const std::vector<float>& draw_radii,
    float                  center_r);
