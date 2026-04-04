// ─────────────────────────────────────────────────────────────────────────────
// data/terminal_input.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include "data/connect/terminal_input.hpp"
#include "data/connect/vscode_bridge.hpp"
#include "raylib.h"

#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>

// ── Cola thread-safe ──────────────────────────────────────────────────────────

static std::queue<std::string> s_cmd_queue;
static std::mutex              s_mutex;

// ── Helpers de string ─────────────────────────────────────────────────────────

static std::string to_lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::vector<std::string> split_args(const std::string& line) {
    std::istringstream ss(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// ── Hilo stdin ────────────────────────────────────────────────────────────────

static void stdin_thread_fn() {
    // En Windows, la consola asociada al .exe es la terminal existente.
    // getline bloquea hasta que el usuario pulsa Enter; no consume CPU.
    std::string line;
    terminal_print_prompt();
    while (std::getline(std::cin, line)) {
        if (!line.empty()) {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_cmd_queue.push(line);
        }
        // El prompt se imprime en el main thread después de procesar,
        // para que no se entremezcle con salida de raylib TraceLog.
    }
}

// ── terminal_input_init ───────────────────────────────────────────────────────

void terminal_input_init() {
    // detach: el hilo vive hasta que el proceso muere
    std::thread(stdin_thread_fn).detach();
}

// ── terminal_print_prompt ─────────────────────────────────────────────────────

void terminal_print_prompt() {
    printf("arkhe> ");
    fflush(stdout);
}

// ── BFS helper para find ──────────────────────────────────────────────────────

static std::shared_ptr<MathNode>
find_node_bfs(const std::shared_ptr<MathNode>& root, const std::string& query) {
    if (!root) return nullptr;
    std::string q = to_lower(query);
    std::queue<std::shared_ptr<MathNode>> bfs;
    bfs.push(root);
    while (!bfs.empty()) {
        auto n = bfs.front(); bfs.pop();
        if (n->code != "ROOT") {
            if (to_lower(n->code).find(q) != std::string::npos ||
                to_lower(n->label).find(q) != std::string::npos)
                return n;
        }
        for (auto& c : n->children) bfs.push(c);
    }
    return nullptr;
}

// ── Raíz del modo actual ──────────────────────────────────────────────────────

static std::shared_ptr<MathNode> current_root(AppState& state) {
    switch (state.mode) {
    case ViewMode::Mathlib:  return state.mathlib_root;
    case ViewMode::Standard: return state.standard_root;
    default:                 return state.msc_root;
    }
}

// ── Dispatch de comandos ──────────────────────────────────────────────────────

static void dispatch(AppState& state, const std::string& raw) {
    auto args = split_args(raw);
    if (args.empty()) return;
    std::string cmd = to_lower(args[0]);

    // ── pwd ───────────────────────────────────────────────────────────────────
    if (cmd == "pwd") {
        bridge_print_status(state);
        return;
    }

    // ── ls [n] ────────────────────────────────────────────────────────────────
    if (cmd == "ls") {
        MathNode* cur = state.current();
        if (!cur || cur->children.empty()) {
            printf("  (sin hijos)\n");
            return;
        }
        int limit = 20;
        if (args.size() >= 2) {
            try { limit = std::stoi(args[1]); } catch (...) {}
        }
        int n = (int)cur->children.size();
        printf("  %s  [%d hijos]\n", cur->label.c_str(), n);
        for (int i = 0; i < std::min(n, limit); i++) {
            auto& ch = cur->children[i];
            printf("  [%2d]  %-30s  %s\n",
                   i, ch->code.c_str(), ch->label.c_str());
        }
        if (n > limit)
            printf("  ... y %d más. Usa 'ls %d' para ver todos.\n", n - limit, n);
        printf("\n");
        fflush(stdout);
        return;
    }

    // ── cd <nombre|índice> ────────────────────────────────────────────────────
    if (cmd == "cd") {
        if (args.size() < 2) { printf("  uso: cd <nombre|índice>\n"); return; }
        MathNode* cur = state.current();
        if (!cur || cur->children.empty()) { printf("  sin hijos\n"); return; }

        std::string target = args[1];
        std::shared_ptr<MathNode> found;

        // Intento por índice
        bool is_num = !target.empty() &&
            std::all_of(target.begin(), target.end(), ::isdigit);
        if (is_num) {
            int idx = std::stoi(target);
            if (idx >= 0 && idx < (int)cur->children.size())
                found = cur->children[idx];
        }

        // Intento por código/label (case-insensitive, parcial)
        if (!found) {
            std::string tl = to_lower(target);
            for (auto& ch : cur->children) {
                if (to_lower(ch->code).find(tl) != std::string::npos ||
                    to_lower(ch->label).find(tl) != std::string::npos) {
                    found = ch;
                    break;
                }
            }
        }

        if (!found) { printf("  no encontrado: %s\n", target.c_str()); return; }

        if (!found->children.empty()) {
            state.push(found);
            printf("  → %s  (%s)\n", found->label.c_str(), found->code.c_str());
        } else {
            // Nodo hoja: seleccionar sin navegar
            state.selected_code  = found->code;
            state.selected_label = found->label;
            printf("  sel: %s  (%s)  [nodo hoja]\n",
                   found->label.c_str(), found->code.c_str());
        }
        fflush(stdout);
        return;
    }

    // ── back ──────────────────────────────────────────────────────────────────
    if (cmd == "back") {
        if (state.can_go_back()) {
            state.pop();
            MathNode* cur = state.current();
            printf("  ← %s\n", cur ? cur->label.c_str() : "ROOT");
        } else {
            printf("  ya estás en la raíz\n");
        }
        fflush(stdout);
        return;
    }

    // ── open ──────────────────────────────────────────────────────────────────
    if (cmd == "open") {
        bridge_launch_vscode(state);
        printf("  VS Code lanzado.\n");
        fflush(stdout);
        return;
    }

    // ── sync ──────────────────────────────────────────────────────────────────
    if (cmd == "sync") {
        BridgeState bs;
        if (bridge_poll(bs)) {
            if (bridge_navigate(state, bs))
                printf("  sync: navegado a '%s'\n", bs.node_code.c_str());
            else
                printf("  sync: nodo '%s' no encontrado\n", bs.node_code.c_str());
        } else {
            printf("  sync: sin datos nuevos de VS Code\n");
        }
        fflush(stdout);
        return;
    }

    // ── dep ───────────────────────────────────────────────────────────────────
    if (cmd == "dep") {
        // Activar dep_view. La inicialización real ocurre en main/dep_view
        // al detectar que dep_view_active cambia a true con dep_focus_id set.
        MathNode* cur = state.current();
        std::string focus = state.selected_code.empty()
            ? (cur ? cur->code : "") : state.selected_code;
        if (focus.empty() || focus == "ROOT") {
            printf("  dep: selecciona un nodo primero\n");
        } else {
            state.dep_focus_id    = focus;
            state.dep_view_active = true;
            printf("  dep: activando vista de dependencias para '%s'\n",
                   focus.c_str());
        }
        fflush(stdout);
        return;
    }

    // ── find <query> ──────────────────────────────────────────────────────────
    if (cmd == "find") {
        if (args.size() < 2) { printf("  uso: find <query>\n"); return; }
        std::string query = raw.substr(raw.find(args[1])); // resto de la línea
        auto root = current_root(state);
        auto node = find_node_bfs(root, query);
        if (!node) {
            printf("  find: no encontrado: %s\n", query.c_str());
        } else {
            printf("  find: %s  (%s)\n", node->label.c_str(), node->code.c_str());
            // Navegar usando pending_nav
            state.pending_nav.active = true;
            state.pending_nav.mode   = state.mode;
            state.pending_nav.code   = node->code;
            state.pending_nav.label  = node->label;
            state.pending_nav.root   = root;
            state.pending_nav.node   = node;
        }
        fflush(stdout);
        return;
    }

    // ── mode <m> ──────────────────────────────────────────────────────────────
    if (cmd == "mode") {
        if (args.size() < 2) {
            printf("  uso: mode <msc|mathlib|std>\n"); return;
        }
        std::string m = to_lower(args[1]);
        ViewMode nm = state.mode;
        if      (m == "msc"     || m == "msc2020")  nm = ViewMode::MSC2020;
        else if (m == "mathlib" || m == "ml")        nm = ViewMode::Mathlib;
        else if (m == "std"     || m == "standard")  nm = ViewMode::Standard;
        else { printf("  modo desconocido: %s\n", args[1].c_str()); return; }
        state.mode = nm;
        printf("  modo cambiado a %s\n", args[1].c_str());
        fflush(stdout);
        return;
    }

    // ── help ──────────────────────────────────────────────────────────────────
    if (cmd == "help" || cmd == "?") {
        printf(
            "\n  Comandos Arkhe terminal:\n"
            "  ─────────────────────────────────────────────────────\n"
            "  pwd              estado actual (modo, nodo, .tex)\n"
            "  ls [n]           listar hijos del nodo actual\n"
            "  cd <nombre|idx>  navegar a hijo\n"
            "  back             subir un nivel\n"
            "  open             abrir nodo actual en VS Code\n"
            "  sync             leer bridge → navegar a nodo de VS Code\n"
            "  dep              activar vista de dependencias\n"
            "  find <query>     buscar nodo en todo el árbol\n"
            "  mode <m>         cambiar modo: msc | mathlib | std\n"
            "  help             esta ayuda\n"
            "  ─────────────────────────────────────────────────────\n\n"
        );
        fflush(stdout);
        return;
    }

    printf("  comando desconocido: '%s'  (escribe 'help' para ayuda)\n", cmd.c_str());
    fflush(stdout);
}

// ── terminal_input_poll ───────────────────────────────────────────────────────

void terminal_input_poll(AppState& state) {
    // Vaciar la cola de comandos pendientes (puede haber más de uno por frame)
    std::queue<std::string> local;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::swap(local, s_cmd_queue);
    }

    bool had_cmds = !local.empty();
    while (!local.empty()) {
        dispatch(state, local.front());
        local.pop();
    }

    // Reimprimir el prompt solo si hubo actividad
    if (had_cmds) terminal_print_prompt();
}
