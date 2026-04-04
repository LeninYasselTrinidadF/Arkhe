#include "ui/aesthetic/font_manager.hpp"
#include "raylib.h"
#include <algorithm>

FontManager g_fonts;

void FontManager::load(const std::string& path, int atlas_size) {
    // Intentar cargar con el atlas_size pedido
    // codepoints: ASCII 32-126 + Latin Extended 160-255 + algunos símbolos math
    int cp_count = 0;
    // Generamos el rango manualmente para no depender de malloc en el header
    static int codepoints[512];
    for (int i = 32;  i <= 126; i++) codepoints[cp_count++] = i;
    for (int i = 160; i <= 255; i++) codepoints[cp_count++] = i;
    // Flechas y operadores math básicos
    int extras[] = { 0x2190,0x2191,0x2192,0x2193,0x21D2,0x21D4,
                     0x2200,0x2203,0x2208,0x2209,0x2212,0x2217,
                     0x2227,0x2228,0x2229,0x222A,0x2260,0x2264,0x2265 };
    for (int cp : extras) codepoints[cp_count++] = cp;

    Font f = LoadFontEx(path.c_str(), atlas_size, codepoints, cp_count);
    if (f.texture.id == 0) {
        TraceLog(LOG_WARNING, "FontManager: no se pudo cargar %s, usando default", path.c_str());
        loaded = false;
        return;
    }
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    font   = f;
    loaded = true;
    TraceLog(LOG_INFO, "FontManager: fuente cargada %s (atlas %dpx)", path.c_str(), atlas_size);
}

void FontManager::unload() {
    if (loaded) {
        UnloadFont(font);
        font   = {};
        loaded = false;
    }
}

void FontManager::draw(const char* text, int x, int y, int size, Color color) const {
    int s = scale(size);
    if (!loaded) {
        DrawText(text, x, y, s, color);
        return;
    }
    // spacing proporcional al tamaño
    float spacing = s * 0.06f;
    DrawTextEx(font, text, {(float)x, (float)y}, (float)s, spacing, color);
}

int FontManager::measure(const char* text, int size) const {
    int s = scale(size);
    if (!loaded) return MeasureText(text, s);
    float spacing = s * 0.06f;
    return (int)MeasureTextEx(font, text, (float)s, spacing).x;
}
