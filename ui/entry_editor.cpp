#include "entry_editor.hpp"
#include "constants.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ── Rutas ─────────────────────────────────────────────────────────────────────

std::string EntryEditor::index_path() const {
    return std::string(state.toolbar.entries_path) + "entries_index.json";
}

std::string EntryEditor::full_tex_path(const std::string& f) const {
    return std::string(state.toolbar.entries_path) + f;
}

// ── Índice en disco ───────────────────────────────────────────────────────────

void EntryEditor::load_entries_index() {
    entries_index.clear();
    std::ifstream f(index_path());
    if (!f.is_open()) return;
    try {
        json j = json::parse(f);
        for (auto& [k, v] : j.items())
            entries_index[k] = v.get<std::string>();
    }
    catch (...) {}
}

void EntryEditor::save_entries_index() {
    try { fs::create_directories(state.toolbar.entries_path); }
    catch (...) {}
    json j = json::object();
    for (auto& [k, v] : entries_index) j[k] = v;
    std::ofstream f(index_path());
    if (f.is_open()) f << j.dump(2);
}

// ── Archivos .tex ─────────────────────────────────────────────────────────────

void EntryEditor::refresh_file_list() {
    tex_files.clear();
    try {
        for (auto& e : fs::directory_iterator(state.toolbar.entries_path))
            if (e.is_regular_file() && e.path().extension() == ".tex")
                tex_files.push_back(e.path().filename().string());
    }
    catch (...) {}
    std::sort(tex_files.begin(), tex_files.end());
    files_stale = false;
}

void EntryEditor::load_tex_file(const std::string& filename, MathNode* sel) {
    std::ifstream f(full_tex_path(filename));
    if (!f.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(f)), {});
    if (content.size() > 8191) content.resize(8191);
    strncpy(edit.body_buf, content.c_str(), 8191); edit.body_buf[8191] = '\0';
    edit.tex_file = filename;
    edit.body_dirty = false;
    body_scroll = 0.0f;
    if (sel) { entries_index[sel->code] = filename; save_entries_index(); }
}

void EntryEditor::save_tex_file(MathNode* sel) {
    if (!sel) return;
    if (edit.tex_file.empty()) {
        std::string safe = sel->code;
        for (char& c : safe)
            if (c == '/' || c == '\\' || c == ':' || c == ' ') c = '_';
        edit.tex_file = safe + ".tex";
    }
    try { fs::create_directories(state.toolbar.entries_path); }
    catch (...) {}
    std::ofstream f(full_tex_path(edit.tex_file));
    if (!f.is_open()) return;
    f << edit.body_buf;
    edit.body_dirty = false;
    entries_index[sel->code] = edit.tex_file;
    save_entries_index();
    files_stale = true;
}

// ── EditState::sync ───────────────────────────────────────────────────────────

void EntryEditor::EditState::sync(MathNode* sel,
    const std::string& body, const std::string& fname)
{
    if (!sel || sel->code == last_code) return;
    last_code = sel->code;
    tex_file = fname;
    body_dirty = false;

    strncpy(note_buf, sel->note.c_str(), 511); note_buf[511] = '\0';
    strncpy(tex_buf, sel->texture_key.c_str(), 127); tex_buf[127] = '\0';
    strncpy(body_buf, body.c_str(), 8191); body_buf[8191] = '\0';

    std::string joined;
    for (int i = 0; i < (int)sel->msc_refs.size(); i++) {
        if (i > 0) joined += ',';
        joined += sel->msc_refs[i];
    }
    strncpy(msc_buf, joined.c_str(), 511); msc_buf[511] = '\0';

    new_title[0] = '\0';
    new_content[0] = '\0';
}

// ── draw_node_fields ──────────────────────────────────────────────────────────

