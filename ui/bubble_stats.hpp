#pragma once
#include "../app.hpp"
#include "../data/math_node.hpp"
#include <unordered_map>
#include <string>

// ── BubbleStats ───────────────────────────────────────────────────────────────
// Calcula, a partir del crossref_map existente, tres métricas por nodo:
//
//  connected   → ¿tiene al menos una referencia cruzada (directa o en descendientes)?
//  progress    → fracción [0,1] de hijos con al menos una crossref (para el arco)
//  weight      → número total de descendientes (para radio proporcional)
//
// Todo se deriva en runtime del crossref_map; no hace falta tocar MathNode ni JSON.
// Los resultados se cachean en un flat map<code, Stats> que se recalcula cuando
// cambia el mapa (pasar crossref_map al primer uso de cada frame con ensure()).
// ─────────────────────────────────────────────────────────────────────────────

struct BubbleStats {
    bool  connected = false;  // tiene crossref directa o en descendientes
    float progress  = 0.0f;  // hijos con crossref / total hijos  [0,1]
    int   weight    = 1;      // total de hojas/descendientes (para radio)
};

// Computa y cachea las estadísticas para todo el árbol.
// Llama una vez por frame (es barato si el mapa no cambió).
void bubble_stats_ensure(
    const MathNode* root,
    const std::unordered_map<std::string, CrossRef>& crossref_map,
    ViewMode mode);

// Devuelve las stats del nodo con `code`.
// Si no se encontró, devuelve stats por defecto (connected=false, progress=0).
const BubbleStats& bubble_stats_get(const std::string& code);

// Limpia el caché (llamar al recargar assets).
void bubble_stats_clear();
