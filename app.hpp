#pragma once
#include "data/math_node.hpp"
#include <string>
#include <vector>
#include <memory>
#include <atomic>

// ── Resultado de búsqueda Loogle ─────────────────────────────────────────────
struct LoogleResult {
    std::string name;       // nombre del teorema/definición
    std::string module;     // ruta en Mathlib
    std::string type_sig;   // tipo/firma
};

// ── Estado global de la aplicación ───────────────────────────────────────────
struct AppState {
    // Navegación
    ViewMode mode = ViewMode::MSC2020;
    std::vector<std::shared_ptr<MathNode>> nav_stack;

    // Selección
    std::string selected_code;
    std::string selected_label;

    // Búsqueda local
    char search_buf[256] = {};

    // Búsqueda Loogle (async)
    char loogle_buf[256]  = {};
    std::vector<LoogleResult> loogle_results;
    std::atomic<bool> loogle_loading{false};
    std::string loogle_error;

    // Scroll del panel de recursos (píxeles desplazados)
    float resource_scroll = 0.0f;

    // Helpers
    MathNode* current() const {
        return nav_stack.empty() ? nullptr : nav_stack.back().get();
    }
    bool can_go_back() const { return nav_stack.size() > 1; }

    void push(std::shared_ptr<MathNode> node) {
        nav_stack.push_back(node);
        selected_code.clear();
        selected_label.clear();
        resource_scroll = 0.0f;
    }
    void pop() {
        if (can_go_back()) {
            nav_stack.pop_back();
            selected_code.clear();
            selected_label.clear();
            resource_scroll = 0.0f;
        }
    }
};
