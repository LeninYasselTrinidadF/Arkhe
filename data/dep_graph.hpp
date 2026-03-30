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
// También acepta códigos de área de 2 dígitos ("34") para agrupar sub-áreas.
// ─────────────────────────────────────────────────────────────────────────────

struct DepNode {
    std::string              id;           // code MSC2020 (puede ser área o sub-área)
    std::string              label;
    std::vector<std::string> depends_on;  // codes de los que este depende
};

class DepGraph {
public:
    // Carga desde un archivo deps.json (ej. "assets/deps.json").
    void load(const std::string& deps_json_path);
    void clear();

    // Búsqueda exacta por id.
    const DepNode* get(const std::string& id) const;

    // Dado el code de un MathNode (puede ser área "34" o sub-área "34A12"),
    // devuelve el mejor DepNode que coincide: primero exacto, luego por prefijo
    // de área (primeros 2 caracteres). Retorna nullptr si no hay coincidencia.
    const DepNode* find_best(const std::string& code) const;

    // Nodos que listan `id` en su depends_on (inverso).
    std::vector<std::string> get_dependents(const std::string& id) const;

    const std::unordered_map<std::string, DepNode>& nodes() const { return nodes_; }
    bool   empty() const { return nodes_.empty(); }
    size_t size()  const { return nodes_.size(); }

private:
    std::unordered_map<std::string, DepNode>                  nodes_;
    std::unordered_map<std::string, std::vector<std::string>> dependents_;
    // Mapa de prefijo de área (2 chars) → primer DepNode con ese prefijo
    std::unordered_map<std::string, std::string>              area_prefix_;

    void build_reverse();
    void build_prefix_index();
};
