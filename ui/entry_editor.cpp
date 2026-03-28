#include "entry_editor.hpp"
#include "constants.hpp"
#include <cstring>

// ── EditState ─────────────────────────────────────────────────────────────────

void EntryEditor::EditState::sync(MathNode* sel) {
    if (!sel || sel->code == last_code) return;
    last_code = sel->code;

    strncpy(note_buf, sel->note.c_str(),        511); note_buf[511] = '\0';
    strncpy(tex_buf,  sel->texture_key.c_str(), 127); tex_buf[127]  = '\0';

    std::string joined;
    for (int i = 0; i < (int)sel->msc_refs.size(); i++) {
        if (i > 0) joined += ',';
        joined += sel->msc_refs[i];
    }
    strncpy(msc_buf, joined.c_str(), 511); msc_buf[511] = '\0';

    new_title[0]   = '\0';
    new_content[0] = '\0';
}

// ── Campos principales del nodo ───────────────────────────────────────────────

void EntryEditor::draw_node_fields(MathNode* sel, int lx, int lw, int& y, Vector2 mouse) {
    int& aid = state.toolbar.active_field;

    draw_labeled_field("Nota / descripcion:",          edit.note_buf, 512, lx, y, lw, 11, aid, F_NOTE, mouse); sel->note        = edit.note_buf; y += 38;
    draw_labeled_field("Clave de imagen (texture_key):", edit.tex_buf, 128, lx, y, lw, 11, aid, F_TEX,  mouse); sel->texture_key = edit.tex_buf;  y += 38;
    draw_labeled_field("MSC refs (separadas por coma):", edit.msc_buf, 512, lx, y, lw, 11, aid, F_MSC,  mouse);

    // Parsear msc_buf → vector
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

// ── Lista de recursos existentes ──────────────────────────────────────────────

void EntryEditor::draw_resource_list(MathNode* sel, int lx, int lw,
    int /*pw*/, int ph, int py, int& y, Vector2 mouse)
{
    DrawText("RECURSOS (en memoria)", lx, y, 11, { 100, 130, 100, 220 }); y += 18;

    int to_remove = -1;
    for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
        auto& res = sel->resources[i];
        std::string line = "[" + res.kind + "] " + res.title;
        if ((int)line.size() > 54) line = line.substr(0, 53) + "...";

        DrawRectangle(lx, y, lw - 26, 20, { 18, 22, 18, 255 });
        DrawText(line.c_str(), lx + 4, y + 4, 10, { 170, 200, 170, 210 });

        Rectangle del_r = { (float)(lx + lw - 22), (float)y, 20.0f, 20.0f };
        bool del_hov = CheckCollisionPointRec(mouse, del_r);
        DrawRectangleRec(del_r, del_hov ? Color{ 160, 40, 40, 255 } : Color{ 40, 20, 20, 200 });
        DrawText("x", (int)del_r.x + 6, (int)del_r.y + 4, 11, WHITE);
        if (del_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) to_remove = i;

        y += 24;
    }
    if (to_remove >= 0) sel->resources.erase(sel->resources.begin() + to_remove);
}

// ── Formulario de nuevo recurso ───────────────────────────────────────────────

void EntryEditor::draw_add_resource_form(MathNode* sel, int lx, int lw,
    int /*pw*/, int ph, int py, int& y, Vector2 mouse)
{
    if (y + 90 >= py + ph) return;

    int& aid = state.toolbar.active_field;
    DrawText("+ Nuevo recurso:", lx, y, 10, { 90, 130, 90, 200 }); y += 14;

    draw_text_field(edit.new_kind,    32,  lx,       y, 60,          18, 10, aid, F_KIND,    mouse);
    draw_text_field(edit.new_title,  128,  lx + 66,  y, lw / 2 - 70, 18, 10, aid, F_TITLE,   mouse);
    y += 22;

    const int add_bw = 54;
    draw_text_field(edit.new_content, 256, lx, y, lw - add_bw - 4,  18, 10, aid, F_CONTENT, mouse);

    Rectangle add_r = { (float)(lx + lw - add_bw), (float)y, (float)add_bw, 20.0f };
    bool add_hov = CheckCollisionPointRec(mouse, add_r);
    DrawRectangleRec(add_r, add_hov ? Color{ 40, 100, 50, 255 } : Color{ 28, 60, 34, 255 });
    DrawRectangleLinesEx(add_r, 1.0f, { 60, 140, 70, 200 });
    DrawText("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);

    if (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && strlen(edit.new_title) > 0) {
        Resource r;
        r.kind    = edit.new_kind;
        r.title   = edit.new_title;
        r.content = edit.new_content;
        sel->resources.push_back(r);
        edit.new_title[0]   = '\0';
        edit.new_content[0] = '\0';
    }
}

// ── draw ──────────────────────────────────────────────────────────────────────

void EntryEditor::draw(Vector2 mouse) {
    if (!state.toolbar.editor_open) return;

    const int pw = 520, ph = 460;
    const int px = SW() - pw - 10, py = TOOLBAR_H;

    if (draw_window_frame(px, py, pw, ph,
            "EDITOR DE ENTRADAS", { 130, 220, 130, 255 },
            { 80, 120, 80, 255 }, mouse))
    {
        state.toolbar.editor_open = false;
        state.toolbar.active_tab  = ToolbarTab::None;
        return;
    }

    // Resolver nodo seleccionado
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code) sel = cur;
    }

    const int lx = px + 14, lw = pw - 28;
    int y = py + 38;

    if (!sel) {
        DrawText("Selecciona una burbuja para editar sus datos.", lx, y + 10, 12, { 120, 120, 160, 200 });
        return;
    }

    // Cabecera del nodo
    std::string node_info = sel->code + "  |  " + sel->label;
    if ((int)node_info.size() > 60) node_info = node_info.substr(0, 59) + "\xe2\x80\xa6";
    DrawText("Nodo:", lx, y, 11, { 90, 100, 140, 200 });
    DrawText(node_info.c_str(), lx + 46, y, 11, { 160, 200, 160, 230 });
    y += 20;
    draw_h_line(lx, y, lx + lw, { 35, 45, 35, 200 }); y += 10;

    edit.sync(sel);
    draw_node_fields(sel, lx, lw, y, mouse);

    draw_h_line(lx, y, lx + lw, { 35, 45, 35, 200 }); y += 8;
    draw_resource_list(sel, lx, lw, pw, ph, py, y, mouse);
    draw_add_resource_form(sel, lx, lw, pw, ph, py, y, mouse);
}

void draw_entry_editor(AppState& state, Vector2 mouse) {
    EntryEditor panel(state);
    panel.draw(mouse);
}
