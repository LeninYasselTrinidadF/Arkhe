#pragma once
// ── equation_cache.hpp ────────────────────────────────────────────────────────
// Caché de ecuaciones LaTeX compiladas a Texture2D via pdflatex + pdftoppm.
//
// Flujo por ecuación:
//   1. request(src, state) → si no está en caché, lanza un worker thread.
//   2. Worker: escribe .tex standalone → pdflatex → pdftoppm → escribe png_path
//              → done.store(true).
//   3. poll()  → main thread: si done && !loaded → LoadTexture(png_path).
//      ¡LoadTexture es OpenGL y DEBE llamarse desde el main thread!
//   4. get(src) → devuelve EqEntry* (nullptr si aún no se pidió).
//
// Notas:
//   · EqEntry vive en heap (unique_ptr) para que el thread pueda escribir
//     en él sin invalidación por rehash del mapa.
//   · Los threads se detachan: no esperar join en clear(). Si el programa
//     cierra mientras un thread corre, es aceptable (el .tex/png queda en
//     disco para la próxima ejecución).
//   · El directorio de caché es  entries_path + ".eq_cache/<hash>/".
// ─────────────────────────────────────────────────────────────────────────────

#include "../../app.hpp"
#include "raylib.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>

// ── EqEntry ───────────────────────────────────────────────────────────────────

struct EqEntry {
    // Escritos por el worker (antes de done = true):
    std::string         png_path;
    bool                failed  = false;

    // Señal de sincronización worker → main:
    std::atomic<bool>   done    { false };

    // Escritos por el main thread (en poll):
    Texture2D           tex     = {};
    bool                loaded  = false;

    EqEntry() = default;
    // No copiable ni movible (atomic + Texture2D)
    EqEntry(const EqEntry&)            = delete;
    EqEntry& operator=(const EqEntry&) = delete;
};

// ── EquationCache ─────────────────────────────────────────────────────────────

class EquationCache {
public:
    ~EquationCache();

    /// Solicita la compilación de `src` si aún no está en caché.
    /// src incluye delimitadores: "$...$" o "$$...$$".
    void request(const std::string& src, const AppState& state);

    /// Sube las texturas de los workers terminados al contexto OpenGL.
    /// Llamar una vez por frame desde el main thread.
    void poll();

    /// Devuelve nullptr si nunca se pidió la ecuación.
    EqEntry* get(const std::string& src);

    /// Libera todas las texturas cargadas.
    /// ⚠  Si hay workers activos, sus punteros siguen válidos (las entradas
    ///    no se destruyen hasta que el programa cierra). Seguro para single-thread Raylib.
    void clear();

private:
    // key = hash hex de src
    std::unordered_map<std::string, std::unique_ptr<EqEntry>> cache_;

    static std::string eq_hash  (const std::string& src);
    static std::string cache_dir(const AppState& state);

    // Función del worker (ejecutada en thread separado)
    static void compile_worker(EqEntry*    entry,
                                std::string src,
                                std::string latex_exe,
                                std::string pdftoppm_exe,
                                std::string work_dir);
};

// Instancia global — accesible desde editor_sections sin pasar parámetros extra.
extern EquationCache g_eq_cache;
