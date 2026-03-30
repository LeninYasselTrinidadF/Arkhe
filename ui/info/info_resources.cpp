#include "info_resources.hpp"
#include "../core/font_manager.hpp"
#include "../core/theme.hpp"
#include "../core/skin.hpp"
#include "../constants.hpp"

#include <algorithm>
#include <string>

// ── Sprite preview ────────────────────────────────────────────────────────────

void draw_sprite_preview(AppState& state, MathNode* node, int x, int y, int size) {
    if (!node || node->texture_key.empty()) return;
    Texture2D tex = state.textures.get(node->texture_key);
    if (tex.id == 0) return;

    const Theme& th = g_theme;
    float scale = (float)size / std::max(tex.width, tex.height);
    int dw = (int)(tex.width  * scale);
    int dh = (int)(tex.height * scale);

    DrawRectangle(x, y, size, size, th.bg_panel);
    DrawRectangleLinesEx({ (float)x, (float)y, (float)size, (float)size },
                         1.0f, th_alpha(th.border));
    DrawTexturePro(tex,
        { 0, 0, (float)tex.width, (float)tex.height },
        { (float)(x + (size - dw) / 2), (float)(y + (size - dh) / 2),
          (float)dw, (float)dh },
        { 0, 0 }, 0.0f, WHITE);
}

// ── Style por tipo de recurso ─────────────────────────────────────────────────

struct CardStyle { const char* tag; Color kc; };

static CardStyle resource_style(const std::string& kind) {
    if (kind == "book")        return { "LIBRO", { 200, 140,  40, 255 } };
    if (kind == "link")        return { "WEB",   {  60, 150, 220, 255 } };
    if (kind == "latex")       return { "LEAN",  {  80, 200, 120, 255 } };
    if (kind == "explanation") return { "NOTA",  { 140, 140, 200, 255 } };
    return { "RES", { 80, 80, 130, 255 } };
}

// ── draw_resources_block ──────────────────────────────────────────────────────

int draw_resources_block(AppState& state, MathNode* sel,
                         int col, int y,
                         int card_w, int card_h,
                         Vector2 mouse)
{
    const Theme& th = g_theme;
    DrawTextF("RECURSOS", col, y, 11, th_alpha(th.text_dim));
    y += 18;

    const std::vector<Resource>* res = sel ? &sel->resources : nullptr;
    bool has = res && !res->empty();

    if (!has) {
        // Placeholders
        struct Ph {
            const char* tag, *title, *desc;
            Color kc;
        };
        Ph ph[] = {
            { "LIBRO", "Introduction",   "Texto introductorio", { 200, 140,  40, 255 } },
            { "WEB",   "MathSciNet",     "mathscinet.ams.org",  {  60, 150, 220, 255 } },
            { "NOTA",  "Nota tecnica",   "Descripcion formal",  { 140, 140, 200, 255 } },
            { "WEB",   "zbMATH Open",    "zbmath.org",          {  60, 150, 220, 255 } },
            { "PAPER", "Survey article", "Estado del arte",     { 180, 100, 200, 255 } },
            { "LEAN",  "Def. formal",    "Formulacion Lean",    {  80, 200, 120, 255 } },
        };
        for (int i = 0; i < 6; i++) {
            int rx = col + (i % 3) * (card_w + 10);
            int ry = y   + (i / 3) * (card_h + 10);

            if (g_skin.card.valid())
                g_skin.card.draw((float)rx, (float)ry,
                                 (float)card_w, (float)card_h,
                                 th.bg_card);
            else {
                DrawRectangle(rx, ry, card_w, card_h, th.bg_card);
                DrawRectangleLinesEx(
                    { (float)rx,(float)ry,(float)card_w,(float)card_h },
                    1, th_alpha(th.border));
            }

            int tw = MeasureTextF(ph[i].tag, 9);
            DrawRectangle(rx + card_w - tw - 12, ry + 6, tw + 10, 16,
                          { ph[i].kc.r, ph[i].kc.g, ph[i].kc.b, 40 });
            DrawTextF(ph[i].tag,
                     rx + card_w - tw - 7, ry + 8,
                     9, ph[i].kc);
            DrawTextF(ph[i].title,
                     rx + 10, ry + 10, 13, th_alpha(th.text_secondary));
            DrawTextF(ph[i].desc,
                     rx + 10, ry + 30, 11, th_alpha(th.text_dim));
        }
        y += 2 * (card_h + 10) + 10;

    } else {
        int total = (int)res->size();
        for (int i = 0; i < total; i++) {
            auto& r    = (*res)[i];
            auto style = resource_style(r.kind);
            int rx     = col + (i % 3) * (card_w + 10);
            int ry     = y   + (i / 3) * (card_h + 10);

            bool hov = (mouse.x > rx && mouse.x < rx + card_w &&
                        mouse.y > ry && mouse.y < ry + card_h);

            Color bg     = hov ? th.bg_card_hover : th.bg_card;
            Color border = hov ? style.kc : th_alpha(th.border);

            if (g_skin.card.valid()) {
                g_skin.card.draw((float)rx,(float)ry,
                                 (float)card_w,(float)card_h, bg);
                DrawRectangleLinesEx(
                    {(float)rx,(float)ry,(float)card_w,(float)card_h},
                    1.0f, border);
            } else {
                DrawRectangle(rx, ry, card_w, card_h, bg);
                DrawRectangleLinesEx(
                    {(float)rx,(float)ry,(float)card_w,(float)card_h},
                    1, border);
            }

            int tw = MeasureTextF(style.tag, 9);
            DrawRectangle(rx + card_w - tw - 12, ry + 6, tw + 10, 16,
                          { style.kc.r, style.kc.g, style.kc.b, 40 });
            DrawTextF(style.tag, rx + card_w - tw - 7, ry + 8,
                     9, style.kc);
            DrawTextF(r.title.c_str(),
                     rx + 10, ry + 10, 13, th.text_primary);
            DrawTextF(r.content.substr(0, 42).c_str(),
                     rx + 10, ry + 30, 11, th_alpha(th.text_secondary));

            if (r.kind == "link" && hov &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                OpenURL(r.content.c_str());
        }
        y += ((total + 2) / 3) * (card_h + 10) + 10;
    }
    return y;
}
