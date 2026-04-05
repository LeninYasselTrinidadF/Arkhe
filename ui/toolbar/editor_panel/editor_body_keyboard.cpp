// ── editor_body_keyboard.cpp ──────────────────────────────────────────────────
// Incluido por editor_body.cpp — NO compilar directamente.
// Implementa: handle_body_keyboard  (static helper de draw_body_section)
//
// Gestiona todo el input de teclado del textarea de cuerpo LaTeX:
//   · Ctrl+A/C/X/V  (selección y portapapeles)
//   · Flechas + Shift  (movimiento y selección)
//   · Home / End
//   · Backspace / Delete
//   · Enter y caracteres imprimibles
//   · Auto-scroll para mantener el cursor visible
// ─────────────────────────────────────────────────────────────────────────────

namespace editor_sections {

static void handle_body_keyboard(
    EditState& edit,
    int line_h, int PAD,
    float& body_scroll, float max_scroll, int area_h)
{
    static constexpr int BMAX = 8191;

    int  len   = (int)strlen(edit.body_buf);
    bool ctrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);

    g_body.cursor_pos = std::clamp(g_body.cursor_pos, 0, len);
    g_body.sel_from   = std::clamp(g_body.sel_from,   0, len);
    g_body.sel_to     = std::clamp(g_body.sel_to,     0, len);

