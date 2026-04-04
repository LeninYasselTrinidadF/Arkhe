#include "data/editor/equation_cache.hpp"

#include <fstream>
#include <filesystem>
#include <functional>
#include <sstream>
#include <iomanip>
#include <thread>
#include <cstdlib>

namespace fs = std::filesystem;

EquationCache g_eq_cache;

// ─────────────────────────────────────────────────────────────────────────────
// Destructor
// ─────────────────────────────────────────────────────────────────────────────

EquationCache::~EquationCache() { clear(); }

// ─────────────────────────────────────────────────────────────────────────────
// Helpers privados
// ─────────────────────────────────────────────────────────────────────────────

std::string EquationCache::eq_hash(const std::string& src)
{
    std::size_t h = std::hash<std::string>{}(src);
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
}

std::string EquationCache::cache_dir(const AppState& state)
{
    std::string ep = state.toolbar.entries_path;
    if (!ep.empty() && ep.back() != '/' && ep.back() != '\\') ep += '/';
    return ep + ".eq_cache/";
}

// ─────────────────────────────────────────────────────────────────────────────
// compile_worker  (ejecutado en thread separado)
// ─────────────────────────────────────────────────────────────────────────────

void EquationCache::compile_worker(EqEntry*    entry,
                                    std::string src,
                                    std::string latex_exe,
                                    std::string pdftoppm_exe,
                                    std::string work_dir)
{
    // ── 1. Generar .tex standalone ────────────────────────────────────────────
    std::string tex_path    = work_dir + "eq.tex";
    std::string pdf_path    = work_dir + "eq.pdf";
    std::string png_prefix  = work_dir + "eq";

    {
        std::ofstream f(tex_path);
        if (!f.is_open()) { entry->failed = true; entry->done.store(true); return; }

        // standalone con preview recorta exactamente alrededor del contenido.
        f << "\\documentclass[preview,border=3pt]{standalone}\n"
          << "\\usepackage{amsmath,amssymb,amsthm}\n"
          << "\\begin{document}\n"
          << src << "\n"
          << "\\end{document}\n";
    }

    // ── 2. pdflatex ───────────────────────────────────────────────────────────
    #ifdef _WIN32
        const std::string null_dev = "> nul 2>&1";
        // En Windows, cmd.exe /c necesita que todo el comando vaya envuelto
        // en comillas exteriores cuando el ejecutable también va entre comillas.
        // Forma: cmd /c ""exe" args"
        std::string cmd_latex =
            "cmd /c \""
            "\"" + latex_exe + "\""
            " -interaction=nonstopmode"
            " -output-directory=\"" + work_dir + "\""
            " \"" + tex_path + "\""
            "\" " + null_dev;
    #else
        const std::string null_dev = "> /dev/null 2>&1";
        std::string cmd_latex =
            "\"" + latex_exe + "\""
            " -interaction=nonstopmode"
            " -output-directory=\"" + work_dir + "\""
            " \"" + tex_path + "\" " + null_dev;
    #endif

    if (std::system(cmd_latex.c_str()) != 0) {
        entry->failed = true;
        entry->done.store(true);
        return;
    }

    // ── 3. pdftoppm → PNG ────────────────────────────────────────────────────
    #ifdef _WIN32
        std::string cmd_ppm =
            "cmd /c \""
            "\"" + pdftoppm_exe + "\""
            " -r 150 -png"
            " \"" + pdf_path + "\""
            " \"" + png_prefix + "\""
            "\" " + null_dev;
    #else
        std::string cmd_ppm =
            "\"" + pdftoppm_exe + "\""
            " -r 150 -png"
            " \"" + pdf_path + "\""
            " \"" + png_prefix + "\" " + null_dev;
    #endif

    std::system(cmd_ppm.c_str());

    // pdftoppm nombra el PNG como prefix-1.png (< 10 pág) o prefix-01.png
    std::string png1  = png_prefix + "-1.png";
    std::string png01 = png_prefix + "-01.png";

    if      (fs::exists(png1))  entry->png_path = png1;
    else if (fs::exists(png01)) entry->png_path = png01;
    else    { entry->failed = true; }

    entry->done.store(true);
}

// ─────────────────────────────────────────────────────────────────────────────
// request
// ─────────────────────────────────────────────────────────────────────────────

void EquationCache::request(const std::string& src, const AppState& state)
{
    std::string key = eq_hash(src);
    if (cache_.count(key)) return;   // ya solicitada

    // Crear la entrada antes de lanzar el thread (evita condición de carrera)
    auto& entry_ptr = cache_[key];
    entry_ptr = std::make_unique<EqEntry>();
    EqEntry* raw = entry_ptr.get();

    std::string dir = cache_dir(state) + key + "/";
    try { fs::create_directories(dir); }
    catch (...) { raw->failed = true; raw->done.store(true); return; }

    // Lanzar worker y desligarlo (detach)
    std::thread(compile_worker,
                raw,
                src,
                std::string(state.toolbar.latex_path),
                std::string(state.toolbar.pdftoppm_path),
                dir
    ).detach();
}

// ─────────────────────────────────────────────────────────────────────────────
// poll  (main thread — cada frame)
// ─────────────────────────────────────────────────────────────────────────────

void EquationCache::poll()
{
    for (auto& [key, ep] : cache_) {
        EqEntry& e = *ep;
        if (!e.done.load())  continue;   // worker aún corriendo
        if (e.loaded)        continue;   // ya subida
        if (e.failed)        continue;   // falló

        if (e.png_path.empty()) { e.failed = true; continue; }

        e.tex    = LoadTexture(e.png_path.c_str());
        e.loaded = (e.tex.id != 0);
        if (!e.loaded) e.failed = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// get
// ─────────────────────────────────────────────────────────────────────────────

EqEntry* EquationCache::get(const std::string& src)
{
    auto it = cache_.find(eq_hash(src));
    return it == cache_.end() ? nullptr : it->second.get();
}

// ─────────────────────────────────────────────────────────────────────────────
// clear
// ─────────────────────────────────────────────────────────────────────────────

void EquationCache::clear()
{
    for (auto& [key, ep] : cache_)
        if (ep && ep->loaded) UnloadTexture(ep->tex);
    cache_.clear();
}
