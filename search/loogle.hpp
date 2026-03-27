#pragma once
#include "../app.hpp"
#include <string>
#include <vector>
#include <functional>

// ── Búsqueda local fuzzy ──────────────────────────────────────────────────────

// Score fuzzy simple: cuántos caracteres de query aparecen en orden en target
// Retorna 0 si no hay match, >0 si hay (mayor = mejor match)
int fuzzy_score(const std::string& query, const std::string& target);

struct SearchHit {
    std::shared_ptr<MathNode> node;
    int score;
};

// Busca en todo el subárbol del nodo actual (BFS limitado)
std::vector<SearchHit> fuzzy_search(
    const MathNode* root,
    const std::string& query,
    int max_results = 30
);

// ── Loogle (HTTP async) ───────────────────────────────────────────────────────

// Lanza búsqueda en thread separado. Escribe en state.loogle_results
// cuando termina. state.loogle_loading indica si está en curso.
void loogle_search_async(AppState& state, const std::string& query);
