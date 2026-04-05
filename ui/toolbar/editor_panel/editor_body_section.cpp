// ── editor_body_section.cpp ───────────────────────────────────────────────────
// Incluido por editor_body.cpp — NO compilar directamente.
// Implementa: draw_body_section
//
// «Guardar» en la barra de sección:
//   1. on_save_tex  → escribe el .tex al disco
//   2. on_save_json → actualiza el JSON completo del nodo
// ─────────────────────────────────────────────────────────────────────────────

namespace editor_sections {

void draw_body_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    float& body_scroll, bool& body_active, bool& show_file_manager,
    SaveFn on_save_json,
    void (*on_save_tex)(MathNode*, EditState&, AppState&, bool&))
{
    // ── Barra colapsable ──────────────────────────────────────────────────────
    char bar_title[128];
    if (!edit.tex_file.empty())
        snprintf(bar_title, sizeof(bar_title), "CUERPO LATEX  [%s]", edit.tex_file.c_str());
    else
        snprintf(bar_title, sizeof(bar_title), "CUERPO LATEX");

    bool save_clicked = false;
    if (!draw_section_bar(bar_title, edit.sec_body_open,
                          lx, lw, y, mouse,
                          edit.body_dirty, true, &save_clicked))
        return;

    // ── Botones secundarios: Preview | Importar ───────────────────────────────
    const int BTN_W = 56, BTN_H = 16;
    const float btn_y = (float)(y - 1);

    Rectangle prev_r = { (float)(lx + lw - 120), btn_y, (float)BTN_W, (float)BTN_H };
    Rectangle imp_r  = { (float)(lx + lw -  60), btn_y, (float)BTN_W, (float)BTN_H };

    int  prev_idx = kbnav_toolbar_register(ToolbarNavKind::Button, prev_r,
                        edit.preview_mode ? "Editar" : "Preview");
    int  imp_idx  = kbnav_toolbar_register(ToolbarNavKind::Button, imp_r, "Importar");
    bool prev_hov = CheckCollisionPointRec(mouse, prev_r);
    bool imp_hov  = CheckCollisionPointRec(mouse, imp_r);

    bool do_prev = (prev_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                 || kbnav_toolbar_is_activated(prev_idx);
    bool do_imp  = (imp_hov  && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                 || kbnav_toolbar_is_activated(imp_idx);

    RL prev_col = edit.preview_mode ? RL{ 50,90,140,255 } : RL{ 28,35,60,255 };
    DrawRectangleRec(prev_r,
        (prev_hov || kbnav_toolbar_is_focused(prev_idx)) ? RL{ 70,110,170,255 } : prev_col);
    DrawRectangleLinesEx(prev_r, 1.f, { 60,110,200,200 });
    DrawTextF(edit.preview_mode ? "Editar" : "Preview",
        (int)prev_r.x + 8, (int)prev_r.y + 2, 9, WHITE);

    DrawRectangleRec(imp_r,
        (imp_hov || kbnav_toolbar_is_focused(imp_idx)) ? RL{ 50,80,140,255 } : RL{ 28,35,60,255 });
    DrawRectangleLinesEx(imp_r, 1.f, { 60,100,200,200 });
    DrawTextF("Importar", (int)imp_r.x + 4, (int)imp_r.y + 2, 9, WHITE);

    if (do_prev) { edit.preview_mode = !edit.preview_mode; body_scroll = 0; }
    if (do_imp)  show_file_manager = !show_file_manager;

    // Guardar: .tex primero, luego JSON del nodo
    if (save_clicked) {
        if (on_save_tex)  on_save_tex(sel, edit, state, show_file_manager);
        if (on_save_json) on_save_json(sel, edit, state);
    }

    y += 20;

    // ── Modo preview ──────────────────────────────────────────────────────────
    if (edit.preview_mode) {
        draw_body_preview(sel, edit, state, lx, lw, py, ph, y, mouse, body_scroll);
        return;
    }

    // ── Modo edición ──────────────────────────────────────────────────────────

    const int font    = 11;
    const int line_h  = font + 3;
    const int PAD     = 5;
    const int text_w  = lw - 2 * PAD - 6;

    // Word-wrap
    auto wrap_logical = [&](const std::string& logical) -> std::vector<std::string>
    {
        std::vector<std::string> out;
        std::string s = logical;
        if (s.empty()) { out.push_back(""); return out; }
        while (!s.empty()) {
            int chars = 1;
            while (chars < (int)s.size() &&
                MeasureTextF(s.substr(0, chars + 1).c_str(), font) <= text_w)
                chars++;
            if (chars < (int)s.size()) {
                int sp = (int)s.rfind(' ', chars);
                if (sp > 0) chars = sp;
            }
            out.push_back(s.substr(0, chars));
            s = s.substr(chars);
            if (!s.empty() && s[0] == ' ') s = s.substr(1);
        }
        return out;
    };

    std::vector<std::string> vlines;
    {
        const char* p = edit.body_buf;
        while (true) {
            const char* nl      = strchr(p, '\n');
            int         seg_len = nl ? (int)(nl - p) : (int)strlen(p);
            for (auto& l : wrap_logical(std::string(p, seg_len)))
                vlines.push_back(l);
            if (!nl) break;
            p = nl + 1;
        }
    }
    if (vlines.empty()) vlines.push_back("");

    // Altura dinámica: mínimo 5 líneas, máximo 20 líneas
    const int min_body_h = line_h * 5 + 2 * PAD;
    const int max_body_h = std::min((py + ph) - y - 20, line_h * 20 + 2 * PAD);
    const int content_h  = (int)vlines.size() * line_h + 2 * PAD;
    const int area_h     = std::clamp(content_h, min_body_h, std::max(min_body_h, max_body_h));

    Rectangle area_r   = { (float)lx, (float)y, (float)lw, (float)area_h };
    bool      area_hov = CheckCollisionPointRec(mouse, area_r);

    // Registrar Textarea en kbnav
    {
        int body_idx = kbnav_toolbar_register(ToolbarNavKind::Textarea, area_r, "Cuerpo LaTeX");
        if (kbnav_toolbar_is_focused(body_idx) && !body_active) {
            body_active                = true;
            state.toolbar.active_field = -99;
            g_body.set_cursor((int)strlen(edit.body_buf));
        }
    }

    RL border = body_active ? RL{ 70,130,255,255 }
              : area_hov   ? RL{ 70, 70,120,255 }
                           : RL{ 45, 45, 72, 200 };
    DrawRectangleRec(area_r, { 18, 18, 32, 255 });
    DrawRectangleLinesEx(area_r, 1.f, border);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool was = body_active;
        body_active = area_hov;
        if (area_hov) {
            state.toolbar.active_field = -99;
            if (!was) g_body.set_cursor((int)strlen(edit.body_buf));
        }
    }

    if (area_hov) body_scroll -= GetMouseWheelMove() * (float)(line_h * 3);
    const float max_scroll = std::max(0.f, (float)(content_h - area_h));
    body_scroll = std::clamp(body_scroll, 0.f, max_scroll);

    if (body_active)
        handle_body_keyboard(edit, line_h, PAD, body_scroll, max_scroll, area_h);

    render_body_edit(area_r, PAD, font, line_h, text_w,
        edit, vlines, body_scroll, content_h, area_h, max_scroll, body_active);

    y += area_h + 6;
}

} // namespace editor_sections
