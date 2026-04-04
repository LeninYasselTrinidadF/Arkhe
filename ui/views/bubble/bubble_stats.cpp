#include "ui/views/bubble/bubble_stats.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>

// ── Estado interno del caché ──────────────────────────────────────────────────

static std::unordered_map<std::string, BubbleStats> s_cache;

// Índices inversos: en modo MSC2020/Standard el crossref_map está keyed por
// códigos Mathlib y apunta a MSC/Standard. Para saber si un nodo MSC2020 o
// Standard tiene crossref necesitamos buscar por valor, no por clave.
// Construimos estos sets una sola vez por build.
static std::unordered_set<std::string> s_msc_codes;   // MSC codes que tienen crossref
static std::unordered_set<std::string> s_std_codes;   // Standard codes que tienen crossref

static const MathNode* s_last_root = nullptr;
static ViewMode         s_last_mode = ViewMode::MSC2020;
static size_t           s_last_cref_sz = 0;

// ── Helpers ───────────────────────────────────────────────────────────────────

// Construye los índices inversos a partir del crossref_map.
static void build_reverse_indices(
    const std::unordered_map<std::string, CrossRef>& cmap)
{
    s_msc_codes.clear();
    s_std_codes.clear();
    for (auto& [key, ref] : cmap) {
        for (auto& m : ref.msc)      s_msc_codes.insert(m);
        for (auto& s : ref.standard) s_std_codes.insert(s);
    }
}

// Devuelve true si el nodo `code` tiene al menos una crossref según el modo.
static bool has_crossref(
    const std::string& code,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    if (code.empty() || code == "ROOT") return false;

    // ── Mathlib: el mapa está keyed por códigos Mathlib ───────────────────────
    if (mode == ViewMode::Mathlib) {
        auto it = cmap.find(code);
        if (it != cmap.end() && (!it->second.msc.empty() || !it->second.standard.empty()))
            return true;
        // Búsqueda por prefijo: si alguna clave del mapa empieza con `code`
        for (auto& [key, ref] : cmap) {
            if (key.size() >= code.size() && key.substr(0, code.size()) == code)
                if (!ref.msc.empty() || !ref.standard.empty()) return true;
        }
        return false;
    }

    // ── MSC2020: busca en el índice inverso de códigos MSC ────────────────────
    if (mode == ViewMode::MSC2020) {
        if (s_msc_codes.count(code)) return true;
        // Prefijo: ¿algún código MSC en el índice empieza con `code`?
        for (auto& msc : s_msc_codes) {
            if (msc.size() >= code.size() && msc.substr(0, code.size()) == code)
                return true;
        }
        return false;
    }

    // ── Standard: índice inverso de códigos Standard ──────────────────────────
    if (mode == ViewMode::Standard) {
        if (s_std_codes.count(code)) return true;
        for (auto& std_code : s_std_codes) {
            if (std_code.size() >= code.size() && std_code.substr(0, code.size()) == code)
                return true;
        }
        return false;
    }

    return false;
}

// Recursión DFS: rellena el caché para `node` y todos sus descendientes.
// Devuelve {any_connected, total_leaves}.
static std::pair<bool, int> build_node(
    const MathNode* node,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    if (!node) return { false, 0 };

    BubbleStats st;

    if (node->children.empty()) {
        // Hoja
        st.connected = has_crossref(node->code, cmap, mode);
        st.progress = st.connected ? 1.0f : 0.0f;
        st.weight = 1;
        s_cache[node->code] = st;
        return { st.connected, 1 };
    }

    int  total_children = (int)node->children.size();
    int  connected_children = 0;
    int  total_weight = 0;
    bool any_connected = false;

    for (auto& child : node->children) {
        auto [child_conn, child_w] = build_node(child.get(), cmap, mode);
        if (child_conn) connected_children++;
        any_connected |= child_conn;
        total_weight += child_w;
    }

    // El nodo padre también puede tener crossref directa
    bool self_conn = has_crossref(node->code, cmap, mode);
    any_connected |= self_conn;

    st.connected = any_connected;
    st.progress = (total_children > 0)
        ? (float)connected_children / (float)total_children
        : 0.0f;
    st.weight = std::max(1, total_weight);

    s_cache[node->code] = st;
    return { any_connected, total_weight };
}

// ── API pública ───────────────────────────────────────────────────────────────

void bubble_stats_ensure(
    const MathNode* root,
    const std::unordered_map<std::string, CrossRef>& cmap,
    ViewMode mode)
{
    if (root == s_last_root &&
        mode == s_last_mode &&
        cmap.size() == s_last_cref_sz &&
        !s_cache.empty())
        return;  // caché válido

    s_cache.clear();
    s_last_root = root;
    s_last_mode = mode;
    s_last_cref_sz = cmap.size();

    // Reconstruir índices inversos (necesarios para MSC2020 y Standard)
    build_reverse_indices(cmap);

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
    s_msc_codes.clear();
    s_std_codes.clear();
    s_last_root = nullptr;
    s_last_cref_sz = 0;
}