#include "bubble_stats.hpp"
#include <unordered_map>
#include <string>
#include <queue>

// ── Estado interno del caché ──────────────────────────────────────────────────

static std::unordered_map<std::string, BubbleStats> s_cache;
static const MathNode*  s_last_root   = nullptr;
static ViewMode         s_last_mode   = ViewMode::MSC2020;
static size_t           s_last_cref_sz = 0;   // tamaño del crossref_map al último build

// ── Helpers ───────────────────────────────────────────────────────────────────

// Devuelve true si el nodo `code` tiene al menos una entrada en crossref_map
// que le corresponda (coincidencia exacta o por prefijo del código).
static bool has_crossref(
    const std::string& code,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    if (code.empty() || code == "ROOT") return false;

    // Búsqueda exacta primero
    auto it = cmap.find(code);
    if (it != cmap.end()) {
        bool ok = (mode == ViewMode::Mathlib)
                ? (!it->second.msc.empty() || !it->second.standard.empty())
                : (!it->second.msc.empty() || !it->second.standard.empty());
        if (ok) return true;
    }

    // Búsqueda por prefijo: si alguna clave del mapa empieza con `code`
    // (p.ej. el área "34" cubre "34A12") → el área padre está "parcialmente conectada"
    for (auto& [key, ref] : cmap) {
        if (key.size() >= code.size() && key.substr(0, code.size()) == code) {
            if (!ref.msc.empty() || !ref.standard.empty()) return true;
        }
    }
    return false;
}

// Recursión DFS: rellena el caché para `node` y todos sus descendientes.
// Devuelve {connected, total_leaves}.
static std::pair<bool, int> build_node(
    const MathNode* node,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    if (!node) return {false, 0};

    BubbleStats st;

    if (node->children.empty()) {
        // Hoja
        st.connected = has_crossref(node->code, cmap, mode);
        st.progress  = st.connected ? 1.0f : 0.0f;
        st.weight    = 1;
        s_cache[node->code] = st;
        return {st.connected, 1};
    }

    int   total_children    = (int)node->children.size();
    int   connected_children = 0;
    int   total_weight      = 0;
    bool  any_connected     = false;

    for (auto& child : node->children) {
        auto [child_conn, child_w] = build_node(child.get(), cmap, mode);
        if (child_conn) connected_children++;
        any_connected |= child_conn;
        total_weight  += child_w;
    }

    // El nodo padre también puede tener crossref directa
    bool self_conn = has_crossref(node->code, cmap, mode);
    any_connected |= self_conn;

    st.connected = any_connected;
    st.progress  = (total_children > 0)
                 ? (float)connected_children / (float)total_children
                 : 0.0f;
    st.weight    = std::max(1, total_weight);

    s_cache[node->code] = st;
    return {any_connected, total_weight};
}

// ── API pública ───────────────────────────────────────────────────────────────

void bubble_stats_ensure(
    const MathNode* root,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    // Invalidar si cambió root, modo o tamaño del mapa
    if (root == s_last_root &&
        mode == s_last_mode &&
        cmap.size() == s_last_cref_sz &&
        !s_cache.empty())
        return;  // caché válido

    s_cache.clear();
    s_last_root    = root;
    s_last_mode    = mode;
    s_last_cref_sz = cmap.size();

    if (root) build_node(root, cmap, mode);
}

static const BubbleStats s_default{};

const BubbleStats& bubble_stats_get(const std::string& code) {
    auto it = s_cache.find(code);
    if (it != s_cache.end()) return it->second;
    return s_default;
}

void bubble_stats_clear() {
    s_cache.clear();
    s_last_root    = nullptr;
    s_last_cref_sz = 0;
}
