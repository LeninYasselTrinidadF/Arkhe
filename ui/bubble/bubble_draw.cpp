#include "bubble_draw.hpp"
#include "core/theme.hpp"
#include "core/nine_patch.hpp"
#include "core/skin.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>

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
    float outer     = r + thick + 2.0f;
    float inner     = r + 2.0f;
    int   segments  = std::max(6, (int)(angle_deg / 4.0f));
    DrawRing({cx, cy}, inner, outer, -90.0f, -90.0f + angle_deg, segments, col);
}

// ── proportional_radius ───────────────────────────────────────────────────────

float proportional_radius(int weight, int max_weight, float min_r, float max_r) {
    if (max_weight <= 1) return (min_r + max_r) * 0.5f;
    float t = std::sqrt((float)weight / (float)max_weight);
    return min_r + t * (max_r - min_r);
}

// ── short_label ───────────────────────────────────────────────────────────────

std::string short_label(const std::string& label, float radius) {
    int mc = (int)(radius * 1.6f / 4.8f);
    if (mc < 1) mc = 1;
    if ((int)label.size() <= mc) return label;
    return label.substr(0, mc - 1) + ".";
}

// ── arc_color ─────────────────────────────────────────────────────────────────

Color arc_color(ViewMode mode, float progress) {
    const Theme& th = g_theme;
    if (progress >= 0.99f) return th.success;
    if (progress >= 0.5f)  return th.warning;
    return Color{th.success.r, th.success.g, th.success.b, 160};
}

// ── draw_bubble ───────────────────────────────────────────────────────────────

void draw_bubble(AppState& state, float cx, float cy, float radius,
                 Color col, const std::string& tex_key)
{
    if (g_skin.bubble.valid())
        np_draw_bubble(cx, cy, radius, col);
    else
        DrawCircleV({cx, cy}, radius, col);

    if (!tex_key.empty()) {
        Texture2D tex = state.textures.get(tex_key);
        if (tex.id > 0) {
            float size  = radius * 1.4f;
            float scale = size / (float)std::max(tex.width, tex.height);
            float dw = tex.width * scale, dh = tex.height * scale;
            DrawTexturePro(tex,
                {0, 0, (float)tex.width, (float)tex.height},
                {cx - dw*0.5f, cy - dh*0.5f, dw, dh},
                {0, 0}, 0.0f, {255, 255, 255, 200});
        }
    }
}
