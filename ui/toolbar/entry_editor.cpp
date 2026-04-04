#include "ui/toolbar/entry_editor.hpp"
#include "data/editor/editor_io.hpp"
#include "ui/toolbar/editor_panel/editor_sections.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"

#include <fstream>
#include <cstring>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

EntryEditor::EntryEditor(AppState& s) : PanelWidget(s) {
    pos_x = -1;           // -1 → posicionar en el primer draw
    pos_y = TOOLBAR_H;
    load_index();
}

// ─────────────────────────────────────────────────────────────────────────────
// IO helpers
// ─────────────────────────────────────────────────────────────────────────────

void EntryEditor::load_index() {
    editor_io::load_index(state, entries_index);
}

void EntryEditor::save_tex_file(MathNode* sel) {
    if (!sel) return;
    if (edit.tex_file.empty())
        edit.tex_file = editor_io::safe_filename(sel->code);

    editor_io::write_tex(state, edit.tex_file, edit.body_buf);
    edit.body_dirty = false;

    entries_index[sel->code] = edit.tex_file;
    editor_io::save_index(state, entries_index);
    files_stale = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Callback estático para draw_body_section
// ─────────────────────────────────────────────────────────────────────────────

void EntryEditor::on_save_callback(MathNode* sel, EditState& edit,
    AppState& state, bool& /*show_fm*/)
{
    (void)sel; (void)edit; (void)state;
    // El cuerpo real está en el lambda capturado en draw().
}

// ─────────────────────────────────────────────────────────────────────────────
// draw
// ─────────────────────────────────────────────────────────────────────────────

void EntryEditor::draw(Vector2 mouse) {
    if (!state.toolbar.editor_open) return;

    // ── Primera apertura: posicionar a la derecha ─────────────────────────────
    if (pos_x == -1) {
        pos_x = std::max(0, SW() - 540 - 10);
        pos_y = TOOLBAR_H;
    }

    // ── Resolver nodo seleccionado ────────────────────────────────────────────
    // Prioridad:
    //   1. Hijo de cur cuyo code == selected_code  (clic en burbuja hija)
    //   2. cur mismo si su code == selected_code   (clic en burbuja central)
    //   3. cur mismo si selected_code está vacío   (push limpió la selección;
    //      el usuario navegó dentro de un nodo → editar ese nodo directamente)
    // El check != "ROOT" evita exponer el nodo raíz invisible.
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code)
            sel = cur;
        if (!sel && cur->code != "ROOT")
            sel = cur;
    }

    // ── Sync: al cambiar nodo, leer body desde disco ──────────────────────────
    if (sel && sel->code != edit.last_code) {
        std::string body, fname;
        auto it = entries_index.find(sel->code);
        if (it != entries_index.end()) {
            fname = it->second;
            body = editor_io::read_tex(state, fname);
        }
        edit.sync(sel, body, fname);
        body_scroll = 0;
        files_stale = true;
        body_active = false;
    }

    // ── Frame principal ───────────────────────────────────────────────────────
    if (draw_window_frame(540, 560,
        "EDITOR DE ENTRADAS", { 130,220,130,255 }, { 80,120,80,255 }, mouse))
    {
        state.toolbar.editor_open = false;
        state.toolbar.active_tab = ToolbarTab::None;
        show_file_manager = false;
        return;
    }

    const int pw = cur_pw, ph = cur_ph;
    const int lx = pos_x + 14, lw = pw - 28;
    int y = pos_y + 38;

    if (!sel) {
        DrawTextF("Selecciona una burbuja para editar.",
            lx, y + 10, 12, { 120,120,160,200 });
        return;
    }

    // ── Contenido del panel ───────────────────────────────────────────────────
    editor_sections::draw_node_header(sel, lx, lw, y);

    editor_sections::draw_node_fields(sel, edit, state, lx, lw, y, mouse);

    DrawLine(lx, y, lx + lw, y, { 30,30,50,200 }); y += 8;

    static EntryEditor* s_self = nullptr;
    s_self = this;
    editor_sections::draw_body_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse,
        body_scroll, body_active, show_file_manager,
        [](MathNode* s, EditState& e, AppState& st, bool& fm) {
            if (s_self) s_self->save_tex_file(s);
            (void)e; (void)st; (void)fm;
        }
    );

    DrawLine(lx, y, lx + lw, y, { 30,30,50,200 }); y += 8;

    editor_sections::draw_resource_list(sel, lx, lw, pos_y, ph, y, mouse);
    editor_sections::draw_add_resource_form(sel, edit, state, lx, lw, pos_y, ph, y, mouse);

    // ── File manager (siempre encima, al final) ───────────────────────────────
    if (show_file_manager && files_stale) {
        tex_files = editor_io::list_tex_files(state);
        files_stale = false;
    }

    editor_sections::draw_file_manager(
        sel, edit, state,
        pos_x, pos_y, mouse,
        show_file_manager, tex_files,
        file_list_scroll, files_stale,
        entries_index
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Wrapper de compatibilidad
// ─────────────────────────────────────────────────────────────────────────────

void draw_entry_editor(AppState& state, Vector2 mouse) {
    EntryEditor panel(state);
    panel.draw(mouse);
}