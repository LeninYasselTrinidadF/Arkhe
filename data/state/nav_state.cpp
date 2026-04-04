#include "nav_state.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <queue>

using json = nlohmann::json;

// ── Helpers ───────────────────────────────────────────────────────────────────

static const char* mode_key(ViewMode mode) {
    switch (mode) {
    case ViewMode::MSC2020:  return "MSC2020";
    case ViewMode::Mathlib:  return "Mathlib";
    case ViewMode::Standard: return "Standard";
    }
    return "MSC2020";
}

// BFS: devuelve el nodo con code==target, o nullptr.
static std::shared_ptr<MathNode>
find_by_code(const std::shared_ptr<MathNode>& root, const std::string& code)
{
    if (!root || code.empty()) return nullptr;
    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (n->code == code) return n;
        for (auto& c : n->children) q.push(c);
    }
    return nullptr;
}

// Lee el JSON del archivo; devuelve objeto vacío si no existe o está corrupto.
static json read_file() {
    std::ifstream f(NAV_STATE_FILE);
    if (!f.is_open()) return json::object();
    try { return json::parse(f); }
    catch (...) { return json::object(); }
}

// Escribe el JSON en el archivo.
static void write_file(const json& j) {
    try {
        std::ofstream f(NAV_STATE_FILE);
        if (f.is_open()) f << j.dump(2);
    } catch (...) {}
}

// ── nav_state_save ────────────────────────────────────────────────────────────

void nav_state_save(const AppState& state, ViewMode mode) {
    // Códigos del stack a partir de índice 1 (skip root)
    json stack = json::array();
    for (int i = 1; i < (int)state.nav_stack.size(); i++)
        stack.push_back(state.nav_stack[i]->code);

    json entry;
    entry["stack"]          = stack;
    entry["selected_code"]  = state.selected_code;
    entry["selected_label"] = state.selected_label;

    json doc = read_file();
    doc[mode_key(mode)] = entry;
    write_file(doc);
}

// ── nav_state_load ────────────────────────────────────────────────────────────

void nav_state_load(AppState& state,
                    ViewMode mode,
                    const std::shared_ptr<MathNode>& root)
{
    json doc = read_file();
    const char* key = mode_key(mode);
    if (!doc.contains(key)) return;

    const json& entry = doc[key];

    // Reconstruir stack: push_back directo para no limpiar selected en cada paso
    if (entry.contains("stack") && entry["stack"].is_array()) {
        for (auto& code_j : entry["stack"]) {
            std::string code = code_j.get<std::string>();
            auto node = find_by_code(root, code);
            if (!node) break;   // árbol cambió — parar aquí y quedarse en este nivel
            state.nav_stack.push_back(node);
        }
    }

    // Restaurar selección
    if (entry.contains("selected_code") && entry["selected_code"].is_string())
        state.selected_code  = entry["selected_code"].get<std::string>();
    if (entry.contains("selected_label") && entry["selected_label"].is_string())
        state.selected_label = entry["selected_label"].get<std::string>();

    state.resource_scroll = 0.0f;
}
