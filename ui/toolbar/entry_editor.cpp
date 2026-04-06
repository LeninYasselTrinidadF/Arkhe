#include "ui/toolbar/entry_editor.hpp"
#include "data/editor/editor_io.hpp"
#include "ui/toolbar/editor_panel/editor_sections.hpp"
#include "ui/key_controls/kbnav_toolbar/kbnav_toolbar.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
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
        std::string body, fname;
        auto it = entries_index.find(sel->code);
        if (it != entries_index.end()) {
            fname = it->second;
            body  = editor_io::read_tex(state, fname);
        }
        editor_io::load_node_json(state, sel, edit);

        edit.sync(sel, body, fname, edit.cross_refs, sel->resources.size());
        body_scroll  = 0;
        panel_scroll = 0;   // resetear scroll maestro al cambiar nodo
        files_stale  = true;
        body_active  = false;
    }

    // ── Resultado del picker de referencia cruzada ────────────────────────────
    // Se verifica DESPUÉS de sync para que new_xref_code no sea borrado por sync.
    if (!state.crossref_picker_result.empty()) {
        strncpy(edit.new_xref_code, state.crossref_picker_result.c_str(), 63);
        edit.new_xref_code[63] = '\0';
        state.crossref_picker_result.clear();
        edit.sec_xrefs_open = true;   // asegurar que la sección esté abierta
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

    editor_sections::SaveFn json_save =
        [](MathNode* s, EditState& /*e*/, AppState& /*st*/) {
            if (s_self) s_self->save_json(s);
        };

    static MathNode* s_sel = nullptr;
    s_sel = sel;
    void (*tex_save_fn)(MathNode*, EditState&, AppState&, bool&) =
        [](MathNode* s, EditState& /*e*/, AppState& /*st*/, bool& /*fm*/) {
            if (s_self) s_self->save_tex_file(s_sel);
        };

    // ── Cabecera sticky (no desplazada por scroll) ────────────────────────────
    editor_sections::draw_node_header(sel, lx, lw, y);
    const int content_start_y = y;

    // ── Sugerencias de referencias cruzadas (cache por nodo) ─────────────────
    // Se derivan de los cross_refs de nodos hermanos (hijos del mismo padre).
    // Solo se recalculan cuando cambia el nodo editado.
    static std::string s_sugg_for;
    static std::vector<editor_sections::CrossRefSuggestion> s_suggestions;

    if (sel->code != s_sugg_for) {
        s_sugg_for = sel->code;
        s_suggestions.clear();

        if (cur) {
            for (auto& child_ptr : cur->children) {
                MathNode* sibling = child_ptr.get();
                if (!sibling || sibling->code == sel->code) continue;

                // Leer cross_refs del hermano sin efectos secundarios
                auto sibling_refs = editor_io::load_cross_refs_only(state, sibling->code);

                for (auto& cr : sibling_refs) {
                    if (cr.target_code == sel->code) continue; // no auto-referencia

                    // Saltar si ya está en las refs del nodo actual
                    bool already = false;
                    for (auto& ex : edit.cross_refs)
                        if (ex.target_code == cr.target_code) { already = true; break; }
                    // Saltar duplicados dentro de suggestions
                    if (!already) {
                        for (auto& ex_s : s_suggestions)
                            if (ex_s.ref.target_code == cr.target_code) { already = true; break; }
                    }
                    if (!already)
                        s_suggestions.push_back({ cr, sibling->label, sibling->code });
                }
            }
        }
    }

    // ── Scroll maestro: manejar rueda sobre el panel ──────────────────────────
    {
        Rectangle panel_r = { (float)pos_x, (float)content_start_y,
                               (float)pw,   (float)(pos_y + ph - content_start_y) };
        if (CheckCollisionPointRec(mouse, panel_r)) {
            float wh = GetMouseWheelMove();
            if (wh != 0.f) panel_scroll -= wh * 35.f;
        }
    }

    // Aplicar desplazamiento ANTES de dibujar secciones
    y -= (int)panel_scroll;

    // ── Scissor: recortar contenido al área del panel ─────────────────────────
    const int scissor_h = pos_y + ph - content_start_y;
    BeginScissorMode(pos_x, content_start_y, pw, scissor_h);

    // ── hint_codes para autocompletar en refs cruzadas ────────────────────────
    std::vector<std::string> hint_codes;
    if (cur)
        for (auto& ch : cur->children)
            hint_codes.push_back(ch->code);

    // ── Secciones ─────────────────────────────────────────────────────────────
    editor_sections::draw_basic_info_section(
        sel, edit, state, lx, lw, y, mouse, json_save);

    editor_sections::draw_body_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse,
        body_scroll, body_active, show_file_manager,
        json_save, tex_save_fn);

    editor_sections::draw_cross_refs_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse,
        hint_codes, s_suggestions, json_save);

    editor_sections::draw_resources_section(
        sel, edit, state, lx, lw, pos_y, ph, y, mouse, json_save);

    const int content_end_y = y;  // y final tras todas las secciones

    EndScissorMode();

    // ── Scrollbar vertical ────────────────────────────────────────────────────
    {
        const int total_content_h = content_end_y + (int)panel_scroll - content_start_y;
        const float max_scroll = std::max(0.f, (float)(total_content_h - scissor_h));

        // Clamp ahora que sabemos el rango
        panel_scroll = std::clamp(panel_scroll, 0.f, max_scroll);

        if (max_scroll > 0.f) {
            const int sb_x = pos_x + pw - 9;
            const int sb_y = content_start_y;
            const int sb_h = scissor_h;

            // Track
            DrawRectangle(sb_x, sb_y, 7, sb_h, { 20, 22, 40, 180 });

            // Thumb
            float ratio   = (float)sb_h / (float)total_content_h;
            int   thumb_h = std::max(20, (int)(sb_h * ratio));
            float thumb_y = sb_y + (panel_scroll / max_scroll) * (float)(sb_h - thumb_h);
            DrawRectangle(sb_x + 1, (int)thumb_y, 5, thumb_h, { 90, 110, 200, 230 });
            DrawRectangleLinesEx({ (float)sb_x, (float)sb_y, 7.f, (float)sb_h },
                1.f, { 45, 55, 100, 120 });
        }
    }

    // ── File manager (no se desplaza, flota sobre el panel) ──────────────────
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
