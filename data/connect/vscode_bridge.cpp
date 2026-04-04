// ─────────────────────────────────────────────────────────────────────────────
// data/vscode_bridge.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include "data/connect/vscode_bridge.hpp"
#include "data/state/nav_state.hpp"
#include "app.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <shellapi.h>

// Como usamos NOUSER para no chocar con Raylib, definimos esto a mano:
#ifndef SW_SHOWDEFAULT
#define SW_SHOWDEFAULT 10
#endif

#pragma comment(lib, "shell32.lib")

// Limpieza post-include para Raylib
#undef CloseWindow
#undef ShowCursor
#endif

#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>
#include <queue>

using json = nlohmann::json;

// ── Helpers internos ──────────────────────────────────────────────────────────

// Cadena del modo actual
static const char* mode_str(ViewMode m) {
    switch (m) {
    case ViewMode::MSC2020:  return "MSC2020";
    case ViewMode::Mathlib:  return "Mathlib";
    case ViewMode::Standard: return "Standard";
    }
    return "MSC2020";
}

// ViewMode desde string
static ViewMode mode_from_str(const std::string& s) {
    if (s == "Mathlib")  return ViewMode::Mathlib;
    if (s == "Standard") return ViewMode::Standard;
    return ViewMode::MSC2020;
}

// BFS genérico sobre el árbol de MathNode
static std::shared_ptr<MathNode>
find_node(const std::shared_ptr<MathNode>& root, const std::string& code) {
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

// Construye la ruta al .tex del nodo seleccionado consultando entries_index.json.
// Devuelve cadena vacía si no hay entrada.
static std::string resolve_tex_path(const AppState& state, const std::string& code) {
    if (code.empty() || code == "ROOT") return {};

    // Leer entries_index.json
    std::string index_path =
        std::string(state.toolbar.entries_path) + "entries_index.json";
    std::ifstream f(index_path);
    if (!f.is_open()) return {};
    try {
        auto j = json::parse(f);
        if (j.contains(code) && j[code].is_string()) {
            std::string filename = j[code].get<std::string>();
            return std::string(state.toolbar.entries_path) + filename;
        }
    } catch (...) {}
    return {};
}

// ── bridge_write ──────────────────────────────────────────────────────────────

void bridge_write(const AppState& state) {
    MathNode* cur  = state.current();
    MathNode* node = nullptr;

    // Nodo seleccionado o nodo actual si nada está seleccionado
    if (cur && !state.selected_code.empty()) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { node = child.get(); break; }
    }
    if (!node) node = cur;
    if (!node) return;

    json j;
    j["direction"]  = "arkhe";
    j["mode"]       = mode_str(state.mode);
    j["node_code"]  = node->code;
    j["node_label"] = node->label;
    j["tex_file"]   = resolve_tex_path(state, node->code);
    j["timestamp"]  = (long long)std::time(nullptr);

    try {
        std::ofstream f(BRIDGE_FILE);
        if (f.is_open()) f << j.dump(2);
        TraceLog(LOG_INFO, "bridge: escrito nodo=%s", node->code.c_str());
    } catch (...) {
        TraceLog(LOG_ERROR, "bridge: error al escribir %s", BRIDGE_FILE);
    }
}

// ── bridge_poll ───────────────────────────────────────────────────────────────

bool bridge_poll(BridgeState& out) {
    static long long last_ts = 0;   // último timestamp procesado

    std::ifstream f(BRIDGE_FILE);
    if (!f.is_open()) return false;

    json j;
    try { j = json::parse(f); }
    catch (...) { return false; }

    // Solo nos interesan mensajes que vengan de VS Code
    if (!j.contains("direction") || j["direction"] != "vscode") return false;

    long long ts = j.value("timestamp", 0LL);
    if (ts <= last_ts) return false;   // dato ya procesado

    last_ts          = ts;
    out.direction    = "vscode";
    out.mode         = j.value("mode",       "MSC2020");
    out.node_code    = j.value("node_code",  "");
    out.node_label   = j.value("node_label", "");
    out.tex_file     = j.value("tex_file",   "");
    out.timestamp    = ts;
    return true;
}

// ── bridge_launch_vscode ──────────────────────────────────────────────────────

void bridge_launch_vscode(const AppState& state) {
    // 1. Actualizar el bridge con la posición actual
    bridge_write(state);

    // 2. Resolver qué abrir: .tex del nodo o carpeta entries/
    MathNode* cur  = state.current();
    std::string target;
    if (cur && !state.selected_code.empty()) {
        target = resolve_tex_path(state, state.selected_code);
    }
    if (target.empty() && cur) {
        target = resolve_tex_path(state, cur->code);
    }
    if (target.empty()) {
        // Fallback: abrir carpeta entries/
        target = state.toolbar.entries_path;
    }

    TraceLog(LOG_INFO, "bridge: lanzando VS Code con '%s'", target.c_str());

#ifdef _WIN32
    // ShellExecute busca "code" en PATH automáticamente
    HINSTANCE r = ShellExecuteA(
        nullptr,      // hwnd
        "open",       // operación
        "code",       // ejecutable (VS Code CLI)
        target.c_str(),
        nullptr,      // directorio de trabajo
        SW_SHOWDEFAULT
    );
    if ((INT_PTR)r <= 32)
        TraceLog(LOG_WARNING, "bridge: ShellExecute falló (code=%d). ¿Está 'code' en PATH?", (int)(INT_PTR)r);
#else
    // Linux/Mac: popen
    std::string cmd = "code \"" + target + "\" &";
    system(cmd.c_str());
#endif
}

