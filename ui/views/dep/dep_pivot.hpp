#pragma once
// ── dep_pivot.hpp ─────────────────────────────────────────────────────────────
// Modo "pivote rojo" en dep_view.
//
// Mientras Ctrl está presionado, existe un segundo cursor (rojo) que actúa
// como nodo central. Los nodos conectados a él se organizan en anillos
// concéntricos lógicos según distancia BFS:
//
//   Anillo -N … -1  upstream   (nodos de los que el pivote depende)
//   Anillo  0       el pivote  (centro, cursor rojo)
//   Anillo +1 … +N downstream (nodos que dependen del pivote)
//
// Teclas con Ctrl mantenido:
//   Izquierda → anillo más negativo (upstream más lejano)
//   Derecha   → anillo más positivo (downstream más lejano)
//   Arriba    → slot anterior en el anillo actual (anti-horario)
//   Abajo     → slot siguiente en el anillo actual (horario)
//   Enter     → re-pivotear: el nodo azul se convierte en el nuevo pivote
//               y se llama a dep_view_init con ese id.
//
// La navegación geográfica normal (sin Ctrl) sigue activa en paralelo.
// ─────────────────────────────────────────────────────────────────────────────

#include "app.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// ── DepPivotRing ──────────────────────────────────────────────────────────────
// Un anillo lógico alrededor del pivote.

struct DepPivotRing {
    int                      ring_idx;   // negativo = upstream, positivo = downstream
    std::vector<std::string> slots;      // ids de nodos, orden estable (alphabético)
};

// ── DepPivotState ─────────────────────────────────────────────────────────────

struct DepPivotState {
    bool        active     = false;   // true mientras Ctrl está presionado
    std::string pivot_id;             // id del nodo pivote rojo (cursor rojo)
    int         cur_ring   = 1;       // anillo actualmente seleccionado (!=0)
    int         cur_slot   = 0;       // posición dentro del anillo actual

    // Anillos calculados a partir del pivote (excluye anillo 0 = el propio pivote)
    std::vector<DepPivotRing> rings;  // ordenados por ring_idx

    // Indica si hay datos calculados válidos para pivot_id actual
    bool        rings_valid = false;
};

extern DepPivotState g_dep_pivot;

// ── API ───────────────────────────────────────────────────────────────────────

// Recalcula los anillos BFS desde pivot_id usando s_phys como conjunto visible.
// Llama cuando pivot_id cambia o cuando s_phys cambia (dep_view_init).
void dep_pivot_rebuild(const AppState& state);

// Devuelve el id del nodo actualmente apuntado por el cursor azul dentro del
// modo pivote (ring cur_ring, slot cur_slot). "" si no hay nada.
std::string dep_pivot_selected_id();

// Procesa las teclas de navegación con Ctrl mantenido.
// Devuelve true si alguna tecla fue consumida.
// Si Enter es consumido, llena `out_repivot_id` con el nuevo id a re-pivotar
// (caller debe llamar dep_view_init + dep_pivot_rebuild).
bool dep_pivot_handle_keys(bool& out_repivot);

// Activa el modo pivote estableciendo pivot_id = dep_sel_id actual.
// Si dep_sel_id está vacío usa s_focus_id.
void dep_pivot_activate(const AppState& state);

// Desactiva el modo pivote.
void dep_pivot_deactivate();
