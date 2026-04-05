#include "ui/views/dep/dep_draw.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "raymath.h"
#include <cmath>
#include <algorithm>

// ── dep_safe_trunc ────────────────────────────────────────────────────────────

std::string dep_safe_trunc(const std::string& s, int max_chars) {
    if ((int)s.size() <= max_chars) return s;
    int cut = max_chars - 1;
    while (cut > 4 && s[cut] != ' ') cut--;
    return s.substr(0, std::max(cut, max_chars - 1)) + "\xE2\x80\xA6"; // U+2026
}

// ── dep_get_role ──────────────────────────────────────────────────────────────

NodeRole dep_get_role(const AppState& state, const std::string& id) {
    if (id == s_focus_id) return NodeRole::Focus;
    const DepGraph& g = get_dep_graph_for_const(state);
    const DepNode* focus = g.get(s_focus_id);
    if (focus) {
        for (auto& d : focus->depends_on)
            if (d == id) return NodeRole::Upstream;
    }
    for (auto& d : g.get_dependents(s_focus_id))
        if (d == id) return NodeRole::Downstream;
    return NodeRole::Other;
}

// ── dep_draw_edge ─────────────────────────────────────────────────────────────

void dep_draw_edge(Vector2 from, Vector2 to,
    float w_from, float h_from,
    float w_to, float h_to,
    Color col)
{
    Vector2 dir = Vector2Normalize(Vector2Subtract(to, from));
    if (Vector2Length(dir) < 0.001f) return;

    Vector2 src = Vector2Add(from, Vector2Scale(dir, w_from * 0.5f + 2.f));
    Vector2 dst = Vector2Subtract(to, Vector2Scale(dir, w_to * 0.5f + 6.f));

    Vector2 perp = { -dir.y, dir.x };
    float   midlen = Vector2Length(Vector2Subtract(dst, src)) * 0.25f;
    Vector2 ctrl1 = Vector2Add(src, Vector2Add(
        Vector2Scale(dir, midlen), Vector2Scale(perp, midlen * 0.3f)));
    Vector2 ctrl2 = Vector2Subtract(dst, Vector2Add(
        Vector2Scale(dir, midlen), Vector2Scale(perp, midlen * 0.3f)));

    const int SEG = 20;
    Vector2 prev = src;
    for (int k = 1; k <= SEG; k++) {
        float   t = (float)k / SEG;
        float   t2 = t * t, t3 = t2 * t;
        float   u = 1.f - t, u2 = u * u, u3 = u2 * u;
        Vector2 pt = {
            u3 * src.x + 3 * u2 * t * ctrl1.x + 3 * u * t2 * ctrl2.x + t3 * dst.x,
            u3 * src.y + 3 * u2 * t * ctrl1.y + 3 * u * t2 * ctrl2.y + t3 * dst.y
        };
        DrawLineEx(prev, pt, 1.6f, col);
        prev = pt;
    }

    Vector2 adir = Vector2Normalize(Vector2Subtract(dst, ctrl2));
    Vector2 aperp = { -adir.y * 6.f, adir.x * 6.f };
    Vector2 tip = Vector2Add(dst, Vector2Scale(adir, 4.f));
    Vector2 base_l = Vector2Add(Vector2Subtract(tip, Vector2Scale(adir, 12.f)), aperp);
    Vector2 base_r = Vector2Subtract(Vector2Subtract(tip, Vector2Scale(adir, 12.f)), aperp);

    DrawTriangleLines(tip, base_l, base_r, col);
    DrawLineEx(tip, Vector2Lerp(base_l, base_r, 0.5f), 2.f, col);
    DrawLineEx(base_l, base_r, 1.5f, col);
}

// ── dep_draw_node ─────────────────────────────────────────────────────────────
// Tamaños de fuente y truncación aumentados para aprovechar los nodos más anchos.
// El nodo foco se dibuja 14px más ancho y 8px más alto que el resto para
// destacarlo visualmente sin cambiar su posición física.

static constexpr int   LABEL_FONT = 15;
static constexpr int   CODE_FONT = 11;
static constexpr float FOCUS_PAD_W = 14.f;
static constexpr float FOCUS_PAD_H = 8.f;
static constexpr float CHAR_W_LABEL = 8.5f;  // px/char estimado para font 15
static constexpr float CHAR_W_CODE = 7.0f;  // px/char estimado para font 11
static constexpr float PAD_X = 28.f;  // padding horizontal total del nodo