void EntryEditor::draw_node_fields(MathNode* sel, int lx, int lw, int& y, Vector2 mouse) {
    int& aid = state.toolbar.active_field;

    draw_labeled_field("Nota / descripcion:", edit.note_buf, 512, lx, y, lw, 11, aid, F_NOTE, mouse);
    sel->note = edit.note_buf; y += 38;

    draw_labeled_field("Clave de imagen (texture_key):", edit.tex_buf, 128, lx, y, lw, 11, aid, F_TEX, mouse);
    sel->texture_key = edit.tex_buf; y += 38;

    draw_labeled_field("MSC refs (coma separados):", edit.msc_buf, 512, lx, y, lw, 11, aid, F_MSC, mouse);
    sel->msc_refs.clear();
    std::string ms(edit.msc_buf);
    size_t pos;
    while ((pos = ms.find(',')) != std::string::npos) {
        if (!ms.substr(0, pos).empty()) sel->msc_refs.push_back(ms.substr(0, pos));
        ms = ms.substr(pos + 1);
    }
    if (!ms.empty()) sel->msc_refs.push_back(ms);
    y += 38;
}

// ── draw_body_section ─────────────────────────────────────────────────────────

void EntryEditor::draw_body_section(MathNode* sel, int lx, int lw,
    int py, int ph, int& y, Vector2 mouse)
{
    const int font = 11;
    const int line_h = font + 3;
    const int area_h = 130;

    // ── Label + filename + dirty ─────────────────────────────────────────────
    DrawText("Cuerpo LaTeX:", lx, y, 10, { 100, 110, 150, 220 });
    if (!edit.tex_file.empty()) {
        std::string lbl = "  " + edit.tex_file + (edit.body_dirty ? " *" : "");
        DrawText(lbl.c_str(), lx + MeasureText("Cuerpo LaTeX:", 10) + 6,
            y, 10, { 80, 150, 200, 200 });
    }

    // ── Botones Importar / Guardar ───────────────────────────────────────────
    Rectangle imp_r = { (float)(lx + lw - 120), (float)(y - 1), 56.0f, 16.0f };
    Rectangle sav_r = { (float)(lx + lw - 60),  (float)(y - 1), 56.0f, 16.0f };

    bool imp_hov = CheckCollisionPointRec(mouse, imp_r);
    bool sav_hov = CheckCollisionPointRec(mouse, sav_r);

    DrawRectangleRec(imp_r, imp_hov ? Color{ 50,80,140,255 } : Color{ 28,35,60,255 });
    DrawRectangleLinesEx(imp_r, 1.0f, { 60,100,200,200 });
    DrawText("Importar", (int)imp_r.x + 4, (int)imp_r.y + 2, 9, WHITE);

    Color sav_bg = sav_hov ? Color{ 60,120,70,255 }
        : edit.body_dirty ? Color{ 40,90,50,255 }
    : Color{ 28,35,60,255 };
    DrawRectangleRec(sav_r, sav_bg);
    DrawRectangleLinesEx(sav_r, 1.0f, { 60,140,70,200 });
    DrawText("Guardar", (int)sav_r.x + 4, (int)sav_r.y + 2, 9, WHITE);

    if (imp_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        show_file_manager = !show_file_manager;
        if (show_file_manager && files_stale) refresh_file_list();
    }
    if (sav_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        save_tex_file(sel);

    y += 18;

    // ── Textarea ─────────────────────────────────────────────────────────────
    Rectangle area_r = { (float)lx, (float)y, (float)lw, (float)area_h };
    bool area_hov = CheckCollisionPointRec(mouse, area_r);

    Color border = body_active ? Color{ 70,130,255,255 }
        : area_hov ? Color{ 70,70,120,255 }
    : Color{ 45,45,72,200 };
    DrawRectangleRec(area_r, { 18,18,32,255 });
    DrawRectangleLinesEx(area_r, 1.0f, border);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        body_active = area_hov;
        if (area_hov) state.toolbar.active_field = -99;  // desactiva campos de texto normales
    }

    if (body_active) {
        if (area_hov) {
            body_scroll -= GetMouseWheelMove() * line_h * 3;
            if (body_scroll < 0) body_scroll = 0;
        }
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
        if (IsKeyPressed(KEY_ENTER)) {
            int len = (int)strlen(edit.body_buf);
            if (len < 8190) {
                edit.body_buf[len] = '\n';
                edit.body_buf[len + 1] = '\0';
                edit.body_dirty = true;
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            int len = (int)strlen(edit.body_buf);
            if (len > 0) { edit.body_buf[len - 1] = '\0'; edit.body_dirty = true; }
        }
    }

    // ── Render texto con scissor ─────────────────────────────────────────────
    BeginScissorMode((int)area_r.x + 1, (int)area_r.y + 1,
        (int)area_r.width - 2, (int)area_r.height - 2);
    {
        int draw_y = (int)area_r.y + 4 - (int)body_scroll;
        int n_lines = 0;
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
                    DrawText(tmp, lx + 4, draw_y, font, { 200,210,220,210 });
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
            int last_len = (int)strlen(last_start > edit.body_buf ? last_start - 1 : last_start);
            // last_start apunta al comienzo de la última línea
            const char* ls = (last_start > edit.body_buf) ? last_start : edit.body_buf;
            // Recalcular: la última línea es desde el último \n hasta el final
            const char* last_nl = nullptr;
            for (const char* p = edit.body_buf; *p; p++)
                if (*p == '\n') last_nl = p;
            const char* last_line = last_nl ? last_nl + 1 : edit.body_buf;
            int cy = (int)area_r.y + 4 - (int)body_scroll + (n_lines - 1) * line_h;
            int cx = lx + 4 + MeasureText(last_line, font);
            if (cy >= (int)area_r.y && cy < (int)(area_r.y + area_r.height))
                DrawLine(cx, cy, cx, cy + font + 1, { 150,180,255,220 });
        }

        // Clamp scroll
        float max_scroll = (float)std::max(0, n_lines * line_h - area_h + 8);
        if (body_scroll > max_scroll) body_scroll = max_scroll;
    }
    EndScissorMode();

    y += area_h + 6;
}

