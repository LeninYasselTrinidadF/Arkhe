#pragma once
// ── TextureCache ──────────────────────────────────────────────────────────────
// Carga texturas desde assets/graphics/ bajo demanda y las almacena en cache.
// Formatos soportados: PNG, JPG, BMP, TGA, GIF (cualquiera que soporte raylib).
//
// Uso:
//   TextureCache cache;
//   cache.set_root("assets/graphics");          // llamar antes de usar
//   Texture2D tex = cache.get("algebra_icon");  // busca algebra_icon.*
//
// La ruta de busqueda es: <root>/<key>.png  (primero .png, luego .jpg, .bmp)
// Si no se encuentra el archivo devuelve una textura invalida (id==0).
// ─────────────────────────────────────────────────────────────────────────────
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class TextureCache {
public:
    // Extensiones buscadas en orden
    static inline const std::vector<std::string> EXTS = {
        ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif"
    };

    TextureCache() = default;
    ~TextureCache() { unload_all(); }

    // No copiable (las texturas tienen ownership de GPU)
    TextureCache(const TextureCache&)            = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    // ── Configuracion ─────────────────────────────────────────────────────────
    void set_root(const std::string& path) {
        root_ = path;
        // Limpiar cache al cambiar raiz
        unload_all();
    }
    const std::string& root() const { return root_; }

    // ── Acceso ────────────────────────────────────────────────────────────────
    // Devuelve la textura (puede ser invalida si no existe el archivo).
    // La primera llamada la carga; las siguientes usan cache.
    Texture2D get(const std::string& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        // Buscar archivo con alguna extension soportada
        for (auto& ext : EXTS) {
            std::string path = root_ + "/" + key + ext;
            if (fs::exists(path)) {
                Texture2D tex = LoadTexture(path.c_str());
                if (tex.id > 0) {
                    cache_[key] = tex;
                    return tex;
                }
            }
        }

        // No encontrado: guardar textura invalida para no buscar de nuevo
        cache_[key] = Texture2D{};
        return Texture2D{};
    }

    // Carga explicita por ruta completa (sin usar root_)
    Texture2D load_path(const std::string& key, const std::string& full_path) {
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        Texture2D tex = LoadTexture(full_path.c_str());
        cache_[key] = tex;
        return tex;
    }

    bool has(const std::string& key) const {
        auto it = cache_.find(key);
        return it != cache_.end() && it->second.id > 0;
    }

    // Lista todas las texturas validas cargadas actualmente
    std::vector<std::string> loaded_keys() const {
        std::vector<std::string> out;
        for (auto& [k, t] : cache_)
            if (t.id > 0) out.push_back(k);
        return out;
    }

    // Escanea root_ y precarga todas las imagenes disponibles
    void preload_all() {
        if (root_.empty() || !fs::exists(root_)) return;
        for (auto& entry : fs::directory_iterator(root_)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            // Normalizar extension a minusculas
            for (char& c : ext) c = (char)tolower((unsigned char)c);
            bool valid_ext = false;
            for (auto& e : EXTS) if (e == ext) { valid_ext = true; break; }
            if (!valid_ext) continue;
            std::string key = entry.path().stem().string();
            get(key); // carga y cachea
        }
    }

    void unload(const std::string& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (it->second.id > 0) UnloadTexture(it->second);
            cache_.erase(it);
        }
    }

    void unload_all() {
        for (auto& [k, t] : cache_)
            if (t.id > 0) UnloadTexture(t);
        cache_.clear();
    }

private:
    std::string root_ = "assets/graphics";
    std::unordered_map<std::string, Texture2D> cache_;
};