void dep_draw_node(const std::string& id, const NodePhys& p,
    NodeRole role, bool hov,
    const Theme& th, const AppState& state)
{
    bool is_focus = (role == NodeRole::Focus);

    // El nodo foco se pinta levemente más grande para que destaque
    float pw = is_focus ? p.w + FOCUS_PAD_W : p.w;
    float ph = is_focus ? p.h + FOCUS_PAD_H : p.h;

    Rectangle rect = { p.x - pw * 0.5f, p.y - ph * 0.5f, pw, ph };
    float rnd = 7.f;

    Color bg, border, text_c;
    switch (role) {
    case NodeRole::Focus:
        bg = th.accent;
        border = th.success;
        text_c = th.bg_app;
        break;
    case NodeRole::Upstream:
        bg = hov ? th.ctrl_bg_hover : ColorAlpha(th.success, 0.18f);
        border = th.success;
        text_c = th.ctrl_text;
        break;
    case NodeRole::Downstream:
        bg = hov ? th.ctrl_bg_hover : ColorAlpha(th.accent, 0.18f);
        border = th.accent;
        text_c = th.ctrl_text;
        break;
    default:
        bg = hov ? th.ctrl_bg_hover : ColorAlpha(th.ctrl_bg, 0.85f);
        border = hov ? th.bubble_hover_ring : th.ctrl_border;
        text_c = th.ctrl_text;
        break;
    }

    // Sombra
    DrawRectangleRounded(
        { rect.x + 3.f, rect.y + 3.f, rect.width, rect.height },
        rnd / rect.height, 6, ColorAlpha(BLACK, 0.40f));
    // Fondo
    DrawRectangleRounded(rect, rnd / rect.height, 6, bg);
    // Borde
    float bthick = is_focus ? 2.5f : 1.4f;
    DrawRectangleRoundedLinesEx(rect, rnd / rect.height, 6, bthick, border);
    // Anillo hover
    if (hov && !is_focus) {
        DrawRectangleRoundedLinesEx(
            { rect.x - 3.f, rect.y - 3.f, rect.width + 6.f, rect.height + 6.f },
            (rnd + 3.f) / (rect.height + 6.f), 6, 1.f,
            ColorAlpha(th.bubble_hover_ring, 0.5f));
    }

    // Calcular cuántos chars caben en el ancho real del nodo
    float usable_w = pw - PAD_X;
    int max_label_chars = std::max(6, (int)(usable_w / CHAR_W_LABEL));
    int max_code_chars = std::max(6, (int)(usable_w / CHAR_W_CODE));

    // Label (línea superior, centrada)
    const DepNode* dn = get_dep_graph_for_const(state).get(id);
    std::string    lbl = dn
        ? dep_safe_trunc(dn->label, max_label_chars)
        : dep_safe_trunc(id, max_label_chars);
    int tw = MeasureTextF(lbl.c_str(), LABEL_FONT);
    DrawTextF(lbl.c_str(),
        (int)(p.x - tw * 0.5f),
        (int)(p.y - ph * 0.5f + 10.f),
        LABEL_FONT, text_c);

    // Sub-label: código (línea inferior, centrada, semitransparente)
    std::string code_lbl = dep_safe_trunc(id, max_code_chars);
    int cw = MeasureTextF(code_lbl.c_str(), CODE_FONT);
    DrawTextF(code_lbl.c_str(),
        (int)(p.x - cw * 0.5f),
        (int)(p.y + ph * 0.5f - CODE_FONT - 8.f),
        CODE_FONT, ColorAlpha(text_c, 0.55f));
}

// ── dep_draw_pivot_node ───────────────────────────────────────────────────────

void dep_draw_pivot_node(const NodePhys& p, const Theme& th) {
    float t = (float)GetTime();
    float pulse = 1.f + 0.06f * sinf(t * 3.5f);
    float pad = 8.f * pulse;
    float rnd = 7.f;

    Color pivot_col = { 220, 50,  50,  230 };
    Color pivot_col_outer = { 220, 50,  50,  100 };

    // Usar el tamaño expandido del foco para que el anillo lo rodee correctamente
    float pw = p.w + FOCUS_PAD_W;
    float ph = p.h + FOCUS_PAD_H;

    Rectangle inner = {
        p.x - pw * 0.5f - pad,
        p.y - ph * 0.5f - pad,
        pw + pad * 2.f,
        ph + pad * 2.f
    };
    Rectangle outer = {
        inner.x - 3.f, inner.y - 3.f,
        inner.width + 6.f, inner.height + 6.f
    };

    float corner_i = rnd / std::max(inner.height, 1.f);
    float corner_o = rnd / std::max(outer.height, 1.f);

    DrawRectangleRoundedLinesEx(inner, corner_i, 6, 2.2f, pivot_col);
    DrawRectangleRoundedLinesEx(outer, corner_o, 6, 1.f, pivot_col_outer);

    const char* lbl = "[ PIVOTE ]";
    int lw = MeasureTextF(lbl, 10);
    DrawTextF(lbl, (int)(p.x - lw * 0.5f),
        (int)(p.y - ph * 0.5f - pad - 14.f), 10, pivot_col);
}

// ── dep_draw_pivot_selector ───────────────────────────────────────────────────

void dep_draw_pivot_selector(const NodePhys& p, const Theme& th) {
    float t = (float)GetTime();
    float pulse = 1.f + 0.05f * sinf(t * 4.f);
    float pad = 6.f * pulse;
    float rnd = 7.f;

    Color sel_col = { 180, 80, 200, 230 };
    Color sel_col_outer = { 180, 80, 200, 100 };

    Rectangle inner = {
        p.x - p.w * 0.5f - pad,
        p.y - p.h * 0.5f - pad,
        p.w + pad * 2.f,
        p.h + pad * 2.f
    };
    Rectangle outer = {
        inner.x - 2.f, inner.y - 2.f,
        inner.width + 4.f, inner.height + 4.f
    };

    float corner_i = rnd / std::max(inner.height, 1.f);
    float corner_o = rnd / std::max(outer.height, 1.f);

    DrawRectangleRoundedLinesEx(inner, corner_i, 6, 2.f, sel_col);
    DrawRectangleRoundedLinesEx(outer, corner_o, 6, 1.f, sel_col_outer);

    const char* hint = "[ Enter: re-pivotar ]";
    int hw = MeasureTextF(hint, 9);
    DrawTextF(hint, (int)(p.x - hw * 0.5f),
        (int)(p.y + p.h * 0.5f + pad + 2.f), 9, ColorAlpha(sel_col, 0.85f));
}