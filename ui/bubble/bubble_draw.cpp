#include "bubble_draw.hpp"
#include "core/theme.hpp"
#include "core/nine_patch.hpp"
#include "core/skin.hpp"
#include "core/font_manager.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

// ── bubble_color ──────────────────────────────────────────────────────────────

Color bubble_color(Color base, const BubbleStats& st) {
    if (st.connected) return base;
    constexpr float factor = 0.40f;
    return Color{
        (unsigned char)(base.r * factor),
        (unsigned char)(base.g * factor),
        (unsigned char)(base.b * factor),
        base.a
    };
}

// ── draw_progress_arc ─────────────────────────────────────────────────────────

void draw_progress_arc(float cx, float cy, float r,
    float progress, float thick, Color col)
{
    if (progress <= 0.001f) return;
    float angle_deg = progress * 360.0f;
    float outer = r + thick + 2.0f;
    float inner = r + 2.0f;
    int   segments = std::max(6, (int)(angle_deg / 4.0f));
    DrawRing({ cx, cy }, inner, outer, -90.0f, -90.0f + angle_deg, segments, col);
}

// ── proportional_radius ───────────────────────────────────────────────────────

float proportional_radius(int weight, int max_weight, float min_r, float max_r) {
    if (max_weight <= 1) return (min_r + max_r) * 0.5f;
    float t = std::sqrt((float)weight / (float)max_weight);
    return min_r + t * (max_r - min_r);
}

// ── Label helpers ─────────────────────────────────────────────────────────────

// Divide una cadena en palabras (split por espacios)
static std::vector<std::string> split_words(const std::string& s) {
    std::vector<std::string> words;
    std::istringstream ss(s);
    std::string w;
    while (ss >> w) words.push_back(w);
    return words;
}

// Genera una abreviatura de N letras mayúsculas (primeras letras de cada palabra)
// ignorando palabras función como "and", "of", "the", "a", "in", "for", etc.
static std::string make_abbrev(const std::vector<std::string>& words, int letters = 0) {
    static const std::unordered_set<std::string> skip = {
        "and","of","the","a","an","in","for","by","to","with","from","on","at"
    };
    std::string abbr;
    for (auto& w : words) {
        std::string lw = w;
        for (char& c : lw) c = (char)tolower((unsigned char)c);
        if (skip.count(lw)) continue;
        if (!w.empty()) abbr += (char)toupper((unsigned char)w[0]);
    }
    // Si pedimos más letras en la primera palabra significativa, las añadimos
    if (letters > 1) {
        // Reconstruir: primera palabra significativa con `letters` letras
        std::string result;
        bool first_done = false;
        for (auto& w : words) {
            std::string lw = w;
            for (char& c : lw) c = (char)tolower((unsigned char)c);
            if (skip.count(lw)) continue;
            if (!first_done) {
                // Tomar `letters` letras de la primera palabra sig.
                int take = std::min(letters, (int)w.size());
                result += w.substr(0, take);
                first_done = true;
            }
            else {
                if (!w.empty()) result += (char)toupper((unsigned char)w[0]);
            }
        }
        return result;
    }
    return abbr;
}

// Resuelve colisiones de abreviaturas en un conjunto de labels:
// Devuelve un mapa label→abreviatura única (incrementando letras de primera
// palabra hasta que no haya duplicados, máx. longitud de esa palabra).
static std::unordered_map<std::string, std::string>
resolve_abbrevs(const std::vector<std::string>& labels) {
    std::unordered_map<std::string, std::string> result;
    // Primero generamos abreviaturas base (1 letra por palabra significativa)
    std::unordered_map<std::string, std::vector<std::string>> by_abbr;
    for (auto& lbl : labels) {
        auto words = split_words(lbl);
        std::string abbr = make_abbrev(words, 1);
        by_abbr[abbr].push_back(lbl);
        result[lbl] = abbr;
    }
    // Para cada grupo con colisión, incrementar letras de la primera pal. sig.
    for (auto& [abbr, group] : by_abbr) {
        if (group.size() <= 1) continue;
        // Intentar con 2, 3, ... letras hasta que todos sean únicos
        for (int extra = 2; extra <= 8; ++extra) {
            std::unordered_map<std::string, std::vector<std::string>> check;
            for (auto& lbl : group) {
                auto words = split_words(lbl);
                std::string a = make_abbrev(words, extra);
                check[a].push_back(lbl);
            }
            // Resolver los que ya son únicos
            bool all_unique = true;
            for (auto& [a2, g2] : check) {
                if (g2.size() > 1) { all_unique = false; }
                else { result[g2[0]] = a2; }
            }
            if (all_unique) break;
        }
    }
    return result;
}

