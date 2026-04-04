#pragma once
// ── entry_editor.hpp ──────────────────────────────────────────────────────────
// Panel flotante para editar un MathNode seleccionado.
//
// La lógica se distribuye así:
//   ui/editor/edit_state.hpp      → struct EditState (buffers + sync)
//   ui/editor/editor_io.hpp/.cpp  → IO de archivos .tex y del índice JSON
//   ui/editor/editor_sections.hpp/.cpp → secciones de dibujo reutilizables
//   ui/panels/entry_editor.hpp/.cpp    → orquestación (esta clase)
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/widgets/panel_widget.hpp"
#include "data/editor/edit_state.hpp"
#include "ui/constants.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class EntryEditor : public PanelWidget {

    // ── Estado de edición ─────────────────────────────────────────────────────
    EditState edit;

    // ── Índice persistente: code → filename.tex ───────────────────────────────
    std::unordered_map<std::string, std::string> entries_index;

    // ── File manager ──────────────────────────────────────────────────────────
    std::vector<std::string> tex_files;
    float file_list_scroll = 0.0f;
    bool  show_file_manager = false;
    bool  files_stale       = true;

    // ── Textarea ──────────────────────────────────────────────────────────────
    float body_scroll = 0.0f;
    bool  body_active = false;

    // ── IO helpers (delegan en editor_io) ─────────────────────────────────────
    void load_index();
    void save_tex_file(MathNode* sel);   ///< Escribe body_buf al disco y actualiza índice.

    // ── Callback de guardado para draw_body_section ───────────────────────────
    static void on_save_callback(MathNode* sel, EditState& edit,
                                 AppState& state, bool& show_fm);

public:
    explicit EntryEditor(AppState& s);
    void draw(Vector2 mouse) override;
};

void draw_entry_editor(AppState& state, Vector2 mouse);
