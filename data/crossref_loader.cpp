#include "crossref_loader.hpp"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <queue>

using json = nlohmann::json;

std::unordered_map<std::string, CrossRef> load_crossref(const std::string& path) {
    std::unordered_map<std::string, CrossRef> result;
    std::ifstream f(path);
    if (!f.is_open()) {
        TraceLog(LOG_WARNING, "crossref: no se encontro %s", path.c_str());
        return result;
    }
    try {
        auto j = json::parse(f);
        for (auto& [module, entry] : j.items()) {
            if (module.starts_with("_")) continue;  // skip comentarios
            CrossRef ref;
            if (entry.contains("msc"))
                for (auto& c : entry["msc"]) ref.msc.push_back(c.get<std::string>());
            if (entry.contains("standard"))
                for (auto& s : entry["standard"]) ref.standard.push_back(s.get<std::string>());
            result[module] = std::move(ref);
        }
        TraceLog(LOG_INFO, "crossref: %d entradas cargadas", (int)result.size());
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "crossref parse error: %s", e.what());
    }
    return result;
}

void inject_crossrefs(
    MathNode* root,
    const std::unordered_map<std::string, CrossRef>& refs)
{
    if (!root) return;
    std::queue<MathNode*> q;
    q.push(root);
    int injected = 0;
    while (!q.empty()) {
        MathNode* node = q.front(); q.pop();
        // El code de un nodo archivo es el modulo exacto: "Mathlib.Data.Real.Basic"
        auto it = refs.find(node->code);
        if (it != refs.end()) {
            node->msc_refs      = it->second.msc;
            node->standard_refs = it->second.standard;
            injected++;
        }
        for (auto& child : node->children) q.push(child.get());
    }
    TraceLog(LOG_INFO, "crossref: %d nodos anotados", injected);
}
