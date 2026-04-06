#pragma once
// ── entry_editor.hpp ──────────────────────────────────────────────────────────
// Panel flotante para editar un MathNode seleccionado.
//
// Secciones (todas colapsables, con Guardar individual):
//   · BASIC INFO       → note, texture_key, msc_refs  → <basename>.json
//   · CUERPO LATEX     → body_buf                     → <basename>.tex + .json
//   · REFERENCIAS      → cross_refs                   → <basename>.json
//   · RECURSOS         → sel->resources               → <basename>.json
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

    // ── Índice persistente: code → basename.tex ───────────────────────────────
    std::unordered_map<std::string, std::string> entries_index;

    // ── File manager ──────────────────────────────────────────────────────────
    std::vector<std::string> tex_files;
    float file_list_scroll  = 0.f;
    bool  show_file_manager = false;
    bool  files_stale       = true;

    // ── Textarea ──────────────────────────────────────────────────────────────
    float body_scroll = 0.f;
    bool  body_active = false;

    // ── Scroll maestro del panel ──────────────────────────────────────────────
    // Permite desplazar todo el contenido del editor cuando el cuerpo LaTeX
    // crece y empuja las secciones inferiores fuera de la ventana.
    float panel_scroll = 0.f;

    // ── IO helpers ────────────────────────────────────────────────────────────
    void load_index();
    void save_tex_file(MathNode* sel);   ///< Escribe body_buf al disco.
    void save_json(MathNode* sel);       ///< Serializa todas las secciones JSON.

public:
    explicit EntryEditor(AppState& s);
    void draw(Vector2 mouse) override;
};

void draw_entry_editor(AppState& state, Vector2 mouse);
