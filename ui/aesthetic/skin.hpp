#pragma once
#include "ui/aesthetic/nine_patch.hpp"
#include "ui/aesthetic/texture_cache.hpp"
#include <string>

// ── Skin ──────────────────────────────────────────────────────────────────────
// Contiene los NinePatch para cada tipo de elemento de UI.
// Se carga desde assets/graphics/ui_skin.json.
//
// Convención de nombres en el JSON:
//   "bubble"     → burbuja (central e hijos)
//   "button"     → botones genéricos
//   "panel"      → fondos de paneles flotantes
//   "field"      → campos de texto
//   "toolbar_bg" → fondo del toolbar
//   "card"       → tarjetas de recursos/crossrefs
// ─────────────────────────────────────────────────────────────────────────────

struct Skin {
    NinePatch bubble;
    NinePatch button;
    NinePatch panel;
    NinePatch field;
    NinePatch toolbar_bg;
    NinePatch card;

    // true si la skin fue cargada correctamente (al menos una textura válida)
    bool loaded = false;

    // Carga todas las texturas y márgenes desde ui_skin.json.
    // graphics_root: carpeta donde están las imágenes (ej: "assets/graphics/").
    // Si el archivo no existe o falla, loaded = false y los NinePatch son inválidos
    // (las funciones np_draw_* simplemente no dibujan nada en ese caso).
    void load(const std::string& graphics_root, TextureCache& cache);

    void unload(TextureCache& cache);
};

extern Skin g_skin;

// Recarga la skin (llamar cuando cambia graphics_path).
void reload_skin(const std::string& graphics_root, TextureCache& cache);
