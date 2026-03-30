#include "dep_graph.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

// ── load ──────────────────────────────────────────────────────────────────────

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
        if (code.empty() || code[0] == '_') continue;
        if (!val.is_object()) continue;

        DepNode node;
        node.id    = code;
        node.label = val.value("label", code);

        if (val.contains("depends_on") && val["depends_on"].is_array()) {
            for (auto& dep : val["depends_on"]) {
                if (dep.is_string()) {
                    std::string dep_str = dep.get<std::string>();
                    // Guardia: evitar strings vacíos o demasiado largos
                    if (!dep_str.empty() && dep_str.size() < 64)
                        node.depends_on.push_back(std::move(dep_str));
                }
            }
        }

        nodes_[code] = std::move(node);
    }

    build_reverse();
    build_prefix_index();
}

// ── clear ─────────────────────────────────────────────────────────────────────

void DepGraph::clear() {
    nodes_.clear();
    dependents_.clear();
    area_prefix_.clear();
}

// ── get ───────────────────────────────────────────────────────────────────────

const DepNode* DepGraph::get(const std::string& id) const {
    if (id.empty()) return nullptr;
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

// ── find_best ─────────────────────────────────────────────────────────────────
// Intenta: (1) coincidencia exacta, (2) prefijo de área de 2 chars.

const DepNode* DepGraph::find_best(const std::string& code) const {
    if (code.empty()) return nullptr;

    // 1. Exacto
    auto it = nodes_.find(code);
    if (it != nodes_.end()) return &it->second;

    // 2. Prefijo de área (primeros 2 chars numéricos)
    if (code.size() >= 2) {
        std::string prefix = code.substr(0, 2);
        auto pit = area_prefix_.find(prefix);
        if (pit != area_prefix_.end()) {
            auto nit = nodes_.find(pit->second);
            if (nit != nodes_.end()) return &nit->second;
        }
    }

    return nullptr;
}

// ── get_dependents ────────────────────────────────────────────────────────────

std::vector<std::string> DepGraph::get_dependents(const std::string& id) const {
    if (id.empty()) return {};
    auto it = dependents_.find(id);
    return it != dependents_.end() ? it->second : std::vector<std::string>{};
}

// ── build_reverse ─────────────────────────────────────────────────────────────

void DepGraph::build_reverse() {
    dependents_.clear();
    for (auto& [id, node] : nodes_) {
        for (auto& dep_id : node.depends_on) {
            if (!dep_id.empty())
                dependents_[dep_id].push_back(id);
        }
    }
}

// ── build_prefix_index ────────────────────────────────────────────────────────

void DepGraph::build_prefix_index() {
    area_prefix_.clear();
    for (auto& [id, node] : nodes_) {
        if (id.size() >= 2) {
            std::string prefix = id.substr(0, 2);
            // Solo guardar el primer nodo encontrado por prefijo (estable)
            area_prefix_.emplace(prefix, id);
        }
    }
}
