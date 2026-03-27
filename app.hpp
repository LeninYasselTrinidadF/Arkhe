#pragma once
#include "data/math_node.hpp"
#include "data/crossref_loader.hpp"
#include "ui/texture_cache.hpp"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>
#include "raylib.h"

struct LoogleResult {
    std::string name;
    std::string module;
    std::string type_sig;
};

struct CamState {
    Vector2 target = { 0,0 };
    float   zoom = 1.0f;
};

// ── Estado del toolbar ────────────────────────────────────────────────────────
struct ToolbarState {
    // (a) Selector de carpeta de assets
    bool  assets_edit_active = false;   // el campo de texto esta activo
    char  assets_path[512]   = "assets";// ruta actual (editable)
    bool  assets_changed     = false;   // senial para recargar assets

    // (b) Panel de documentacion rapida
    bool  docs_open          = false;

    // (c) Editor de entradas (crossref / recursos)
    bool  editor_open        = false;
    char  editor_node_code[256] = {};   // nodo que se esta editando
};

struct AppState {
    ViewMode mode = ViewMode::MSC2020;
    std::vector<std::shared_ptr<MathNode>> nav_stack;

    std::string selected_code;
    std::string selected_label;

    char search_buf[256] = {};
    char loogle_buf[256] = {};

    std::vector<LoogleResult> loogle_results;
    std::atomic<bool> loogle_loading{ false };
    std::string loogle_error;

    float resource_scroll = 0.0f;

    std::shared_ptr<MathNode> mathlib_root;
    std::shared_ptr<MathNode> msc_root;
    std::shared_ptr<MathNode> standard_root;

    std::unordered_map<std::string, CrossRef> crossref_map;
    std::unordered_map<std::string, CamState> cam_memory;

    // ── Sistema de texturas/sprites ───────────────────────────────────────────
    TextureCache textures;

    // ── Estado del toolbar ────────────────────────────────────────────────────
    ToolbarState toolbar;

    // ── Navegacion diferida ───────────────────────────────────────────────────
    // Problema: si info_panel o search_panel escriben state.mode = X y hacen
    // push() en el mismo frame, main.cpp detecta (mode != prev_mode) y ejecuta
    // nav_stack.clear() + push(root), pisando la navegacion recien hecha.
    //
    // Solucion: en lugar de navegar directamente, escribir la intencion aqui.
    // main.cpp la consume al INICIO del siguiente frame, antes del bloque
    // prev_mode, de modo que ese bloque ve mode == prev_mode y no interfiere.
    struct PendingNav {
        bool        active = false;
        ViewMode    mode = ViewMode::MSC2020;
        std::string code;
        std::string label;
        std::shared_ptr<MathNode> root;
        std::shared_ptr<MathNode> node;
    };
    PendingNav pending_nav;

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

    std::string cam_key() const {
        std::string m;
        switch (mode) {
        case ViewMode::MSC2020:  m = "MSC"; break;
        case ViewMode::Mathlib:  m = "ML";  break;
        case ViewMode::Standard: m = "STD"; break;
        }
        MathNode* cur = current();
        return m + ":" + (cur ? cur->code : "ROOT");
    }

    std::string cam_key_for(ViewMode m, const std::string& node_code) const {
        std::string ms;
        switch (m) {
        case ViewMode::MSC2020:  ms = "MSC"; break;
        case ViewMode::Mathlib:  ms = "ML";  break;
        case ViewMode::Standard: ms = "STD"; break;
        }
        return ms + ":" + node_code;
    }

    void save_cam(const Vector2& target, float zoom) {
        cam_memory[cam_key()] = { target, zoom };
    }
    void save_cam(const Camera2D& cam) {
        cam_memory[cam_key()] = { cam.target, cam.zoom };
    }
    void save_cam(const Camera2D& cam, const std::string& key) {
        cam_memory[key] = { cam.target, cam.zoom };
    }

    CamState load_cam() const {
        auto it = cam_memory.find(cam_key());
        if (it != cam_memory.end()) return it->second;
        return {};
    }
    void restore_cam(Camera2D& cam) const {
        auto cs = load_cam();
        cam.target = cs.target;
        cam.zoom = cs.zoom > 0.f ? cs.zoom : 1.f;
    }
};