// ── draw_file_manager ─────────────────────────────────────────────────────────

void EntryEditor::draw_file_manager(MathNode* sel, int px, int py, Vector2 mouse) {
    if (!show_file_manager) return;

    const int fm_w = 260, fm_h = 230;
    const int fm_x = std::max(10, px - fm_w - 6);
    const int fm_y = py + 30;

    // Sombra + fondo
    DrawRectangle(fm_x + 3, fm_y + 3, fm_w, fm_h, { 0,0,0,100 });
    DrawRectangle(fm_x, fm_y, fm_w, fm_h, { 12,12,22,252 });
    DrawRectangleLinesEx({ (float)fm_x,(float)fm_y,(float)fm_w,(float)fm_h },
        1.0f, { 60,80,160,220 });

    // Header
    DrawRectangle(fm_x, fm_y, fm_w, 26, { 18,22,42,255 });
    DrawText("Archivos .tex", fm_x + 10, fm_y + 7, 11, { 140,170,255,255 });

    // Botón refrescar
    Rectangle ref_r = { (float)(fm_x + fm_w - 60),(float)(fm_y + 4), 52.0f, 18.0f };
    bool ref_hov = CheckCollisionPointRec(mouse, ref_r);
    DrawRectangleRec(ref_r, ref_hov ? Color{ 40,60,120,255 } : Color{ 25,30,55,255 });
    DrawRectangleLinesEx(ref_r, 1.0f, { 50,80,160,200 });
    DrawText("Refrescar", (int)ref_r.x + 3, (int)ref_r.y + 3, 9, WHITE);
    if (ref_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) refresh_file_list();

    // Lista
    const int item_h = 22;
    const int list_y = fm_y + 30;
    const int visible_h = fm_h - 56;

    Rectangle list_r = { (float)fm_x,(float)list_y,(float)fm_w,(float)visible_h };
    if (CheckCollisionPointRec(mouse, list_r))
        file_list_scroll -= GetMouseWheelMove() * item_h * 2;
    int max_scr = std::max(0, (int)tex_files.size() * item_h - visible_h);
    if (file_list_scroll < 0)        file_list_scroll = 0;
    if (file_list_scroll > max_scr)  file_list_scroll = (float)max_scr;

    BeginScissorMode((int)list_r.x, (int)list_r.y, (int)list_r.width, (int)list_r.height);
    for (int i = 0; i < (int)tex_files.size(); i++) {
        int iy = list_y + i * item_h - (int)file_list_scroll;
        if (iy + item_h <= list_y || iy >= list_y + visible_h) continue;
        bool is_cur = (tex_files[i] == edit.tex_file);
        Rectangle ir = { (float)fm_x,(float)iy,(float)fm_w,(float)item_h };
        bool hov = CheckCollisionPointRec(mouse, ir);
        DrawRectangleRec(ir, is_cur ? Color{ 30,50,100,255 } : hov ? Color{ 25,30,55,255 } : Color{ 0,0,0,0 });
        DrawText(tex_files[i].c_str(), fm_x + 10, iy + 5, 10,
            is_cur ? Color{ 150,200,255,255 } : Color{ 180,185,210,220 });
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            load_tex_file(tex_files[i], sel);
            show_file_manager = false;
        }
    }
    if (tex_files.empty())
        DrawText("(carpeta vacia)", fm_x + 10, list_y + 8, 10, { 80,85,120,180 });
    EndScissorMode();

    // Botón "Nuevo .tex"
    Rectangle new_r = { (float)(fm_x + 8),(float)(fm_y + fm_h - 22),(float)(fm_w - 16), 18.0f };
    bool new_hov = CheckCollisionPointRec(mouse, new_r);
    DrawRectangleRec(new_r, new_hov ? Color{ 30,70,40,255 } : Color{ 20,45,28,255 });
    DrawRectangleLinesEx(new_r, 1.0f, { 50,120,60,200 });
    DrawText("Nuevo .tex para este nodo", (int)new_r.x + 10, (int)new_r.y + 3, 10, WHITE);
    if (new_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sel) {
        std::string safe = sel->code;
        for (char& c : safe)
            if (c == '/' || c == '\\' || c == ':' || c == ' ') c = '_';
        edit.tex_file = safe + ".tex";
        edit.body_buf[0] = '\0';
        edit.body_dirty = false;
        show_file_manager = false;
        files_stale = true;
    }
}

