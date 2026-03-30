#include "dep_graph.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// ── load ──────────────────────────────────────────────────────────────────────
// Espera un archivo con formato:
// {
//   "CODE": { "label": "...", "depends_on": ["CODE1", "CODE2"] },
//   ...
// }
// Las claves que empiezan con "_" se ignoran (comentarios).

void DepGraph::load(const std::string& deps_json_path) {
    clear();
    if (deps_json_path.empty()) return;

    std::ifstream f(deps_json_path);
    if (!f.is_open()) return;

    json j;
    try { j = json::parse(f); }
    catch (...) { return; }

    if (!j.is_object()) return;

    for (auto& [code, val] : j.items()) {
        // Ignorar claves de comentario
        if (code.empty() || code[0] == '_') continue;
        if (!val.is_object()) continue;

        DepNode node;
        node.id    = code;
        node.label = val.value("label", code);

        if (val.contains("depends_on") && val["depends_on"].is_array()) {
            for (auto& dep : val["depends_on"]) {
                if (dep.is_string())
                    node.depends_on.push_back(dep.get<std::string>());
            }
        }

        nodes_[code] = std::move(node);
    }

    build_reverse();
}

// ── clear ─────────────────────────────────────────────────────────────────────

void DepGraph::clear() {
    nodes_.clear();
    dependents_.clear();
}

// ── get ───────────────────────────────────────────────────────────────────────

const DepNode* DepGraph::get(const std::string& id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

// ── get_dependents ────────────────────────────────────────────────────────────

std::vector<std::string> DepGraph::get_dependents(const std::string& id) const {
    auto it = dependents_.find(id);
    return it != dependents_.end() ? it->second : std::vector<std::string>{};
}

// ── build_reverse ─────────────────────────────────────────────────────────────

void DepGraph::build_reverse() {
    dependents_.clear();
    for (auto& [id, node] : nodes_) {
        for (auto& dep_id : node.depends_on)
            dependents_[dep_id].push_back(id);
    }
}
