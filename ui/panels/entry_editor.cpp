#include "entry_editor.hpp"
#include "../editor/editor_io.hpp"
#include "../editor/editor_sections.hpp"
#include "../core/font_manager.hpp"
#include "../constants.hpp"

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
    // Necesitamos acceso a entries_index y a files_stale, que son miembros de
    // instancia. Usamos el truco de guardar un puntero a la instancia en el
    // estado de la toolbar (campo reutilizable). Como alternativa limpia se
    // puede convertir el callback en std::function<> y capturar [this].
    // Aquí optamos por std::function para no contaminar AppState.
    //
    // NOTA: la firma del callback es la de editor_sections; el puntero a la
    // instancia se captura en draw() con un lambda local.
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
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code) sel = cur;
    }

    // ── Sync: al cambiar nodo, leer body desde disco ──────────────────────────
    if (sel && sel->code != edit.last_code) {
        std::string body, fname;
        auto it = entries_index.find(sel->code);
        if (it != entries_index.end()) {
            fname = it->second;
            body  = editor_io::read_tex(state, fname);
        }
        edit.sync(sel, body, fname);
        body_scroll  = 0;
        files_stale  = true;
        body_active  = false;
    }

    // ── Frame principal ───────────────────────────────────────────────────────
    if (draw_window_frame(540, 560,
        "EDITOR DE ENTRADAS", {130,220,130,255}, {80,120,80,255}, mouse))
    {
        state.toolbar.editor_open = false;
        state.toolbar.active_tab  = ToolbarTab::None;
        show_file_manager = false;
        return;
    }

    const int pw = 540, ph = 560;
    const int lx = pos_x + 14, lw = pw - 28;
    int y = pos_y + 38;

    if (!sel) {
        DrawTextF("Selecciona una burbuja para editar.",
                  lx, y + 10, 12, {120,120,160,200});
        return;
    }

    // ── Contenido del panel ───────────────────────────────────────────────────
    editor_sections::draw_node_header(sel, lx, lw, y);

    editor_sections::draw_node_fields(sel, edit, state, lx, lw, y, mouse);

    DrawLine(lx, y, lx + lw, y, {30,30,50,200}); y += 8;

    // Lambda de guardado (captura this para acceder a entries_index y files_stale)
    auto save_fn = [&](MathNode* s, EditState& e, AppState& /*st*/, bool& /*fm*/) {
        save_tex_file(s);
        (void)e;
    };

    // draw_body_section espera un puntero a función; usamos un wrapper estático
    // con estado capturado a través de una variable de función guardada en el frame.
    // La forma más idiomática en C++14/17 sin std::function es pasar el lambda
    // como template. Como editor_sections usa un puntero a función puro, podemos
    // delegar a un segundo callback que capture el contexto usando una variable
    // estática thread_local (safe en single-thread / Raylib).
    // 
    // Alternativa más limpia: cambiar la firma de draw_body_section a
    // std::function<> (ver nota en editor_sections.hpp).
    // Por ahora usamos la variable estática de frame:
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

    DrawLine(lx, y, lx + lw, y, {30,30,50,200}); y += 8;

    editor_sections::draw_resource_list(sel, lx, lw, pos_y, ph, y, mouse);
    editor_sections::draw_add_resource_form(sel, edit, state, lx, lw, pos_y, ph, y, mouse);

    // ── File manager (siempre encima, al final) ───────────────────────────────
    // Refrescar lista si está desactualizada y el file manager se va a mostrar
    if (show_file_manager && files_stale) {
        tex_files   = editor_io::list_tex_files(state);
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