    // ── Ctrl+A / C / X / V ───────────────────────────────────────────────────
    if (ctrl && IsKeyPressed(KEY_A)) {
        g_body.sel_from = 0; g_body.sel_to = len; g_body.cursor_pos = len;
    }
    if (ctrl && IsKeyPressed(KEY_C) && g_body.has_selection()) {
        std::string clip(edit.body_buf + g_body.sel_min(),
                         g_body.sel_max() - g_body.sel_min());
        g_tf_clipboard = clip;
        SetClipboardText(clip.c_str());
    }
    if (ctrl && IsKeyPressed(KEY_X) && g_body.has_selection()) {
        std::string clip(edit.body_buf + g_body.sel_min(),
                         g_body.sel_max() - g_body.sel_min());
        g_tf_clipboard = clip;
        SetClipboardText(clip.c_str());
        int np = g_body.sel_min();
        memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                len - g_body.sel_max() + 1);
        g_body.set_cursor(np);
        edit.body_dirty = true;
    }
    if (ctrl && IsKeyPressed(KEY_V)) {
        const char* so  = GetClipboardText();
        std::string clip = (so && so[0]) ? so : g_tf_clipboard;
        if (!clip.empty()) {
            if (g_body.has_selection()) {
                int np = g_body.sel_min();
                memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                        strlen(edit.body_buf) - g_body.sel_max() + 1);
                g_body.set_cursor(np);
            }
            int pos   = g_body.cursor_pos;
            int space = BMAX - (int)strlen(edit.body_buf);
            int ins   = std::min((int)clip.size(), space);
            memmove(edit.body_buf + pos + ins, edit.body_buf + pos,
                    strlen(edit.body_buf) - pos + 1);
            memcpy(edit.body_buf + pos, clip.c_str(), ins);
            g_body.set_cursor(pos + ins);
            edit.body_dirty = true;
        }
    }

    // ── Movimiento + selección ────────────────────────────────────────────────
    len = (int)strlen(edit.body_buf);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        int np = std::max(0, g_body.cursor_pos - 1);
        shift ? g_body.extend_sel(np) : g_body.set_cursor(np);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        int np = std::min(len, g_body.cursor_pos + 1);
        shift ? g_body.extend_sel(np) : g_body.set_cursor(np);
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        int pos     = g_body.cursor_pos;
        int prev_nl = pos - 1;
        while (prev_nl > 0 && edit.body_buf[prev_nl - 1] != '\n') prev_nl--;
        int np = std::max(0, prev_nl - 1);
        while (np > 0 && edit.body_buf[np - 1] != '\n') np--;
        np = std::min(np + (pos - prev_nl), prev_nl - 1 < 0 ? 0 : prev_nl - 1);
        np = std::max(0, np);
        shift ? g_body.extend_sel(np) : g_body.set_cursor(np);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        int pos     = g_body.cursor_pos;
        int next_nl = pos;
        while (next_nl < len && edit.body_buf[next_nl] != '\n') next_nl++;
        int np = std::min(len, next_nl + 1);
        shift ? g_body.extend_sel(np) : g_body.set_cursor(np);
    }
    if (IsKeyPressed(KEY_HOME)) {
        int pos = g_body.cursor_pos;
        while (pos > 0 && edit.body_buf[pos - 1] != '\n') pos--;
        shift ? g_body.extend_sel(pos) : g_body.set_cursor(pos);
    }
    if (IsKeyPressed(KEY_END)) {
        int pos = g_body.cursor_pos;
        while (pos < len && edit.body_buf[pos] != '\n') pos++;
        shift ? g_body.extend_sel(pos) : g_body.set_cursor(pos);
    }

    // ── Backspace / Delete ────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        len = (int)strlen(edit.body_buf);
        if (g_body.has_selection()) {
            int np = g_body.sel_min();
            memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                    len - g_body.sel_max() + 1);
            g_body.set_cursor(np);
        } else if (g_body.cursor_pos > 0) {
            int pos = g_body.cursor_pos - 1;
            memmove(edit.body_buf + pos, edit.body_buf + pos + 1, len - pos);
            g_body.set_cursor(pos);
        }
        edit.body_dirty = true;
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        len = (int)strlen(edit.body_buf);
        if (g_body.has_selection()) {
            int np = g_body.sel_min();
            memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                    len - g_body.sel_max() + 1);
            g_body.set_cursor(np);
        } else if (g_body.cursor_pos < len) {
            memmove(edit.body_buf + g_body.cursor_pos,
                    edit.body_buf + g_body.cursor_pos + 1,
                    len - g_body.cursor_pos);
            g_body.set_cursor(g_body.cursor_pos);
        }
        edit.body_dirty = true;
    }

    // ── Enter ─────────────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        len = (int)strlen(edit.body_buf);
        if (g_body.has_selection()) {
            int np = g_body.sel_min();
            memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                    len - g_body.sel_max() + 1);
            g_body.set_cursor(np);
            len = (int)strlen(edit.body_buf);
        }
        if (len < BMAX) {
            int pos = g_body.cursor_pos;
            memmove(edit.body_buf + pos + 1, edit.body_buf + pos, len - pos + 1);
            edit.body_buf[pos] = '\n';
            g_body.set_cursor(pos + 1);
            edit.body_dirty = true;
        }
    }

    // ── Caracteres imprimibles ────────────────────────────────────────────────
    if (!ctrl) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                len = (int)strlen(edit.body_buf);
                if (g_body.has_selection()) {
                    int np = g_body.sel_min();
                    memmove(edit.body_buf + np, edit.body_buf + g_body.sel_max(),
                            len - g_body.sel_max() + 1);
                    g_body.set_cursor(np);
                    len = (int)strlen(edit.body_buf);
                }
                if (len < BMAX) {
                    int pos = g_body.cursor_pos;
                    memmove(edit.body_buf + pos + 1, edit.body_buf + pos,
                            len - pos + 1);
                    edit.body_buf[pos] = (char)key;
                    g_body.set_cursor(pos + 1);
                    edit.body_dirty = true;
                }
            }
            key = GetCharPressed();
        }
    }

    // ── Auto-scroll: mantener cursor visible ──────────────────────────────────
    {
        int lines_before = 0;
        for (int i = 0; i < g_body.cursor_pos && i < (int)strlen(edit.body_buf); i++)
            if (edit.body_buf[i] == '\n') lines_before++;
        float cursor_y = (float)(lines_before * line_h + PAD);
        if (cursor_y - body_scroll > (float)(area_h - line_h))
            body_scroll = cursor_y - (float)(area_h - line_h);
        if (cursor_y - body_scroll < 0)
            body_scroll = cursor_y;
        body_scroll = std::clamp(body_scroll, 0.f, max_scroll);
    }
}

} // namespace editor_sections