static std::string resolve_mathlib_lean_path(const AppState& state, const std::string& code) {
    if (code.empty() || code == "ROOT") return {};

    // Reemplazar puntos por separadores de ruta (conserva el "Mathlib" inicial)
    std::string relative = code;
    for (char& c : relative)
        if (c == '.') c = '/';

    std::string base = state.toolbar.mathlib_src_path;
    if (base.empty()) return {};

    // Asegurar que base termine con separador
    if (!base.empty() && base.back() != '/' && base.back() != '\\')
        base += '/';

    return base + relative + ".lean";
}

void bridge_launch_vscode_mathlib(const AppState& state) {
    // 1. Escribir el estado actual en el puente (como hace el botón VS)
    bridge_write(state);
    std::string target;

    // Si estamos en modo Mathlib y hay un nodo actual (no raíz), abrir su .lean
    if (state.mode == ViewMode::Mathlib) {
        MathNode* cur = state.current();
        if (cur && cur->code != "ROOT") {
            target = resolve_mathlib_lean_path(state, cur->code);
            if (!target.empty())
                TraceLog(LOG_INFO, "bridge: abriendo archivo Mathlib: %s", target.c_str());
        }
    }

    // Fallback: abrir la carpeta raíz de mathlib
    if (target.empty()) {
        target = state.toolbar.mathlib_src_path;
        if (target.empty()) {
            TraceLog(LOG_WARNING, "bridge: mathlib_src_path no configurada");
            return;
        }
        TraceLog(LOG_INFO, "bridge: abriendo carpeta MathLib: %s", target.c_str());
    }

#ifdef _WIN32
    HINSTANCE r = ShellExecuteA(nullptr, "open", "code", target.c_str(), nullptr, SW_SHOWDEFAULT);
    if ((INT_PTR)r <= 32)
        TraceLog(LOG_WARNING, "bridge: ShellExecute falló (code=%d). ¿Está 'code' en PATH?", (int)(INT_PTR)r);
#else
    std::string cmd = "code \"" + target + "\" &";
    system(cmd.c_str());
#endif
}


// ── bridge_navigate ───────────────────────────────────────────────────────────

bool bridge_navigate(AppState& state, const BridgeState& bs) {
    if (bs.node_code.empty()) return false;

    // Seleccionar el árbol correcto según el modo indicado en el bridge
    ViewMode target_mode = mode_from_str(bs.mode);
    std::shared_ptr<MathNode> root;
    switch (target_mode) {
    case ViewMode::MSC2020:  root = state.msc_root;      break;
    case ViewMode::Mathlib:  root = state.mathlib_root;  break;
    case ViewMode::Standard: root = state.standard_root; break;
    }
    if (!root) {
        TraceLog(LOG_WARNING, "bridge: árbol no disponible para modo %s", bs.mode.c_str());
        return false;
    }

    // BFS para encontrar el nodo
    auto node = find_node(root, bs.node_code);
    if (!node) {
        TraceLog(LOG_WARNING, "bridge: nodo '%s' no encontrado en árbol", bs.node_code.c_str());
        return false;
    }

    // Navegar: cambiar modo si hace falta y usar pending_nav
    // (mismo mecanismo que crossref navigation en info_panel)
    state.pending_nav.active = true;
    state.pending_nav.mode   = target_mode;
    state.pending_nav.code   = node->code;
    state.pending_nav.label  = node->label;
    state.pending_nav.root   = root;
    state.pending_nav.node   = node;

    TraceLog(LOG_INFO, "bridge: navegando a '%s' (%s)",
             bs.node_code.c_str(), bs.mode.c_str());
    return true;
}

// ── bridge_print_status ───────────────────────────────────────────────────────

void bridge_print_status(const AppState& state) {
    MathNode* cur  = state.current();
    const char* m  = mode_str(state.mode);

    printf("\n┌─ Arkhe · estado actual ─────────────────────────\n");
    printf("│  modo  : %s\n", m);
    if (cur) {
        printf("│  nodo  : %s\n", cur->code.c_str());
        printf("│  label : %s\n", cur->label.c_str());
        printf("│  hijos : %d\n", (int)cur->children.size());
    } else {
        printf("│  nodo  : (ninguno)\n");
    }
    if (!state.selected_code.empty())
        printf("│  sel   : %s  —  %s\n",
               state.selected_code.c_str(), state.selected_label.c_str());
    std::string tex = cur ? resolve_tex_path(state, cur->code) : "";
    if (!tex.empty())
        printf("│  .tex  : %s\n", tex.c_str());
    printf("└─────────────────────────────────────────────────\n\n");
    fflush(stdout);
}
