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
    return s.substr(0, std::max(cut, max_chars - 1)) + "\xE2\x80\xA6"; // U+2026 UTF-8
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
    Vector2 ctrl1 = Vector2Add(src, Vector2Add(Vector2Scale(dir, midlen),
        Vector2Scale(perp, midlen * 0.3f)));
    Vector2 ctrl2 = Vector2Subtract(dst, Vector2Add(Vector2Scale(dir, midlen),
        Vector2Scale(perp, midlen * 0.3f)));
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

    // Punta de flecha — DrawTriangle requiere CCW en NDC (y-up OpenGL),
    // pero BeginMode2D usa y-down, invirtiendo el winding aparente.
    // Solución: DrawTriangleLines (sin winding) + relleno con líneas.
    Vector2 adir = Vector2Normalize(Vector2Subtract(dst, ctrl2));
    Vector2 aperp = { -adir.y * 6.f, adir.x * 6.f };
    Vector2 tip = Vector2Add(dst, Vector2Scale(adir, 4.f));
    Vector2 base_l = Vector2Add(Vector2Subtract(tip, Vector2Scale(adir, 12.f)), aperp);
    Vector2 base_r = Vector2Subtract(Vector2Subtract(tip, Vector2Scale(adir, 12.f)), aperp);

    DrawTriangleLines(tip, base_l, base_r, col);           // contorno, sin winding
    DrawLineEx(tip, Vector2Lerp(base_l, base_r, 0.5f), 2.f, col); // eje central = relleno
    DrawLineEx(base_l, base_r, 1.5f, col);                 // base del triángulo
}

// ── dep_draw_node ─────────────────────────────────────────────────────────────

void dep_draw_node(const std::string& id, const NodePhys& p,
    NodeRole role, bool hov,
    const Theme& th, const AppState& state)
{
    Rectangle rect = { p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h };
    float rnd = 6.f;

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
    DrawRectangleRounded({ rect.x + 3.f, rect.y + 3.f, rect.width, rect.height },
        rnd / rect.height, 6, ColorAlpha(BLACK, 0.40f));
    // Fondo
    DrawRectangleRounded(rect, rnd / rect.height, 6, bg);
    // Borde
    float bthick = (role == NodeRole::Focus) ? 2.2f : 1.2f;
    DrawRectangleRoundedLinesEx(rect, rnd / rect.height, 6, bthick, border);
    // Anillo hover
    if (hov && role != NodeRole::Focus) {
        DrawRectangleRoundedLinesEx(
            { rect.x - 3.f, rect.y - 3.f, rect.width + 6.f, rect.height + 6.f },
            (rnd + 3.f) / (rect.height + 6.f), 6, 1.f,
            ColorAlpha(th.bubble_hover_ring, 0.5f));
    }

    // Label
    const DepNode* dn = get_dep_graph_for_const(state).get(id);
    std::string    lbl = dn ? dep_safe_trunc(dn->label, 20) : dep_safe_trunc(id, 20);
    int tw = MeasureTextF(lbl.c_str(), 14);
    DrawTextF(lbl.c_str(), (int)(p.x - tw * 0.5f), (int)(p.y - 9), 14, text_c);

    // Sub-label: código
    int cw = MeasureTextF(id.c_str(), 10);
    DrawTextF(id.c_str(), (int)(p.x - cw * 0.5f), (int)(p.y + 6),
        10, ColorAlpha(text_c, 0.55f));
}