// ── draw_resource_list ────────────────────────────────────────────────────────

void EntryEditor::draw_resource_list(MathNode* sel, int lx, int lw,
    int ph, int py, int& y, Vector2 mouse)
{
    DrawText("RECURSOS (en memoria)", lx, y, 11, { 100,130,100,220 }); y += 18;
    int to_remove = -1;
    for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
        auto& res = sel->resources[i];
        std::string line = "[" + res.kind + "] " + res.title;
        if ((int)line.size() > 54) line = line.substr(0, 53) + "...";
        DrawRectangle(lx, y, lw - 26, 20, { 18,22,18,255 });
        DrawText(line.c_str(), lx + 4, y + 4, 10, { 170,200,170,210 });
        Rectangle del_r = { (float)(lx + lw - 22),(float)y, 20.0f, 20.0f };
        bool del_hov = CheckCollisionPointRec(mouse, del_r);
        DrawRectangleRec(del_r, del_hov ? Color{ 160,40,40,255 } : Color{ 40,20,20,200 });
        DrawText("x", (int)del_r.x + 6, (int)del_r.y + 4, 11, WHITE);
        if (del_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) to_remove = i;
        y += 24;
    }
    if (to_remove >= 0) sel->resources.erase(sel->resources.begin() + to_remove);
}

// ── draw_add_resource_form ────────────────────────────────────────────────────

