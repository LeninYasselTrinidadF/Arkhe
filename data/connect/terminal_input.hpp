#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// data/terminal_input.hpp
//
// Lectura no-bloqueante de stdin en un hilo separado.
// El hilo escribe líneas en una cola protegida por mutex.
// El main thread llama terminal_input_poll() cada frame para procesar comandos
// sin bloquear el render loop de raylib.
//
// Comandos disponibles (case-insensitive):
//
//   pwd              muestra modo + nodo actual + .tex asociado
//   ls [n]           lista hijos del nodo actual (hasta n, default 20)
//   cd <nombre|idx>  navega a hijo por nombre parcial o índice (0-based)
//   back             sube un nivel (equivale a ESC)
//   open             escribe bridge + abre VS Code con el nodo actual
//   sync             lee arkhe_bridge.json y navega al nodo de VS Code
//   dep              activa/desactiva dep_view del nodo actual
//   find <query>     busca nodo en todo el árbol (BFS, primera coincidencia)
//   mode <m>         cambia modo: msc | mathlib | std
//   help             muestra esta ayuda
//
// ─────────────────────────────────────────────────────────────────────────────

#include "../app.hpp"

// ── Ciclo de vida ─────────────────────────────────────────────────────────────

// Arranca el hilo de lectura de stdin. Llamar una sola vez desde main().
// El hilo se detiene automáticamente cuando el proceso termina.
void terminal_input_init();

// Procesa todos los comandos pendientes en la cola.
// Debe llamarse cada frame desde el main thread (después de BeginDrawing o antes).
// Modifica AppState directamente (navegación, modo, etc.).
void terminal_input_poll(AppState& state);

// Imprime el prompt "arkhe> " en stdout para que el usuario sepa que puede escribir.
// Se llama internamente después de procesar cada comando, pero puede llamarse manualmente.
void terminal_print_prompt();
