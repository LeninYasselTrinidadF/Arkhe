#pragma once
#include "app.hpp"
#include <memory>

// ── nav_state ─────────────────────────────────────────────────────────────────
// Persiste la posición jerárquica de cada modo en state_nav.json.
//
// Formato del archivo:
// {
//   "MSC2020":  { "stack": ["34","34A"], "selected_code": "34A12", "selected_label": "..." },
//   "Mathlib":  { ... },
//   "Standard": { ... }
// }
//
// "stack" contiene los codes del nav_stack a partir del índice 1
// (el índice 0 es siempre la raíz del modo y no se guarda).
//
// Uso en main.cpp — bloque mode!=prev_mode:
//
//   nav_state_save(state, prev_mode);          // antes de limpiar nav_stack
//   state.nav_stack.clear();
//   if(root) state.push(root);                 // empuja raíz del nuevo modo
//   nav_state_load(state, state.mode, root);   // restaura si hay datos
// ─────────────────────────────────────────────────────────────────────────────

static constexpr const char* NAV_STATE_FILE = "state_nav.json";

// Guarda nav_stack + selected del modo `mode` en state_nav.json.
// No modifica AppState.
void nav_state_save(const AppState& state, ViewMode mode);

// Restaura nav_stack (a partir del índice 1) y selected_code/label
// para el modo `mode` desde state_nav.json.
// Precondición: state.nav_stack ya tiene la raíz del modo en [0].
// Si no hay datos guardados o el archivo no existe, no hace nada.
void nav_state_load(AppState& state,
                    ViewMode mode,
                    const std::shared_ptr<MathNode>& root);
