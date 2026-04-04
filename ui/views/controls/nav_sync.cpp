#include "ui/views/controls/nav_sync.hpp"
#include <queue>
#include <vector>

void nav_to_dep_node(AppState& state, const std::string& dep_id) {
    if (dep_id.empty()) return;

    std::shared_ptr<MathNode> root;
    switch (state.mode) {
    case ViewMode::MSC2020:  root = state.msc_root;      break;
    case ViewMode::Mathlib:  root = state.mathlib_root;  break;
    case ViewMode::Standard: root = state.standard_root; break;
    }
    if (!root) return;

    // BFS: cada entrada es (nodo, camino desde la raíz inclusive)
    using Path = std::vector<std::shared_ptr<MathNode>>;
    std::queue<std::pair<std::shared_ptr<MathNode>, Path>> q;
    q.push({ root, { root } });

    std::shared_ptr<MathNode> found;
    Path found_path;

    while (!q.empty()) {
        auto [node, path] = q.front(); q.pop();

        // Coincidencia exacta, o el code del árbol es prefijo del dep_id
        // (ej. dep_id="34A12", node->code="34A12" | "34A" | "34")
        bool match = (node->code == dep_id)
            || (!dep_id.empty()
                && dep_id.rfind(node->code, 0) == 0
                && dep_id.size() > node->code.size()
                && (dep_id[node->code.size()] == '.'
                    || dep_id[node->code.size()] == '_'));

        if (match) {
            if (!node->children.empty()) {
                // Nodo con hijos: quedarse aquí como tope del stack
                found = node;
                found_path = path;
            }
            else if (path.size() >= 2) {
                // Hoja: quedarse en el padre, nodo queda como selección
                found_path = Path(path.begin(), path.end() - 1);
                found = found_path.back();
            }
            else {
                found = node;
                found_path = path;
            }
            break;
        }

        for (auto& child : node->children) {
            Path child_path = path;
            child_path.push_back(child);
            q.push({ child, child_path });
        }
    }

    if (!found) return;  // dep_id no existe en este árbol — no tocar nada

    state.nav_stack.clear();
    for (auto& n : found_path)
        state.nav_stack.push_back(n);

    state.selected_code = dep_id;
    state.selected_label = found_path.back()->label;
    state.resource_scroll = 0.f;
}