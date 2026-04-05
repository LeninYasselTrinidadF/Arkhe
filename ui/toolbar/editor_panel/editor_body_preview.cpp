// ── editor_body_preview.cpp ───────────────────────────────────────────────────
// Incluido por editor_body.cpp — NO compilar directamente.
// Implementa: draw_body_preview
// ─────────────────────────────────────────────────────────────────────────────

namespace editor_sections {

void draw_body_preview(MathNode* /*sel*/, EditState& edit,
    AppState& state,
    int lx, int lw,
    int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll)
{
    auto segs = parse_body(edit.body_buf);
    for (auto& seg : segs)
        if (seg.type != BodySegment::Type::Text)
            g_eq_cache.request(seg.content, state);
    g_eq_cache.poll();

    const int area_h = std::max(60, py + ph - y - 10);
    Rectangle area_r = { (float)lx, (float)y, (float)lw, (float)area_h };

    DrawRectangleRec(area_r, { 14, 14, 24, 255 });
    DrawRectangleLinesEx(area_r, 1.0f, { 45, 45, 72, 200 });

    if (CheckCollisionPointRec(mouse, area_r)) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) body_scroll -= wh * 17.f * 3.f;
        if (body_scroll < 0) body_scroll = 0;
    }

    BeginScissorMode(lx + 1, (int)area_r.y + 1, lw - 2, area_h - 2);

    constexpr int FONT_SZ      = 11;
    constexpr int LINE_H       = FONT_SZ + 3;
    constexpr int PAD_X        = 6;
    constexpr int MAX_INLINE_H = 28;

    int draw_y  = (int)area_r.y + 4 - (int)body_scroll;
    int total_h = 0;

    for (auto& seg : segs) {
        if (seg.type == BodySegment::Type::Text) {
            const std::string& txt = seg.content;
            size_t start = 0;
            while (start <= txt.size()) {
                size_t nl = txt.find('\n', start);
                if (nl == std::string::npos) nl = txt.size();
                std::string line = txt.substr(start, nl - start);
                if (draw_y + LINE_H > (int)area_r.y &&
                    draw_y < (int)(area_r.y + area_h))
                {
                    DrawTextF(line.c_str(), lx + PAD_X, draw_y,
                        FONT_SZ, { 200, 210, 220, 210 });
                }
                draw_y  += LINE_H;
                total_h += LINE_H;
                start    = nl + 1;
            }
        } else {
            EqEntry* entry     = g_eq_cache.get(seg.content);
            bool     is_disp   = (seg.type == BodySegment::Type::DisplayMath);

            if (entry && entry->loaded) {
                float scale;
                if (is_disp)
                    scale = std::min(1.0f, (float)(lw - 2*PAD_X) / entry->tex.width);
                else {
                    scale = std::min(1.0f, (float)MAX_INLINE_H / entry->tex.height);
                    scale = std::min(scale, (float)(lw - 2*PAD_X) / entry->tex.width);
                }
                int tw = (int)(entry->tex.width  * scale);
                int th = (int)(entry->tex.height * scale);
                int tx = is_disp ? lx + (lw - tw) / 2 : lx + PAD_X;
                if (draw_y + th > (int)area_r.y && draw_y < (int)(area_r.y + area_h)) {
                    DrawRectangle(tx - 2, draw_y - 2, tw + 4, th + 4, WHITE);
                    DrawTextureEx(entry->tex,
                        { (float)tx, (float)draw_y }, 0.0f, scale, WHITE);
                }
                draw_y  += th + 6;
                total_h += th + 6;
            } else if (entry && entry->failed) {
                if (draw_y + 20 > (int)area_r.y && draw_y < (int)(area_r.y + area_h)) {
                    DrawRectangle(lx + PAD_X, draw_y, lw - 2*PAD_X, 20, { 50,10,10,220 });
                    DrawTextF("! Error LaTeX",
                        lx + PAD_X + 4, draw_y + 4, 10, { 255,100,100,240 });
                }
                draw_y  += 24;
                total_h += 24;
            } else {
                if (draw_y + 20 > (int)area_r.y && draw_y < (int)(area_r.y + area_h)) {
                    DrawRectangle(lx + PAD_X, draw_y, lw - 2*PAD_X, 20, { 20,20,40,180 });
                    DrawTextF("Compilando...",
                        lx + PAD_X + 4, draw_y + 4, 10, { 100,130,200,200 });
                }
                draw_y  += 24;
                total_h += 24;
            }
        }
    }

    EndScissorMode();

    float max_scr = (float)std::max(0, total_h - area_h + 8);
    if (body_scroll > max_scr) body_scroll = max_scr;

    y += area_h + 6;
}

} // namespace editor_sections
