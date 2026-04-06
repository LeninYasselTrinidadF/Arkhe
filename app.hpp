#pragma once
#include "data/math_node.hpp"
#include "data/loaders/crossref_loader.hpp"
#include "data/loaders/dep_graph.hpp"
#include "data/gen/mathlib_gen.hpp"
#include "ui/aesthetic/texture_cache.hpp"
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
enum class ToolbarTab { None, Ubicaciones, Docs, Editor, Config };

struct ToolbarState {
    ToolbarTab active_tab = ToolbarTab::None;

    // Rutas independientes
    char assets_path[512] = "assets/";
    char entries_path[512] = "assets/entries/";
    char graphics_path[512] = "assets/graphics/";
    char latex_path[512] = "C:/texlive/2025/bin/windows/pdflatex.exe";
    char pdftoppm_path[512] = "C:/texlive/2025/bin/windows/pdftoppm.exe";
    char mathlib_src_path[512] = "C:/Users/USUARIO/Documents/mathlib_clone";

    // ── Rutas de override para JSONs Mathlib ──────────────────────────────────
    char mathlib_layout_override[512] = {};
    char mathlib_deps_override[512] = {};

    bool assets_changed = false;
    int  active_field = -1;

    // Paneles flotantes
    bool docs_open = false;
    bool editor_open = false;
    bool ubicaciones_open = false;
    bool config_open = false;

    // ── Tema visual ───────────────────────────────────────────────────────────
    int  theme_id = 0;

    // ── Tab focus fix ─────────────────────────────────────────────────────────
    // Cuando kbnav_toolbar activa un TextField vía Tab, este flag indica a
    // draw_text_field que active la captura de chars sin necesitar un clic.
    // draw_text_field debe consumirlo (ponerlo a false) tras procesarlo.
    bool force_field_activate = false;
};

// ── Estado de render LaTeX ────────────────────────────────────────────────────
enum class LatexRenderState { Idle, Compiling, Ready, Failed };

struct LatexRenderJob {
    std::string         tex_code;
    std::string         tex_path;
    std::string         png_path;
    LatexRenderState    state = LatexRenderState::Idle;
    std::string         error_msg;
    Texture2D           texture = {};
    bool                tex_loaded = false;
    std::atomic<bool>   thread_done{ false };
    bool                kb_trigger = false;
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

    TextureCache textures;
    ToolbarState toolbar;
    LatexRenderJob latex_render;

    // ── Vista de dependencias ─────────────────────────────────────────────────
    bool        dep_view_active = false;
    std::string dep_focus_id;
    DepGraph    dep_graph_msc;
    DepGraph    dep_graph_mathlib;
    DepGraph    dep_graph_standard;
    DepGraph    dep_graph;

    bool position_mode_active = false;
    std::unordered_map<std::string, Vector2> temp_positions;

    // ── Generadores Mathlib ───────────────────────────────────────────────────
    MathlibGenJob mathlib_layout_job;
    MathlibGenJob mathlib_deps_job;

    struct PendingNav {
        bool        active = false;
        ViewMode    mode = ViewMode::MSC2020;
        std::string code;
        std::string label;
        std::shared_ptr<MathNode> root;
        std::shared_ptr<MathNode> node;
    };
    PendingNav pending_nav;

    // ── Cross-ref picker ──────────────────────────────────────────────────────
    // Modo interactivo: el usuario navega el grafo y presiona R sobre un nodo
    // para seleccionarlo como referencia cruzada. El editor se cierra mientras
    // dura el picker y se reabre con el resultado cuando el usuario confirma.
    bool        crossref_picker_active  = false;
    std::string crossref_picker_result; // código del nodo seleccionado (vacío si pendiente)

    // Snapshot de navegación guardado al activar el picker.
    // Se restaura al confirmar (R) o cancelar (Esc).
    std::vector<std::shared_ptr<MathNode>> crossref_picker_saved_stack;
    std::string crossref_picker_saved_code;
    std::string crossref_picker_saved_label;
    bool        crossref_picker_saved_dep    = false;
    std::string crossref_picker_saved_dep_id;

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

    // ── Helpers de picker ─────────────────────────────────────────────────────

    /// Activa el modo picker: guarda la nav actual y cierra el editor.
    void crossref_picker_begin() {
        crossref_picker_active      = true;
        crossref_picker_saved_stack = nav_stack;
        crossref_picker_saved_code  = selected_code;
        crossref_picker_saved_label = selected_label;
        crossref_picker_saved_dep   = dep_view_active;
        crossref_picker_saved_dep_id= dep_focus_id;
        toolbar.editor_open         = false;
    }

    /// Confirma el picker con `code` como resultado: restaura nav y reabre editor.
    void crossref_picker_confirm(const std::string& code) {
        crossref_picker_result  = code;
        crossref_picker_active  = false;
        nav_stack               = crossref_picker_saved_stack;
        selected_code           = crossref_picker_saved_code;
        selected_label          = crossref_picker_saved_label;
        dep_view_active         = crossref_picker_saved_dep;
        dep_focus_id            = crossref_picker_saved_dep_id;
        toolbar.editor_open     = true;
    }

    /// Cancela el picker sin resultado: restaura nav y reabre editor.
    void crossref_picker_cancel() {
        crossref_picker_active  = false;
        nav_stack               = crossref_picker_saved_stack;
        selected_code           = crossref_picker_saved_code;
        selected_label          = crossref_picker_saved_label;
        dep_view_active         = crossref_picker_saved_dep;
        dep_focus_id            = crossref_picker_saved_dep_id;
        toolbar.editor_open     = true;
    }
};

// Helper: obtener grafo de dependencias correspondiente al modo actual
inline DepGraph& get_dep_graph_for(AppState& s) {
    switch (s.mode) {
    case ViewMode::MSC2020:  return s.dep_graph_msc;
    case ViewMode::Mathlib:  return s.dep_graph_mathlib;
    case ViewMode::Standard: return s.dep_graph_standard;
    }
    return s.dep_graph_msc;
}

inline const DepGraph& get_dep_graph_for_const(const AppState& s) {
    switch (s.mode) {
    case ViewMode::MSC2020:  return s.dep_graph_msc;
    case ViewMode::Mathlib:  return s.dep_graph_mathlib;
    case ViewMode::Standard: return s.dep_graph_standard;
    }
    return s.dep_graph_msc;
}