// ── Helpers internos de medida ────────────────────────────────────────────────

// Ancho usable dentro de un círculo de radio r (margen 14% por lado)
static inline float usable_width(float r) { return r * 2.0f * 0.72f; }

// Altura usable para texto dentro del círculo (chord a ~60% del radio)
static inline float usable_height(float r) { return r * 2.0f * 0.60f; }

// Word-wrap greedy: devuelve líneas que caben en max_w píxeles
static std::vector<std::string> word_wrap(const std::vector<std::string>& words,
    float max_w, int font_size)
{
    std::vector<std::string> lines;
    std::string cur;
    for (auto& w : words) {
        std::string test = cur.empty() ? w : cur + " " + w;
        if (MeasureTextF(test.c_str(), font_size) <= (int)max_w) {
            cur = test;
        }
        else {
            if (!cur.empty()) lines.push_back(cur);
            cur = w;
        }
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

// ¿Caben todas las líneas (ancho Y alto total) dentro del radio dado?
static bool lines_fit(const std::vector<std::string>& lines, float r,
    int font_size)
{
    float uw = usable_width(r);
    float uh = usable_height(r);
    int line_h = MeasureTextF("Ag", font_size);
    int line_gap = 2;
    int total_h = (int)lines.size() * line_h +
        ((int)lines.size() - 1) * line_gap;
    if ((float)total_h > uh) return false;
    for (auto& ln : lines)
        if (MeasureTextF(ln.c_str(), font_size) > (int)uw) return false;
    return true;
}

// ── fit_radius_for_label ──────────────────────────────────────────────────────
// Calcula el radio mínimo para que el label entre, hasta max_r.
// Devuelve base_r si ya entra, o el radio ajustado, o max_r si nunca entra.

float fit_radius_for_label(const std::string& label, float base_r,
    float max_r, int font_size)
{
    auto words = split_words(label);
    if (words.empty()) return base_r;

    // Paso fino: probar radios de base_r a max_r en incrementos de 1px
    for (float r = base_r; r <= max_r + 0.5f; r += 1.0f) {
        auto lines = word_wrap(words, usable_width(r), font_size);
        if (lines_fit(lines, r, font_size))
            return r;
    }
    return max_r;
}

// ── BubbleLabelLines: estructura para dibujo multilínea ──────────────────────
//
// Reglas (con el radio ya posiblemente expandido por fit_radius_for_label):
//
//  1 palabra   → siempre cabe (el radio fue expandido). Una línea.
//  2-3 palabras → word-wrap en hasta 3 líneas con el radio dado.
//  4-5 palabras → word-wrap; si las últimas 1-2 palabras no entran en la
//                 última línea que sí cabe, se añade "..." al final.
//  6+ palabras  → igual que 4-5, pero si ni con "..." queda legible
//                 → needs_abbrev = true.

BubbleLabelLines make_label_lines(const std::string& label, float radius,
    int font_size)
{
    BubbleLabelLines out;
    auto words = split_words(label);
    if (words.empty()) return out;

    int  n_words = (int)words.size();
    float uw = usable_width(radius);
    float uh = usable_height(radius);
    int  line_h = MeasureTextF("Ag", font_size);
    int  line_gap = 2;

    // ── Caso base: word-wrap greedy ──────────────────────────────────────────
    auto wrapped = word_wrap(words, uw, font_size);

    // Calcular cuántas líneas caben verticalmente
    int max_lines = std::max(1, (int)((uh + line_gap) / (line_h + line_gap)));

    // ── Todas las líneas caben → devolver tal cual ───────────────────────────
    if ((int)wrapped.size() <= max_lines && lines_fit(wrapped, radius, font_size)) {
        out.lines = wrapped;
        return out;
    }

    // ── No todas caben: intentar truncar con "..." ───────────────────────────
    // Solo aplicamos "..." si son 4-5 palabras (6+ van a abreviatura directa)
    if (n_words >= 4 && n_words <= 6) {
        // Tomar las líneas que sí caben y añadir "..." a la última
        std::vector<std::string> truncated;
        for (int i = 0; i < (int)wrapped.size() && i < max_lines; i++)
            truncated.push_back(wrapped[i]);

        if (!truncated.empty()) {
            // Intentar añadir "..." a la última línea sin pasarse del ancho
            std::string& last = truncated.back();
            std::string with_dots = last + "...";
            if (MeasureTextF(with_dots.c_str(), font_size) <= (int)uw) {
                last = with_dots;
            }
            else {
                // Quitar palabras de la última línea hasta que quepa con "..."
                // Reconstruir la última línea sin la última palabra + "..."
                auto last_words = split_words(last);
                while (last_words.size() > 1) {
                    last_words.pop_back();
                    std::string candidate;
                    for (size_t wi = 0; wi < last_words.size(); wi++) {
                        if (wi > 0) candidate += " ";
                        candidate += last_words[wi];
                    }
                    std::string with_d = candidate + "...";
                    if (MeasureTextF(with_d.c_str(), font_size) <= (int)uw) {
                        last = with_d;
                        break;
                    }
                }
                if (last_words.size() == 1) {
                    // Ni una sola palabra + "..." cabe → abreviatura
                    out.needs_abbrev = true;
                    return out;
                }
            }
            out.lines = truncated;
            return out;
        }
    }

    // ── 7+ palabras o no se pudo truncar → abreviatura ───────────────────────
    out.needs_abbrev = true;
    return out;
}

// ── short_label ───────────────────────────────────────────────────────────────
// Versión legacy para compatibilidad — ya no la usa draw_bubble_view
// pero puede quedar como fallback.

std::string short_label(const std::string& label, float radius) {
    auto result = make_label_lines(label, radius, 15);
    if (result.needs_abbrev) {
        // Generar abreviatura simple sin contexto de colisiones
        auto words = split_words(label);
        return make_abbrev(words, 1);
    }
    if (result.lines.size() == 1) return result.lines[0];
    // Unir con separador para la versión de una línea
    std::string out;
    for (size_t i = 0; i < result.lines.size(); ++i) {
        if (i > 0) out += ", ";
        out += result.lines[i];
    }
    return out;
}

// ── build_child_abbrev_map ────────────────────────────────────────────────────
// Llama a resolve_abbrevs sobre todos los labels de los hijos actuales.
// Así se resuelven colisiones una vez por frame.

std::unordered_map<std::string, std::string>
build_child_abbrev_map(const std::vector<std::string>& labels) {
    return resolve_abbrevs(labels);
}

// ── arc_color ─────────────────────────────────────────────────────────────────

Color arc_color(ViewMode mode, float progress) {
    const Theme& th = g_theme;
    if (progress >= 0.99f) return th.success;
    if (progress >= 0.5f)  return th.warning;
    return Color{ th.success.r, th.success.g, th.success.b, 160 };
}

// ── draw_bubble ───────────────────────────────────────────────────────────────

void draw_bubble(AppState& state, float cx, float cy, float radius,
    Color col, const std::string& tex_key)
{
    if (g_skin.bubble.valid())
        np_draw_bubble(cx, cy, radius, col);
    else
        DrawCircleV({ cx, cy }, radius, col);

    if (!tex_key.empty()) {
        Texture2D tex = state.textures.get(tex_key);
        if (tex.id > 0) {
            float size = radius * 1.4f;
            float scale = size / (float)std::max(tex.width, tex.height);
            float dw = tex.width * scale, dh = tex.height * scale;
            DrawTexturePro(tex,
                { 0, 0, (float)tex.width, (float)tex.height },
                { cx - dw * 0.5f, cy - dh * 0.5f, dw, dh },
                { 0, 0 }, 0.0f, { 255, 255, 255, 200 });
        }
    }
}