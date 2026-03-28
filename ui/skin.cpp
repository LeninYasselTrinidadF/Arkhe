#include "skin.hpp"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

Skin g_skin;

// ── Helpers locales ───────────────────────────────────────────────────────────

static NinePatch load_np(const json& elem,
                         const std::string& graphics_root,
                         TextureCache& cache)
{
    NinePatch np;
    if (!elem.is_object()) return np;

    std::string img = elem.value("image", "");
    if (img.empty()) return np;

    // Intentar cargar desde cache (la clave es el nombre del archivo sin ruta)
    // La ruta completa se construye aquí para que TextureCache la almacene bien.
    std::string key  = img;   // ej: "button.png"
    std::string path = graphics_root + img;
    np.tex = cache.load_path(key, path);

    if (elem.contains("margins") && elem["margins"].is_array()
        && elem["margins"].size() == 4)
    {
        np.top    = elem["margins"][0].get<int>();
        np.right  = elem["margins"][1].get<int>();
        np.bottom = elem["margins"][2].get<int>();
        np.left   = elem["margins"][3].get<int>();
    }
    return np;
}

// ── Skin::load ────────────────────────────────────────────────────────────────

void Skin::load(const std::string& graphics_root, TextureCache& cache) {
    loaded = false;

    std::string json_path = graphics_root + "ui_skin.json";
    std::ifstream f(json_path);
    if (!f.is_open()) {
        TraceLog(LOG_WARNING, "Skin: no se encontro %s", json_path.c_str());
        return;
    }

    json data;
    try { data = json::parse(f); }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Skin: error al parsear ui_skin.json: %s", e.what());
        return;
    }

    if (!data.contains("elements") || !data["elements"].is_object()) {
        TraceLog(LOG_ERROR, "Skin: ui_skin.json no tiene 'elements'");
        return;
    }

    auto& elems = data["elements"];

    if (elems.contains("bubble"))     bubble     = load_np(elems["bubble"],     graphics_root, cache);
    if (elems.contains("button"))     button     = load_np(elems["button"],     graphics_root, cache);
    if (elems.contains("panel"))      panel      = load_np(elems["panel"],      graphics_root, cache);
    if (elems.contains("field"))      field      = load_np(elems["field"],      graphics_root, cache);
    if (elems.contains("toolbar_bg")) toolbar_bg = load_np(elems["toolbar_bg"], graphics_root, cache);
    if (elems.contains("card"))       card       = load_np(elems["card"],       graphics_root, cache);

    loaded = bubble.valid() || button.valid() || panel.valid()
          || field.valid()  || toolbar_bg.valid() || card.valid();

    TraceLog(LOG_INFO, "Skin cargada desde %s (loaded=%d)", json_path.c_str(), (int)loaded);
}

void Skin::unload(TextureCache& cache) {
    // Las texturas las gestiona TextureCache; aquí solo invalidamos los NinePatch
    bubble = button = panel = field = toolbar_bg = card = NinePatch{};
    loaded = false;
}

void reload_skin(const std::string& graphics_root, TextureCache& cache) {
    g_skin.unload(cache);
    g_skin.load(graphics_root, cache);
}
