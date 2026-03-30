#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ── DepGraph ──────────────────────────────────────────────────────────────────
// Carga assets/deps.json y construye un DAG de dependencias entre nodos MSC2020.
//
// Formato de deps.json:
// {
//   "CODE": { "label": "...", "depends_on": ["CODE1", "CODE2"] },
//   ...
// }
//
// El code es el mismo que usan los MathNode del árbol (ej. "34A12").
// ─────────────────────────────────────────────────────────────────────────────

struct DepNode {
    std::string              id;           // code MSC2020
    std::string              label;
    std::vector<std::string> depends_on;  // codes de los que este depende
};

class DepGraph {
public:
    // Carga desde un archivo deps.json (ej. "assets/deps.json").
    void load(const std::string& deps_json_path);
    void clear();

    const DepNode* get(const std::string& id) const;

    // Nodos que listan `id` en su depends_on (inverso).
    std::vector<std::string> get_dependents(const std::string& id) const;

    const std::unordered_map<std::string, DepNode>& nodes() const { return nodes_; }
    bool   empty() const { return nodes_.empty(); }
    size_t size()  const { return nodes_.size(); }

private:
    std::unordered_map<std::string, DepNode>                  nodes_;
    std::unordered_map<std::string, std::vector<std::string>> dependents_;

    void build_reverse();
};
