#pragma once
#include "../../app.hpp"
#include <string>

// ── nav_to_dep_node ────────────────────────────────────────────────────────────
// Dado un dep_focus_id, navega el nav_stack de burbujas hasta que el nodo
// quede visible como burbuja (el padre en el tope si es hoja, el nodo mismo
// si tiene hijos), y deja selected_code apuntando a él.
//
// Usado al volver de dep_view → bubble_view para mantener la sincronía
// bidireccional entre ambas vistas.
void nav_to_dep_node(AppState& state, const std::string& dep_id);