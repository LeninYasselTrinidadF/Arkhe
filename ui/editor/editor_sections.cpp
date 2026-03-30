#include "editor_sections.hpp"
#include "editor_io.hpp"
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
// draw_body_section
// ─────────────────────────────────────────────────────────────────────────────

void draw_body_section(MathNode* sel, EditState& edit,
                       AppState& state,
                       int lx, int lw,
                       int py, int ph,
                       int& y, Vector2 mouse,
                       float& body_scroll,
                       bool&  body_active,
                       bool&  show_file_manager,
                       void (*on_save)(MathNode*, EditState&, AppState&, bool&))
{
    const int font   = 11;
    const int line_h = font + 3;
    const int area_h = 130;

    // ── Label + nombre de archivo + indicador de dirty ────────────────────────
    DrawTextF("Cuerpo LaTeX:", lx, y, 10, { 100, 110, 150, 220 });
    if (!edit.tex_file.empty()) {
        std::string lbl = "  " + edit.tex_file + (edit.body_dirty ? " *" : "");
        DrawTextF(lbl.c_str(),
                  lx + MeasureTextF("Cuerpo LaTeX:", 10) + 6,
                  y, 10, { 80, 150, 200, 200 });
    }

    // ── Botones Importar / Guardar ────────────────────────────────────────────
    Rectangle imp_r = { (float)(lx + lw - 120), (float)(y - 1), 56.0f, 16.0f };
    Rectangle sav_r = { (float)(lx + lw - 60),  (float)(y - 1), 56.0f, 16.0f };

    bool imp_hov = CheckCollisionPointRec(mouse, imp_r);
    bool sav_hov = CheckCollisionPointRec(mouse, sav_r);

    DrawRectangleRec(imp_r, imp_hov ? RL{50,80,140,255} : RL{28,35,60,255});
    DrawRectangleLinesEx(imp_r, 1.0f, {60,100,200,200});
    DrawTextF("Importar", (int)imp_r.x + 4, (int)imp_r.y + 2, 9, WHITE);

    RL sav_bg = sav_hov          ? RL{60,120,70,255}
              : edit.body_dirty  ? RL{40,90,50,255}
                                 : RL{28,35,60,255};
    DrawRectangleRec(sav_r, sav_bg);
    DrawRectangleLinesEx(sav_r, 1.0f, {60,140,70,200});
    DrawTextF("Guardar", (int)sav_r.x + 4, (int)sav_r.y + 2, 9, WHITE);

    if (imp_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        show_file_manager = !show_file_manager;

    if (sav_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && on_save)
        on_save(sel, edit, state, show_file_manager);

    y += 18;

    // ── Textarea ──────────────────────────────────────────────────────────────
    Rectangle area_r = { (float)lx, (float)y, (float)lw, (float)area_h };
    bool area_hov = CheckCollisionPointRec(mouse, area_r);

    RL border = body_active ? RL{70,130,255,255}
              : area_hov   ? RL{70,70,120,255}
                           : RL{45,45,72,200};
    DrawRectangleRec(area_r, {18,18,32,255});
    DrawRectangleLinesEx(area_r, 1.0f, border);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        body_active = area_hov;
        if (area_hov) state.toolbar.active_field = -99;
    }

    // Input de teclado
    if (body_active) {
        if (area_hov) {
            body_scroll -= GetMouseWheelMove() * line_h * 3;
            if (body_scroll < 0) body_scroll = 0;
        }
        int key = GetCharPressed();
        while (key > 0) {
            int len = (int)strlen(edit.body_buf);
            if (len < 8190) {
                edit.body_buf[len]     = (char)key;
                edit.body_buf[len + 1] = '\0';
                edit.body_dirty = true;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_ENTER)) {
            int len = (int)strlen(edit.body_buf);
            if (len < 8190) {
                edit.body_buf[len]     = '\n';
                edit.body_buf[len + 1] = '\0';
                edit.body_dirty = true;
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            int len = (int)strlen(edit.body_buf);
            if (len > 0) { edit.body_buf[len - 1] = '\0'; edit.body_dirty = true; }
        }
    }

    // ── Render de texto con scissor ───────────────────────────────────────────
    BeginScissorMode((int)area_r.x + 1, (int)area_r.y + 1,
                     (int)area_r.width - 2, (int)area_r.height - 2);
    {
        int draw_y   = (int)area_r.y + 4 - (int)body_scroll;
        int n_lines  = 0;
        const char* line_start = edit.body_buf;
        const char* last_start = edit.body_buf;

        for (const char* p = edit.body_buf; ; p++) {
            if (*p == '\n' || *p == '\0') {
                int len = (int)(p - line_start);
                if (draw_y + line_h > (int)area_r.y &&
                    draw_y < (int)(area_r.y + area_r.height))
                {
                    char tmp[256];
                    int  copy = len < 255 ? len : 255;
                    strncpy(tmp, line_start, copy); tmp[copy] = '\0';
                    DrawTextF(tmp, lx + 4, draw_y, font, {200,210,220,210});
                }
                last_start = p + 1;
                draw_y += line_h;
                n_lines++;
                if (*p == '\0') break;
                line_start = p + 1;
            }
        }

        // Cursor parpadeante al final de la última línea
        if (body_active && (int)(GetTime() * 2) % 2 == 0) {
            const char* last_nl = nullptr;
            for (const char* p = edit.body_buf; *p; p++)
                if (*p == '\n') last_nl = p;
            const char* last_line = last_nl ? last_nl + 1 : edit.body_buf;
            int cy = (int)area_r.y + 4 - (int)body_scroll + (n_lines - 1) * line_h;
            int cx = lx + 4 + MeasureTextF(last_line, font);
            if (cy >= (int)area_r.y && cy < (int)(area_r.y + area_r.height))
                DrawLine(cx, cy, cx, cy + font + 1, {150,180,255,220});
        }

        // Clamp scroll
        float max_scroll = (float)std::max(0, n_lines * line_h - area_h + 8);
        if (body_scroll > max_scroll) body_scroll = max_scroll;
    }
    EndScissorMode();

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
                       std::unordered_map<std::string,std::string>& entries_index)
{
    if (!show_file_manager) return;

    const int fm_w = 260, fm_h = 230;
    const int fm_x = std::max(10, panel_x - fm_w - 6);
    const int fm_y = panel_y + 30;

    // Sombra + fondo
    DrawRectangle(fm_x + 3, fm_y + 3, fm_w, fm_h, {0,0,0,100});
    DrawRectangle(fm_x, fm_y, fm_w, fm_h, {12,12,22,252});
    DrawRectangleLinesEx({(float)fm_x,(float)fm_y,(float)fm_w,(float)fm_h},
                         1.0f, {60,80,160,220});

    // Header
    DrawRectangle(fm_x, fm_y, fm_w, 26, {18,22,42,255});
    DrawTextF("Archivos .tex", fm_x + 10, fm_y + 7, 11, {140,170,255,255});

    // Botón refrescar
    Rectangle ref_r = {(float)(fm_x + fm_w - 60),(float)(fm_y + 4), 52.0f, 18.0f};
    bool ref_hov = CheckCollisionPointRec(mouse, ref_r);
    DrawRectangleRec(ref_r, ref_hov ? RL{40,60,120,255} : RL{25,30,55,255});
    DrawRectangleLinesEx(ref_r, 1.0f, {50,80,160,200});
    DrawTextF("Refrescar", (int)ref_r.x + 3, (int)ref_r.y + 3, 9, WHITE);
    if (ref_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        tex_files = editor_io::list_tex_files(state);

    // Lista scrolleable
    const int item_h    = 22;
    const int list_y    = fm_y + 30;
    const int visible_h = fm_h - 56;

    Rectangle list_r = {(float)fm_x,(float)list_y,(float)fm_w,(float)visible_h};
    if (CheckCollisionPointRec(mouse, list_r))
        file_list_scroll -= GetMouseWheelMove() * item_h * 2;

    int max_scr = std::max(0, (int)tex_files.size() * item_h - visible_h);
    file_list_scroll = std::clamp(file_list_scroll, 0.0f, (float)max_scr);

    BeginScissorMode((int)list_r.x, (int)list_r.y,
                     (int)list_r.width, (int)list_r.height);
    for (int i = 0; i < (int)tex_files.size(); i++) {
        int  iy     = list_y + i * item_h - (int)file_list_scroll;
        if (iy + item_h <= list_y || iy >= list_y + visible_h) continue;
        bool is_cur = (tex_files[i] == edit.tex_file);
        Rectangle ir = {(float)fm_x,(float)iy,(float)fm_w,(float)item_h};
        bool hov = CheckCollisionPointRec(mouse, ir);
        DrawRectangleRec(ir,
            is_cur ? RL{30,50,100,255}
          : hov    ? RL{25,30,55,255}
                   : RL{0,0,0,0});
        DrawTextF(tex_files[i].c_str(), fm_x + 10, iy + 5, 10,
            is_cur ? RL{150,200,255,255} : RL{180,185,210,220});
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // Cargar archivo seleccionado
            std::string body = editor_io::read_tex(state, tex_files[i]);
            if (body.size() > 8191) body.resize(8191);
            strncpy(edit.body_buf, body.c_str(), 8191);
            edit.body_buf[8191] = '\0';
            edit.tex_file       = tex_files[i];
            edit.body_dirty     = false;
            if (sel) {
                entries_index[sel->code] = tex_files[i];
                editor_io::save_index(state, entries_index);
            }
            show_file_manager = false;
            files_stale = true;
        }
    }
    if (tex_files.empty())
        DrawTextF("(carpeta vacia)", fm_x + 10, list_y + 8, 10, {80,85,120,180});
    EndScissorMode();

    // Botón «Nuevo .tex»
    Rectangle new_r = {(float)(fm_x + 8),(float)(fm_y + fm_h - 22),
                       (float)(fm_w - 16), 18.0f};
    bool new_hov = CheckCollisionPointRec(mouse, new_r);
    DrawRectangleRec(new_r, new_hov ? RL{30,70,40,255} : RL{20,45,28,255});
    DrawRectangleLinesEx(new_r, 1.0f, {50,120,60,200});
    DrawTextF("Nuevo .tex para este nodo", (int)new_r.x + 10, (int)new_r.y + 3, 10, WHITE);
    if (new_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sel) {
        edit.tex_file       = editor_io::safe_filename(sel->code);
        edit.body_buf[0]    = '\0';
        edit.body_dirty     = false;
        show_file_manager   = false;
        files_stale         = true;
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
    DrawTextF("RECURSOS (en memoria)", lx, y, 11, {100,130,100,220});
    y += 18;

    int to_remove = -1;
    for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
        auto& res  = sel->resources[i];
        std::string line = "[" + res.kind + "] " + res.title;
        if ((int)line.size() > 54) line = line.substr(0, 53) + "...";

        DrawRectangle(lx, y, lw - 26, 20, {18,22,18,255});
        DrawTextF(line.c_str(), lx + 4, y + 4, 10, {170,200,170,210});

        Rectangle del_r = {(float)(lx + lw - 22),(float)y, 20.0f, 20.0f};
        bool del_hov = CheckCollisionPointRec(mouse, del_r);
        DrawRectangleRec(del_r, del_hov ? RL{160,40,40,255} : RL{40,20,20,200});
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

    DrawTextF("+ Nuevo recurso:", lx, y, 10, {90,130,90,200});
    y += 14;

    PanelWidget::draw_text_field(edit.new_kind, 32,
        lx, y, 60, 18, 10, aid, F_KIND, mouse);
    PanelWidget::draw_text_field(edit.new_title, 128,
        lx + 66, y, lw / 2 - 70, 18, 10, aid, F_TITLE, mouse);
    y += 22;

    const int add_bw = 54;
    PanelWidget::draw_text_field(edit.new_content, 256,
        lx, y, lw - add_bw - 4, 18, 10, aid, F_CONTENT, mouse);

    Rectangle add_r = {(float)(lx + lw - add_bw),(float)y,(float)add_bw, 20.0f};
    bool add_hov = CheckCollisionPointRec(mouse, add_r);
    DrawRectangleRec(add_r, add_hov ? RL{40,100,50,255} : RL{28,60,34,255});
    DrawRectangleLinesEx(add_r, 1.0f, {60,140,70,200});
    DrawTextF("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);

    if (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        && strlen(edit.new_title) > 0)
    {
        Resource r;
        r.kind    = edit.new_kind;
        r.title   = edit.new_title;
        r.content = edit.new_content;
        sel->resources.push_back(r);
        edit.new_title[0]   = '\0';
        edit.new_content[0] = '\0';
    }
}

} // namespace editor_sections
