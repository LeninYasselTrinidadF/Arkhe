#pragma once
#include <string>
#include <atomic>

// ── MathlibGenOptions ─────────────────────────────────────────────────────────
// Parámetros para los generadores de assets Mathlib.

struct MathlibGenOptions {
    std::string mathlib_path;   // ruta al repo mathlib4 (contiene "Mathlib/")
    std::string assets_path;    // carpeta de salida (assets/)
    int max_decls = 200;        // máximo de declaraciones por archivo .lean
    int min_decls = 0;          // mínimo para incluir un archivo en el layout
};

// ── MathlibGenJob ─────────────────────────────────────────────────────────────
// Estado compartido entre el hilo generador y el hilo de render (UI).
// Todos los campos son seguros de leer desde el hilo principal cuando done==true.

struct MathlibGenJob {
    std::atomic<bool> running{ false };
    std::atomic<bool> done{ false };

    // Rellenado por el generador al terminar
    bool        success    = false;
    std::string status;          // mensaje corto para la UI
    int         folders    = 0;
    int         files      = 0;
    int         decls      = 0;
    int         dep_edges  = 0;
};

// ── API ───────────────────────────────────────────────────────────────────────

// Genera assets/mathlib_layout.json a partir del código fuente Mathlib4.
// Llamar desde un hilo separado; actualiza job.status/done al terminar.
void generate_mathlib_layout(const MathlibGenOptions& opts, MathlibGenJob& job);

// Genera assets/deps_mathlib.json a partir de las declaraciones import .lean.
// Llamar desde un hilo separado; actualiza job.status/done al terminar.
void generate_mathlib_deps(const MathlibGenOptions& opts, MathlibGenJob& job);
