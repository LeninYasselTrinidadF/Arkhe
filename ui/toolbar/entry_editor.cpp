#include "ui/toolbar/entry_editor.hpp"
#include "data/editor/editor_io.hpp"
#include "ui/toolbar/editor_panel/editor_sections.hpp"
#include "ui/key_controls/kbnav_toolbar/kbnav_toolbar.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"

#include <fstream>
#include <cstring>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

EntryEditor::EntryEditor(AppState& s) : PanelWidget(s) {
    pos_x = -1;
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

void EntryEditor::save_json(MathNode* sel) {
    if (!sel) return;
    editor_io::save_node_json(state, sel, edit);
    edit.commit_snapshots(sel);
}

// ─────────────────────────────────────────────────────────────────────────────
// draw
// ─────────────────────────────────────────────────────────────────────────────

void EntryEditor::draw(Vector2 mouse) {
    if (!state.toolbar.editor_open) return;

    kbnav_toolbar_begin_frame();

    if (pos_x == -1) {
        pos_x = std::max(0, SW() - 540 - 10);
        pos_y = TOOLBAR_H;
    }

    // ── Resolver nodo seleccionado ────────────────────────────────────────────
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code) sel = cur;
        if (!sel && cur->code != "ROOT")              sel = cur;
    }

    // ── Sync al cambiar nodo ──────────────────────────────────────────────────
    if (sel && sel->code != edit.last_code) {
        // Cargar .tex
        std::string body, fname;
        auto it = entries_index.find(sel->code);
        if (it != entries_index.end()) {
            fname = it->second;
            body  = editor_io::read_tex(state, fname);
        }
        // Cargar JSON del nodo (actualiza sel->note/resources y edit.cross_refs)
        editor_io::load_node_json(state, sel, edit);

        edit.sync(sel, body, fname, edit.cross_refs, sel->resources.size());
        body_scroll = 0;
        files_stale = true;
        body_active = false;
    }

    // ── Frame del panel ───────────────────────────────────────────────────────
    if (draw_window_frame(540, 620,
        "EDITOR DE ENTRADAS", { 130, 220, 130, 255 }, { 80, 120, 80, 255 }, mouse))
    {
        state.toolbar.editor_open = false;
        state.toolbar.active_tab  = ToolbarTab::None;
        show_file_manager = false;
        return;
    }

    const int pw = cur_pw, ph = cur_ph;
    const int lx = pos_x + 14, lw = pw - 28;
    int y = pos_y + 38;

    if (!sel) {
        DrawTextF("Selecciona una burbuja para editar.",
            lx, y + 10, 12, { 120, 120, 160, 200 });
        return;
    }

    // ── Punteros estáticos para callbacks sin captura ─────────────────────────
    static EntryEditor* s_self = nullptr;
    s_self = this;

    // Callback JSON (guarda todas las secciones JSON)
    editor_sections::SaveFn json_save =
        [](MathNode* s, EditState& /*e*/, AppState& /*st*/) {
            if (s_self) s_self->save_json(s);
        };

    // Callback .tex (guarda el body_buf al disco)
    auto tex_save = [](MathNode* /*s*/, EditState& /*e*/, AppState& /*st*/, bool& /*fm*/) {
        if (s_self) s_self->save_tex_file(nullptr);  // sel se ignora; usa estado interno
    };
    // Versión correcta: captura sel mediante s_self
    static MathNode* s_sel = nullptr;
    s_sel = sel;
    void (*tex_save_fn)(MathNode*, EditState&, AppState&, bool&) =
        [](MathNode* s, EditState& /*e*/, AppState& /*st*/, bool& /*fm*/) {
            if (s_self) s_self->save_tex_file(s_sel);
        };

    // ── Cabecera del nodo (siempre visible) ───────────────────────────────────
    editor_sections::draw_node_header(sel, lx, lw, y);

    // ── Sección 1: Basic Info ─────────────────────────────────────────────────
    editor_sections::draw_basic_info_section(
        sel, edit, state, lx, lw, y, mouse, json_save);

    // ── Sección 2: Cuerpo LaTeX ───────────────────────────────────────────────
    editor_sections::draw_body_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse,
        body_scroll, body_active, show_file_manager,
        json_save, tex_save_fn);

    // ── Sección 3: Referencias Cruzadas ───────────────────────────────────────
    // hint_codes: códigos de hijos del nodo actual para autocompletar
    std::vector<std::string> hint_codes;
    if (cur)
        for (auto& ch : cur->children)
            hint_codes.push_back(ch->code);

    editor_sections::draw_cross_refs_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse,
        hint_codes, json_save);

    // ── Sección 4: Recursos ───────────────────────────────────────────────────
    editor_sections::draw_resources_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse, json_save);

    // ── File manager ──────────────────────────────────────────────────────────
    if (show_file_manager && files_stale) {
        tex_files   = editor_io::list_tex_files(state);
        files_stale = false;
    }
    editor_sections::draw_file_manager(
        sel, edit, state, pos_x, pos_y, mouse,
        show_file_manager, tex_files,
        file_list_scroll, files_stale, entries_index);

    // ── Procesar teclado ──────────────────────────────────────────────────────
    kbnav_toolbar_handle(state, body_active);

    {
        int fi = kbnav_toolbar_focused_idx();
        if (body_active && fi >= 0
            && kbnav_toolbar_focused_kind() != ToolbarNavKind::Textarea)
        {
            body_active = false;
            if (state.toolbar.active_field == -99)
                state.toolbar.active_field = -1;
        }
    }

    kbnav_toolbar_draw();
}

// ─────────────────────────────────────────────────────────────────────────────
// Wrapper de compatibilidad
// ─────────────────────────────────────────────────────────────────────────────

void draw_entry_editor(AppState& state, Vector2 mouse) {
    EntryEditor panel(state);
    panel.draw(mouse);
}
