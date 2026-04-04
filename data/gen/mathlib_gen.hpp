#pragma once
#include <string>
#include <atomic>

// ── MathlibGenOptions ─────────────────────────────────────────────────────────

struct MathlibGenOptions {
    std::string mathlib_path;   // ruta al repo mathlib4 (contiene "Mathlib/")
    std::string assets_path;    // carpeta de salida (assets/)
    int  max_decls = 200;      // máximo de declaraciones por archivo .lean
    int  min_decls = 0;        // mínimo para incluir un archivo en el layout
    bool force_full = false;    // true → regenera desde cero aunque el JSON exista
};

// ── MathlibGenJob ─────────────────────────────────────────────────────────────
// Estado compartido entre el hilo generador y el hilo de render (UI).

struct MathlibGenJob {
    std::atomic<bool> running{ false };
    std::atomic<bool> done{ false };

    bool        success = false;
    std::string status;           // mensaje corto para la UI
    int         folders = 0;
    int         files = 0;
    int         decls = 0;
    int         dep_edges = 0;

    // Solo en modo incremental
    int updated = 0;   // archivos actualizados
    int added = 0;   // archivos nuevos
    int removed = 0;   // archivos eliminados
    int skipped = 0;   // archivos sin cambios
};

// ── API ───────────────────────────────────────────────────────────────────────

// Genera/actualiza assets/mathlib_layout.json.
// Si el archivo ya existe y force_full==false → modo incremental
// (solo procesa .lean modificados desde la última generación).
void generate_mathlib_layout(const MathlibGenOptions& opts, MathlibGenJob& job);

// Genera/actualiza assets/deps_mathlib.json.
// Mismo comportamiento incremental que generate_mathlib_layout.
void generate_mathlib_deps(const MathlibGenOptions& opts, MathlibGenJob& job);