void EntryEditor::draw_add_resource_form(MathNode* sel, int lx, int lw,
    int ph, int py, int& y, Vector2 mouse)
{
    if (y + 70 >= py + ph) return;
    int& aid = state.toolbar.active_field;
    DrawText("+ Nuevo recurso:", lx, y, 10, { 90,130,90,200 }); y += 14;

    draw_text_field(edit.new_kind, 32, lx, y, 60, 18, 10, aid, F_KIND, mouse);
    draw_text_field(edit.new_title, 128, lx + 66, y, lw / 2 - 70, 18, 10, aid, F_TITLE, mouse);
    y += 22;
    const int add_bw = 54;
    draw_text_field(edit.new_content, 256, lx, y, lw - add_bw - 4, 18, 10, aid, F_CONTENT, mouse);

    Rectangle add_r = { (float)(lx + lw - add_bw),(float)y,(float)add_bw, 20.0f };
    bool add_hov = CheckCollisionPointRec(mouse, add_r);
    DrawRectangleRec(add_r, add_hov ? Color{ 40,100,50,255 } : Color{ 28,60,34,255 });
    DrawRectangleLinesEx(add_r, 1.0f, { 60,140,70,200 });
    DrawText("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);
    if (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && strlen(edit.new_title) > 0) {
        Resource r; r.kind = edit.new_kind; r.title = edit.new_title; r.content = edit.new_content;
        sel->resources.push_back(r);
        edit.new_title[0] = '\0'; edit.new_content[0] = '\0';
    }
}

// ── draw ──────────────────────────────────────────────────────────────────────

void EntryEditor::draw(Vector2 mouse) {
    if (!state.toolbar.editor_open) return;

    const int pw = 540, ph = 560;
    const int px = SW() - pw - 10, py = TOOLBAR_H;

    // ── Resolver nodo ────────────────────────────────────────────────────────
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code) sel = cur;
    }

    // ── Sync: al cambiar nodo, leer body desde disco ─────────────────────────
    if (sel && sel->code != edit.last_code) {
        std::string body, fname;
        auto it = entries_index.find(sel->code);
        if (it != entries_index.end()) {
            fname = it->second;
            std::ifstream f(full_tex_path(fname));
            if (f.is_open()) body = std::string((std::istreambuf_iterator<char>(f)), {});
        }
        edit.sync(sel, body, fname);
        body_scroll = 0;
        files_stale = true;
        body_active = false;
    }

    // ── Frame principal ───────────────────────────────────────────────────────
    if (draw_window_frame(px, py, pw, ph,
        "EDITOR DE ENTRADAS", { 130,220,130,255 }, { 80,120,80,255 }, mouse))
    {
        state.toolbar.editor_open = false;
        state.toolbar.active_tab = ToolbarTab::None;
        show_file_manager = false;
        return;
    }

    const int lx = px + 14, lw = pw - 28;
    int y = py + 38;

    if (!sel) {
        DrawText("Selecciona una burbuja para editar.", lx, y + 10, 12, { 120,120,160,200 });
        return;
    }

    // Cabecera del nodo
    std::string ninfo = sel->code + "  |  " + sel->label;
    if ((int)ninfo.size() > 62) ninfo = ninfo.substr(0, 61) + "...";
    DrawText("Nodo:", lx, y, 11, { 90,100,140,200 });
    DrawText(ninfo.c_str(), lx + 46, y, 11, { 160,200,160,230 });
    y += 20;
    draw_h_line(lx, y, lx + lw); y += 10;

    draw_node_fields(sel, lx, lw, y, mouse);
    draw_h_line(lx, y, lx + lw); y += 8;
    draw_body_section(sel, lx, lw, py, ph, y, mouse);
    draw_h_line(lx, y, lx + lw); y += 8;
    draw_resource_list(sel, lx, lw, ph, py, y, mouse);
    draw_add_resource_form(sel, lx, lw, ph, py, y, mouse);

    // File manager encima de todo (se dibuja al final)
    draw_file_manager(sel, px, py, mouse);
}

void draw_entry_editor(AppState& state, Vector2 mouse) {
    EntryEditor panel(state);
    panel.draw(mouse);
}