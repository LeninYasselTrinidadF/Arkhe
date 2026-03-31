#include "editor_sections.hpp"
#include "editor_io.hpp"
#include "body_parser.hpp"
#include "equation_cache.hpp"
#include "../core/font_manager.hpp"
#include "../core/theme.hpp"
#include "../constants.hpp"
#include "../widgets/panel_widget.hpp"   // draw_text_field / draw_labeled_field

#include <algorithm>
#include <cstring>

// Alias cómodos
using RL = ::Color;

namespace editor_sections {

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_node_header
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_node_header(MathNode* sel, int lx, int lw, int& y)
    {
        std::string ninfo = sel->code + "  |  " + sel->label;
        if ((int)ninfo.size() > 62) ninfo = ninfo.substr(0, 61) + "...";

        DrawTextF("Nodo:", lx, y, 11, { 90, 100, 140, 200 });
        DrawTextF(ninfo.c_str(), lx + 46, y, 11, { 160, 200, 160, 230 });
        y += 20;

        DrawLine(lx, y, lx + lw, y, { 30, 30, 50, 200 });
        y += 10;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // IDs internos de campos de texto (deben coincidir con entry_editor.cpp)
    // ─────────────────────────────────────────────────────────────────────────────

    enum FieldId {
        F_NOTE = 0, F_TEX = 1, F_MSC = 2,
        F_KIND = 10, F_TITLE = 11, F_CONTENT = 12
    };

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_node_fields
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_node_fields(MathNode* sel, EditState& edit,
        AppState& state,
        int lx, int lw, int& y, Vector2 mouse)
    {
        int& aid = state.toolbar.active_field;

        PanelWidget::draw_labeled_field(
            "Nota / descripcion:", edit.note_buf, 512,
            lx, y, lw, 11, aid, F_NOTE, mouse);
        sel->note = edit.note_buf;
        y += 38;

        PanelWidget::draw_labeled_field(
            "Clave de imagen (texture_key):", edit.tex_buf, 128,
            lx, y, lw, 11, aid, F_TEX, mouse);
        sel->texture_key = edit.tex_buf;
        y += 38;

        PanelWidget::draw_labeled_field(
            "MSC refs (coma separados):", edit.msc_buf, 512,
            lx, y, lw, 11, aid, F_MSC, mouse);

        // Parsear MSC refs desde el buffer
        sel->msc_refs.clear();
        std::string ms(edit.msc_buf);
        size_t pos;
        while ((pos = ms.find(',')) != std::string::npos) {
            if (!ms.substr(0, pos).empty())
                sel->msc_refs.push_back(ms.substr(0, pos));
            ms = ms.substr(pos + 1);
        }
        if (!ms.empty()) sel->msc_refs.push_back(ms);
        y += 38;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_body_preview (implementación interna)
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_body_preview(MathNode* /*sel*/, EditState& edit,
        AppState& state,
        int lx, int lw,
        int py, int ph,
        int& y, Vector2 mouse,
        float& body_scroll)
    {
        // ── 1. Parsear body_buf ───────────────────────────────────────────────────
        auto segs = parse_body(edit.body_buf);

        // ── 2. Solicitar compilación de ecuaciones únicas ─────────────────────────
        for (auto& seg : segs)
            if (seg.type != BodySegment::Type::Text)
                g_eq_cache.request(seg.content, state);

        // ── 3. Subir texturas terminadas al contexto GL ───────────────────────────
        g_eq_cache.poll();

        // ── 4. Área de preview ────────────────────────────────────────────────────
        const int area_h = std::max(60, py + ph - y - 10);
        Rectangle area_r = { (float)lx, (float)y, (float)lw, (float)area_h };

        DrawRectangleRec(area_r, { 14, 14, 24, 255 });
        DrawRectangleLinesEx(area_r, 1.0f, { 45, 45, 72, 200 });

        // Scroll con rueda del ratón
        if (CheckCollisionPointRec(mouse, area_r)) {
            float wh = GetMouseWheelMove();
            if (wh != 0.0f) body_scroll -= wh * 17.f * 3.f;
            if (body_scroll < 0) body_scroll = 0;
        }

        BeginScissorMode(lx + 1, (int)area_r.y + 1, lw - 2, area_h - 2);

        // ── 5. Render de segmentos ────────────────────────────────────────────────
        constexpr int FONT_SZ = 11;
        constexpr int LINE_H = FONT_SZ + 3;
        constexpr int PAD_X = 6;
        constexpr int MAX_INLINE_H = 28;  // alto máximo para ecuaciones inline

        int draw_y = (int)area_r.y + 4 - (int)body_scroll;
        int total_h = 0;   // para clamp de scroll al final

        for (auto& seg : segs) {

            if (seg.type == BodySegment::Type::Text) {
                // ── Texto plano: split por \n, render línea a línea ───────────────
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
                    draw_y += LINE_H;
                    total_h += LINE_H;
                    start = nl + 1;
                }

            }
            else {
                // ── Ecuación: buscar textura en caché ─────────────────────────────
                EqEntry* entry = g_eq_cache.get(seg.content);
                bool is_display = (seg.type == BodySegment::Type::DisplayMath);

                if (entry && entry->loaded) {
                    // Escalar al tamaño destino
                    float scale;
                    if (is_display) {
                        // DisplayMath: ancho máximo = lw − 2*PAD_X
                        scale = std::min(1.0f,
                            (float)(lw - 2 * PAD_X) / entry->tex.width);
                    }
                    else {
                        // InlineMath: alto máximo = MAX_INLINE_H
                        scale = std::min(1.0f,
                            (float)MAX_INLINE_H / entry->tex.height);
                        scale = std::min(scale,
                            (float)(lw - 2 * PAD_X) / entry->tex.width);
                    }

                    int tw = (int)(entry->tex.width * scale);
                    int th = (int)(entry->tex.height * scale);
                    int tx = is_display
                        ? lx + (lw - tw) / 2          // centrado
                        : lx + PAD_X;                  // alineado a la izquierda

                    if (draw_y + th > (int)area_r.y &&
                        draw_y < (int)(area_r.y + area_h))
                    {
                        // Fondo blanco para la textura (pdflatex genera fondo blanco)
                        DrawRectangle(tx - 2, draw_y - 2, tw + 4, th + 4, WHITE);
                        DrawTextureEx(entry->tex,
                            { (float)tx, (float)draw_y },
                            0.0f, scale, WHITE);
                    }
                    draw_y += th + 6;
                    total_h += th + 6;

                }
                else if (entry && entry->failed) {
                    // Error de compilación
                    if (draw_y + 20 > (int)area_r.y &&
                        draw_y < (int)(area_r.y + area_h))
                    {
                        DrawRectangle(lx + PAD_X, draw_y, lw - 2 * PAD_X, 20,
                            { 50, 10, 10, 220 });
                        DrawTextF("! Error LaTeX", lx + PAD_X + 4, draw_y + 4,
                            10, { 255, 100, 100, 240 });
                    }
                    draw_y += 24;
                    total_h += 24;

                }
                else {
                    // Aún compilando
                    if (draw_y + 20 > (int)area_r.y &&
                        draw_y < (int)(area_r.y + area_h))
                    {
                        DrawRectangle(lx + PAD_X, draw_y, lw - 2 * PAD_X, 20,
                            { 20, 20, 40, 180 });
                        DrawTextF("Compilando...", lx + PAD_X + 4, draw_y + 4,
                            10, { 100, 130, 200, 200 });
                    }
                    draw_y += 24;
                    total_h += 24;
                }
            }
        }

        EndScissorMode();

        // Clamp del scroll
        float max_scr = (float)std::max(0, total_h - area_h + 8);
        if (body_scroll > max_scr) body_scroll = max_scr;

        y += area_h + 6;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_body_section
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_body_section(MathNode* sel, EditState& edit,
        AppState& state,
        int lx, int lw,
        int py, int ph,
        int& y, Vector2 mouse,
        float& body_scroll,
        bool& body_active,
        bool& show_file_manager,
        void (*on_save)(MathNode*, EditState&, AppState&, bool&))
    {
        const int font = 11;
        const int line_h = font + 3;
        const int PAD = 5;

        // Espacio reservado debajo del body para recursos + formulario
        const int RESOURCE_RESERVE = 130;

        // ── Label + nombre de archivo + indicador de dirty ────────────────────────
        DrawTextF("Cuerpo LaTeX:", lx, y, 10, { 100, 110, 150, 220 });
        if (!edit.tex_file.empty()) {
            std::string lbl = "  " + edit.tex_file + (edit.body_dirty ? " *" : "");
            DrawTextF(lbl.c_str(),
                lx + MeasureTextF("Cuerpo LaTeX:", 10) + 6,
                y, 10, { 80, 150, 200, 200 });
        }

        // ── Botones: Preview | Importar | Guardar ─────────────────────────────────
        const int  BTN_W = 56, BTN_H = 16;
        const float btn_y = (float)(y - 1);

        Rectangle prev_r = { (float)(lx + lw - 184), btn_y, (float)BTN_W, (float)BTN_H };
        Rectangle imp_r = { (float)(lx + lw - 124), btn_y, (float)BTN_W, (float)BTN_H };
        Rectangle sav_r = { (float)(lx + lw - 64), btn_y, (float)BTN_W, (float)BTN_H };

        bool prev_hov = CheckCollisionPointRec(mouse, prev_r);
        bool imp_hov = CheckCollisionPointRec(mouse, imp_r);
        bool sav_hov = CheckCollisionPointRec(mouse, sav_r);

        const char* prev_label = edit.preview_mode ? "Editar" : "Preview";
        RL prev_col = edit.preview_mode ? RL{ 50,90,140,255 } : RL{ 28,35,60,255 };
        DrawRectangleRec(prev_r, prev_hov ? RL{ 70,110,170,255 } : prev_col);
        DrawRectangleLinesEx(prev_r, 1.0f, { 60, 110, 200, 200 });
        DrawTextF(prev_label, (int)prev_r.x + 8, (int)prev_r.y + 2, 9, WHITE);
        if (prev_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            edit.preview_mode = !edit.preview_mode;
            body_scroll = 0;
        }

        DrawRectangleRec(imp_r, imp_hov ? RL{ 50,80,140,255 } : RL{ 28,35,60,255 });
        DrawRectangleLinesEx(imp_r, 1.0f, { 60, 100, 200, 200 });
        DrawTextF("Importar", (int)imp_r.x + 4, (int)imp_r.y + 2, 9, WHITE);
        if (imp_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            show_file_manager = !show_file_manager;

        RL sav_bg = sav_hov ? RL{ 60,120,70,255 }
            : edit.body_dirty ? RL{ 40,90,50,255 }
        : RL{ 28,35,60,255 };
        DrawRectangleRec(sav_r, sav_bg);
        DrawRectangleLinesEx(sav_r, 1.0f, { 60, 140, 70, 200 });
        DrawTextF("Guardar", (int)sav_r.x + 4, (int)sav_r.y + 2, 9, WHITE);
        if (sav_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && on_save)
            on_save(sel, edit, state, show_file_manager);

        y += 18;

        // ── Dispatch preview ──────────────────────────────────────────────────────
        if (edit.preview_mode) {
            draw_body_preview(sel, edit, state, lx, lw, py, ph, y, mouse, body_scroll);
            return;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // MODO EDICIÓN
        // ─────────────────────────────────────────────────────────────────────────

        // Ancho útil para texto (menos padding y scrollbar)
        const int text_w = lw - 2 * PAD - 6;

        // ── Word-wrap: partir líneas lógicas (\n) en líneas visuales ─────────────
        // Devuelve las líneas visuales que resultan de ajustar una línea lógica
        // al ancho text_w. Una línea vacía produce exactamente un elemento vacío.
        auto wrap_logical = [&](const std::string& logical) -> std::vector<std::string>
            {
                std::vector<std::string> out;
                std::string s = logical;
                if (s.empty()) { out.push_back(""); return out; }
                while (!s.empty()) {
                    // Avanzar carácter a carácter hasta no caber
                    int chars = 1;
                    while (chars < (int)s.size() &&
                        MeasureTextF(s.substr(0, chars + 1).c_str(), font) <= text_w)
                        chars++;
                    // Si no llegamos al final, romper en el último espacio
                    if (chars < (int)s.size()) {
                        int sp = (int)s.rfind(' ', chars);
                        if (sp > 0) chars = sp;
                    }
                    out.push_back(s.substr(0, chars));
                    s = s.substr(chars);
                    if (!s.empty() && s[0] == ' ') s = s.substr(1); // descartar espacio líder
                }
                return out;
            };

        // Construir el vector completo de líneas visuales
        std::vector<std::string> vlines;
        {
            const char* p = edit.body_buf;
            while (true) {
                const char* nl = strchr(p, '\n');
                int seg_len = nl ? (int)(nl - p) : (int)strlen(p);
                auto wl = wrap_logical(std::string(p, seg_len));
                for (auto& l : wl) vlines.push_back(l);
                if (!nl) break;
                p = nl + 1;
            }
        }
        if (vlines.empty()) vlines.push_back("");

        // ── Altura dinámica ───────────────────────────────────────────────────────
        const int min_body_h = line_h * 3 + 2 * PAD;
        const int max_body_h = std::max(min_body_h, (py + ph) - y - RESOURCE_RESERVE);
        const int content_h = (int)vlines.size() * line_h + 2 * PAD;
        const int area_h = std::clamp(content_h, min_body_h, max_body_h);

        Rectangle area_r = { (float)lx, (float)y, (float)lw, (float)area_h };
        bool area_hov = CheckCollisionPointRec(mouse, area_r);

        // ── Fondo y borde ─────────────────────────────────────────────────────────
        RL border = body_active ? RL{ 70,130,255,255 }
            : area_hov ? RL{ 70,70,120,255 }
        : RL{ 45,45,72,200 };
        DrawRectangleRec(area_r, { 18, 18, 32, 255 });
        DrawRectangleLinesEx(area_r, 1.0f, border);

        // Click activa / desactiva
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            body_active = area_hov;
            if (area_hov) state.toolbar.active_field = -99;
        }

        // ── Scroll con rueda ──────────────────────────────────────────────────────
        if (area_hov)
            body_scroll -= GetMouseWheelMove() * (float)(line_h * 3);

        const float max_scroll = std::max(0.f, (float)(content_h - area_h));
        body_scroll = std::clamp(body_scroll, 0.f, max_scroll);

        // ── Input ─────────────────────────────────────────────────────────────────
        if (body_active) {
            // Caracteres imprimibles
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(edit.body_buf);
                if (len < 8190) {
                    edit.body_buf[len] = (char)key;
                    edit.body_buf[len + 1] = '\0';
                    edit.body_dirty = true;
                }
                key = GetCharPressed();
            }
            // Enter → \n canónico
            if (IsKeyPressed(KEY_ENTER)) {
                int len = (int)strlen(edit.body_buf);
                if (len < 8190) {
                    edit.body_buf[len] = '\n';
                    edit.body_buf[len + 1] = '\0';
                    edit.body_dirty = true;
                }
            }
            // Backspace (con repeat)
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                int len = (int)strlen(edit.body_buf);
                if (len > 0) { edit.body_buf[len - 1] = '\0'; edit.body_dirty = true; }
            }

            // Auto-scroll: mantener el cursor (última línea visual) visible
            const float cursor_bottom = (float)((int)vlines.size() * line_h + PAD);
            if (cursor_bottom - body_scroll > (float)area_h)
                body_scroll = cursor_bottom - (float)area_h + (float)PAD;
            body_scroll = std::clamp(body_scroll, 0.f, max_scroll);
        }

        // ── Render con scissor ────────────────────────────────────────────────────
        BeginScissorMode((int)area_r.x + 1, (int)area_r.y + 1,
            (int)area_r.width - 2, (int)area_r.height - 2);
        {
            int draw_y = (int)area_r.y + PAD - (int)body_scroll;
            for (auto& line : vlines) {
                if (draw_y + line_h > (int)area_r.y && draw_y < (int)area_r.y + area_h)
                    DrawTextF(line.c_str(), (int)area_r.x + PAD, draw_y,
                        font, { 200, 210, 220, 210 });
                draw_y += line_h;
            }

            // Cursor parpadeante al final del buffer
            if (body_active && (int)(GetTime() * 2) % 2 == 0) {
                int last_y = (int)area_r.y + PAD - (int)body_scroll
                    + ((int)vlines.size() - 1) * line_h;
                int cx = (int)area_r.x + PAD
                    + MeasureTextF(vlines.back().c_str(), font);
                if (last_y >= (int)area_r.y && last_y < (int)area_r.y + area_h)
                    DrawLine(cx, last_y + 1, cx, last_y + font + 1, { 150, 180, 255, 255 });
            }
        }
        EndScissorMode();

        // ── Scrollbar (solo cuando hay overflow) ──────────────────────────────────
        if (content_h > area_h) {
            float ratio = (float)area_h / (float)content_h;
            float thumb_h = std::max(16.f, ratio * (float)area_h);
            float thumb_y = (float)area_r.y
                + (max_scroll > 0.f ? body_scroll / max_scroll : 0.f)
                * ((float)area_h - thumb_h);
            DrawRectangle((int)(area_r.x + area_r.width) - 5,
                (int)area_r.y, 4, area_h, { 30, 30, 50, 180 });
            DrawRectangle((int)(area_r.x + area_r.width) - 5,
                (int)thumb_y, 4, (int)thumb_h, { 80, 110, 200, 200 });
        }

        // ── Avanzar y (deja RESOURCE_RESERVE libre abajo) ─────────────────────────
        y += area_h + 6;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_file_manager
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_file_manager(MathNode* sel, EditState& edit,
        AppState& state,
        int panel_x, int panel_y,
        Vector2 mouse,
        bool& show_file_manager,
        std::vector<std::string>& tex_files,
        float& file_list_scroll,
        bool& files_stale,
        std::unordered_map<std::string, std::string>& entries_index)
    {
        if (!show_file_manager) return;

        const int fm_w = 260, fm_h = 230;
        const int fm_x = std::max(10, panel_x - fm_w - 6);
        const int fm_y = panel_y + 30;

        // Sombra + fondo
        DrawRectangle(fm_x + 3, fm_y + 3, fm_w, fm_h, { 0,0,0,100 });
        DrawRectangle(fm_x, fm_y, fm_w, fm_h, { 12,12,22,252 });
        DrawRectangleLinesEx({ (float)fm_x,(float)fm_y,(float)fm_w,(float)fm_h },
            1.0f, { 60,80,160,220 });

        // Header
        DrawRectangle(fm_x, fm_y, fm_w, 26, { 18,22,42,255 });
        DrawTextF("Archivos .tex", fm_x + 10, fm_y + 7, 11, { 140,170,255,255 });

        // Botón refrescar
        Rectangle ref_r = { (float)(fm_x + fm_w - 60),(float)(fm_y + 4), 52.0f, 18.0f };
        bool ref_hov = CheckCollisionPointRec(mouse, ref_r);
        DrawRectangleRec(ref_r, ref_hov ? RL{ 40,60,120,255 } : RL{ 25,30,55,255 });
        DrawRectangleLinesEx(ref_r, 1.0f, { 50,80,160,200 });
        DrawTextF("Refrescar", (int)ref_r.x + 3, (int)ref_r.y + 3, 9, WHITE);
        if (ref_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            tex_files = editor_io::list_tex_files(state);

        // Lista scrolleable
        const int item_h = 22;
        const int list_y = fm_y + 30;
        const int visible_h = fm_h - 56;

        Rectangle list_r = { (float)fm_x,(float)list_y,(float)fm_w,(float)visible_h };
        if (CheckCollisionPointRec(mouse, list_r))
            file_list_scroll -= GetMouseWheelMove() * item_h * 2;

        int max_scr = std::max(0, (int)tex_files.size() * item_h - visible_h);
        file_list_scroll = std::clamp(file_list_scroll, 0.0f, (float)max_scr);

        BeginScissorMode((int)list_r.x, (int)list_r.y,
            (int)list_r.width, (int)list_r.height);
        for (int i = 0; i < (int)tex_files.size(); i++) {
            int  iy = list_y + i * item_h - (int)file_list_scroll;
            if (iy + item_h <= list_y || iy >= list_y + visible_h) continue;
            bool is_cur = (tex_files[i] == edit.tex_file);
            Rectangle ir = { (float)fm_x,(float)iy,(float)fm_w,(float)item_h };
            bool hov = CheckCollisionPointRec(mouse, ir);
            DrawRectangleRec(ir,
                is_cur ? RL{ 30,50,100,255 }
                : hov ? RL{ 25,30,55,255 }
            : RL{ 0,0,0,0 });
            DrawTextF(tex_files[i].c_str(), fm_x + 10, iy + 5, 10,
                is_cur ? RL{ 150,200,255,255 } : RL{ 180,185,210,220 });
            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                std::string body = editor_io::read_tex(state, tex_files[i]);
                if (body.size() > 8191) body.resize(8191);
                strncpy(edit.body_buf, body.c_str(), 8191);
                edit.body_buf[8191] = '\0';
                edit.tex_file = tex_files[i];
                edit.body_dirty = false;
                if (sel) {
                    entries_index[sel->code] = tex_files[i];
                    editor_io::save_index(state, entries_index);
                }
                show_file_manager = false;
                files_stale = true;
            }
        }
        if (tex_files.empty())
            DrawTextF("(carpeta vacia)", fm_x + 10, list_y + 8, 10, { 80,85,120,180 });
        EndScissorMode();

        // Botón «Nuevo .tex»
        Rectangle new_r = { (float)(fm_x + 8),(float)(fm_y + fm_h - 22),
                           (float)(fm_w - 16), 18.0f };
        bool new_hov = CheckCollisionPointRec(mouse, new_r);
        DrawRectangleRec(new_r, new_hov ? RL{ 30,70,40,255 } : RL{ 20,45,28,255 });
        DrawRectangleLinesEx(new_r, 1.0f, { 50,120,60,200 });
        DrawTextF("Nuevo .tex para este nodo", (int)new_r.x + 10, (int)new_r.y + 3, 10, WHITE);
        if (new_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sel) {
            edit.tex_file = editor_io::safe_filename(sel->code);
            edit.body_buf[0] = '\0';
            edit.body_dirty = false;
            show_file_manager = false;
            files_stale = true;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_resource_list
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_resource_list(MathNode* sel,
        int lx, int lw,
        int py, int ph,
        int& y, Vector2 mouse)
    {
        DrawTextF("RECURSOS (en memoria)", lx, y, 11, { 100,130,100,220 });
        y += 18;

        int to_remove = -1;
        for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
            auto& res = sel->resources[i];
            std::string line = "[" + res.kind + "] " + res.title;
            if ((int)line.size() > 54) line = line.substr(0, 53) + "...";

            DrawRectangle(lx, y, lw - 26, 20, { 18,22,18,255 });
            DrawTextF(line.c_str(), lx + 4, y + 4, 10, { 170,200,170,210 });

            Rectangle del_r = { (float)(lx + lw - 22),(float)y, 20.0f, 20.0f };
            bool del_hov = CheckCollisionPointRec(mouse, del_r);
            DrawRectangleRec(del_r, del_hov ? RL{ 160,40,40,255 } : RL{ 40,20,20,200 });
            DrawTextF("x", (int)del_r.x + 6, (int)del_r.y + 4, 11, WHITE);
            if (del_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) to_remove = i;
            y += 24;
        }
        if (to_remove >= 0)
            sel->resources.erase(sel->resources.begin() + to_remove);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // draw_add_resource_form
    // ─────────────────────────────────────────────────────────────────────────────

    void draw_add_resource_form(MathNode* sel, EditState& edit,
        AppState& state,
        int lx, int lw,
        int py, int ph,
        int& y, Vector2 mouse)
    {
        if (y + 70 >= py + ph) return;
        int& aid = state.toolbar.active_field;

        DrawTextF("+ Nuevo recurso:", lx, y, 10, { 90,130,90,200 });
        y += 14;

        PanelWidget::draw_text_field(edit.new_kind, 32,
            lx, y, 60, 18, 10, aid, F_KIND, mouse);
        PanelWidget::draw_text_field(edit.new_title, 128,
            lx + 66, y, lw / 2 - 70, 18, 10, aid, F_TITLE, mouse);
        y += 22;

        const int add_bw = 54;
        PanelWidget::draw_text_field(edit.new_content, 256,
            lx, y, lw - add_bw - 4, 18, 10, aid, F_CONTENT, mouse);

        Rectangle add_r = { (float)(lx + lw - add_bw),(float)y,(float)add_bw, 20.0f };
        bool add_hov = CheckCollisionPointRec(mouse, add_r);
        DrawRectangleRec(add_r, add_hov ? RL{ 40,100,50,255 } : RL{ 28,60,34,255 });
        DrawRectangleLinesEx(add_r, 1.0f, { 60,140,70,200 });
        DrawTextF("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);

        if (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && strlen(edit.new_title) > 0)
        {
            Resource r;
            r.kind = edit.new_kind;
            r.title = edit.new_title;
            r.content = edit.new_content;
            sel->resources.push_back(r);
            edit.new_title[0] = '\0';
            edit.new_content[0] = '\0';
        }
    }

} // namespace editor_sections