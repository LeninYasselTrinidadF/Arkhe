// ── editor_body_render.cpp ────────────────────────────────────────────────────
// Incluido por editor_body.cpp — NO compilar directamente.
// Implementa: render_body_edit  (static helper de draw_body_section)
//
// Dibuja el contenido del textarea con scissor:
//   · Highlight de selección
//   · Texto con word-wrap ya computado en vlines
//   · Cursor parpadeante
//   · Scrollbar vertical
// ─────────────────────────────────────────────────────────────────────────────

namespace editor_sections {

static void render_body_edit(
    const Rectangle& area_r,
    int PAD, int font, int line_h, int text_w,
    EditState& edit,
    const std::vector<std::string>& vlines,
    float body_scroll,
    int content_h, int area_h, float max_scroll,
    bool body_active)
{
    BeginScissorMode((int)area_r.x + 1, (int)area_r.y + 1,
        (int)area_r.width - 2, (int)area_r.height - 2);
    {
        // ── Mapa posición-en-buffer → (vline_idx, px_en_línea) ───────────────
        std::vector<int> vline_buf_start;
        {
            int         vl = 0;
            const char* p  = edit.body_buf;
            vline_buf_start.push_back(0);
            while (true) {
                const char* nl      = strchr(p, '\n');
                int         seg_len = nl ? (int)(nl - p) : (int)strlen(p);
                int         base    = (int)(p - edit.body_buf);
                std::string seg(p, seg_len);

                if (seg.empty()) {
                    if (nl) {
                        vl++;
                        if (vl < (int)vlines.size())
                            vline_buf_start.push_back(base + seg_len + 1);
                    }
                } else {
                    int start = 0;
                    while (start < (int)seg.size()) {
                        int chars = 1;
                        while (chars < (int)seg.size() - start &&
                            MeasureTextF(seg.substr(start, chars + 1).c_str(), font) <= text_w)
                            chars++;
                        if (chars < (int)seg.size() - start) {
                            int sp = (int)seg.rfind(' ', start + chars) - start;
                            if (sp > 0) chars = sp;
                        }
                        start += chars;
                        if (start < (int)seg.size() && seg[start] == ' ') start++;
                        vl++;
                        if (vl < (int)vlines.size())
                            vline_buf_start.push_back(
                                base + std::min(start, (int)seg.size()));
                    }
                    if (nl) {
                        if (vl < (int)vlines.size())
                            vline_buf_start.push_back(base + seg_len + 1);
                        vl++;
                    }
                }
                if (!nl) break;
                p = nl + 1;
            }
            while ((int)vline_buf_start.size() < (int)vlines.size())
                vline_buf_start.push_back((int)strlen(edit.body_buf));
        }

        // ── Convierte posición en buffer a (fila visual, px columna) ─────────
        auto pos_to_screen = [&](int pos) -> std::pair<int, int> {
            int vi = 0;
            for (int i = (int)vline_buf_start.size() - 1; i >= 0; i--) {
                if (pos >= vline_buf_start[i]) { vi = i; break; }
            }
            vi      = std::clamp(vi, 0, (int)vlines.size() - 1);
            int col = std::clamp(pos - vline_buf_start[vi], 0, (int)vlines[vi].size());
            int px  = MeasureTextF(vlines[vi].substr(0, col).c_str(), font);
            return { vi, px };
        };

        int draw_y_base = (int)area_r.y + PAD - (int)body_scroll;

        // ── Highlight de selección ────────────────────────────────────────────
        if (body_active && g_body.has_selection()) {
            auto [sv, spx] = pos_to_screen(g_body.sel_min());
            auto [ev, epx] = pos_to_screen(g_body.sel_max());
            for (int vi = sv; vi <= ev && vi < (int)vlines.size(); vi++) {
                int row_y = draw_y_base + vi * line_h;
                if (row_y + line_h <= (int)area_r.y ||
                    row_y >= (int)(area_r.y + area_h)) continue;
                int hx0 = (vi == sv) ? spx : 0;
                int hx1 = (vi == ev) ? epx
                                     : MeasureTextF(vlines[vi].c_str(), font) + 2;
                hx1 = std::max(hx1, hx0 + 2);
                DrawRectangle((int)area_r.x + PAD + hx0, row_y,
                    hx1 - hx0, line_h,
                    ColorAlpha(g_theme.accent, 0.30f));
            }
        }

        // ── Texto ─────────────────────────────────────────────────────────────
        int draw_y = draw_y_base;
        for (auto& line : vlines) {
            if (draw_y + line_h > (int)area_r.y &&
                draw_y < (int)area_r.y + area_h)
            {
                DrawTextF(line.c_str(), (int)area_r.x + PAD, draw_y,
                    font, { 200, 210, 220, 210 });
            }
            draw_y += line_h;
        }

        // ── Cursor parpadeante ────────────────────────────────────────────────
        if (body_active && !g_body.has_selection() &&
            (int)(GetTime() * 2) % 2 == 0)
        {
            auto [vi, cpx] = pos_to_screen(g_body.cursor_pos);
            int cur_y = draw_y_base + vi * line_h;
            if (cur_y + line_h > (int)area_r.y &&
                cur_y < (int)(area_r.y + area_h))
            {
                int cx = (int)area_r.x + PAD + cpx;
                DrawLine(cx, cur_y + 1, cx, cur_y + font + 1,
                    { 150, 180, 255, 255 });
            }
        }
    }
    EndScissorMode();

    // ── Scrollbar vertical ────────────────────────────────────────────────────
    if (content_h > area_h) {
        float ratio   = (float)area_h / (float)content_h;
        float thumb_h = std::max(16.f, ratio * (float)area_h);
        float thumb_y = (float)area_r.y
            + (max_scroll > 0.f ? body_scroll / max_scroll : 0.f)
            * ((float)area_h - thumb_h);
        DrawRectangle((int)(area_r.x + area_r.width) - 5,
            (int)area_r.y, 4, area_h, { 30, 30, 50, 180 });
        DrawRectangle((int)(area_r.x + area_r.width) - 5,
            (int)thumb_y, 4, (int)thumb_h, { 80, 110, 200, 200 });
    }
}

} // namespace editor_sections
