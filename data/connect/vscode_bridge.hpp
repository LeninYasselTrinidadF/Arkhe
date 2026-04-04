#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// data/vscode_bridge.hpp
//
// Bridge bidireccional Arkhe ↔ VS Code mediante un archivo JSON compartido.
//
// Flujo Arkhe → VS Code:
//   1. bridge_write(state)  escribe posición actual en arkhe_bridge.json
//   2. bridge_launch_vscode(state)  lanza "code <tex_file>" vía ShellExecute
//
// Flujo VS Code → Arkhe:
//   1. bridge_watch.ps1 (tools/) escribe el archivo activo en arkhe_bridge.json
//   2. bridge_poll(state)  se llama cada frame; si hay dato nuevo navega el árbol
//
// El campo "direction" del JSON determina quién escribió el archivo:
//   "arkhe"  → Arkhe lo escribió, VS Code debe leerlo
//   "vscode" → VS Code lo escribió, Arkhe debe leerlo
// ─────────────────────────────────────────────────────────────────────────────

#include "../app.hpp"
#include <string>

// ── Constantes ────────────────────────────────────────────────────────────────

// Ruta del archivo puente (relativa al directorio de trabajo del ejecutable).
static constexpr const char* BRIDGE_FILE = "arkhe_bridge.json";


// ── Estructura del estado del bridge ─────────────────────────────────────────

struct BridgeState {
    std::string direction;   // "arkhe" | "vscode"
    std::string mode;        // "MSC2020" | "Mathlib" | "Standard"
    std::string node_code;   // código del nodo (ej: "Mathlib.Algebra.Group.Basic")
    std::string node_label;  // label legible
    std::string tex_file;    // ruta completa al .tex asociado (puede estar vacía)
    long long   timestamp = 0; // epoch seconds; permite detectar cambios sin releer
};

// ── API pública ───────────────────────────────────────────────────────────────

// Escribe el estado actual de AppState en arkhe_bridge.json (dirección "arkhe").
// Llama esto antes de bridge_launch_vscode o al presionar el botón "→ VS".
void bridge_write(const AppState& state);

// Lee arkhe_bridge.json. Devuelve true si el archivo tiene dirección "vscode"
// y un timestamp distinto al último leído (es decir, dato nuevo de VS Code).
// En ese caso rellena `out` con los datos del bridge.
bool bridge_poll(BridgeState& out);

// Lanza VS Code con el .tex del nodo seleccionado.
// Primero llama bridge_write(), luego ShellExecute("code", tex_file).
// Si tex_file está vacío abre la carpeta entries/ en su lugar.
void bridge_launch_vscode(const AppState& state);

// Lanza VS Code con la carpeta de mathlib
void bridge_launch_vscode_mathlib(const AppState& state);



// Dado un BridgeState (obtenido de bridge_poll), navega AppState al nodo
// indicado. Busca en el árbol del modo correspondiente.
// Devuelve true si se encontró y navegó el nodo.
bool bridge_navigate(AppState& state, const BridgeState& bs);

// Imprime en stdout el estado actual de navegación (útil desde terminal).
void bridge_print_status(const AppState& state);
