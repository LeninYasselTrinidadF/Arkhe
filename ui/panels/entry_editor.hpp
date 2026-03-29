#pragma once
#include "../widgets/panel_widget.hpp"
#include "../constants.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class EntryEditor : public PanelWidget {

    // ── Estado de edición del nodo activo ─────────────────────────────────────
    struct EditState {
        char note_buf[512] = {};
        char tex_buf[128] = {};
        char msc_buf[512] = {};
        char body_buf[8192] = {};     // cuerpo LaTeX
        char new_kind[32] = "link";
        char new_title[128] = {};
        char new_content[256] = {};

        std::string last_code;
        std::string tex_file;           // filename (sin ruta) del .tex vinculado
        bool        body_dirty = false;

        void sync(MathNode* sel, const std::string& body, const std::string& fname);
    };
    EditState edit;

    // ── Índice persistente: code → filename.tex ───────────────────────────────
    std::unordered_map<std::string, std::string> entries_index;

    // ── File manager ──────────────────────────────────────────────────────────
    std::vector<std::string> tex_files;
    float file_list_scroll = 0.0f;
    bool  show_file_manager = false;
    bool  files_stale = true;

    // ── Textarea ──────────────────────────────────────────────────────────────
    float body_scroll = 0.0f;
    bool  body_active = false;

    // ── Helpers de ruta ───────────────────────────────────────────────────────
    std::string index_path()                        const;
    std::string full_tex_path(const std::string& f) const;

    // ── IO ────────────────────────────────────────────────────────────────────
    void load_entries_index();
    void save_entries_index();
    void refresh_file_list();
    void load_tex_file(const std::string& filename, MathNode* sel);
    void save_tex_file(MathNode* sel);

    // ── Secciones de dibujo ───────────────────────────────────────────────────
    void draw_node_fields(MathNode* sel, int lx, int lw, int& y, Vector2 mouse);
    void draw_body_section(MathNode* sel, int lx, int lw, int py, int ph, int& y, Vector2 mouse);
    void draw_file_manager(MathNode* sel, int px, int py, Vector2 mouse);
    void draw_resource_list(MathNode* sel, int lx, int lw, int ph, int py, int& y, Vector2 mouse);
    void draw_add_resource_form(MathNode* sel, int lx, int lw, int ph, int py, int& y, Vector2 mouse);

    enum FieldId { F_NOTE = 0, F_TEX = 1, F_MSC = 2, F_KIND = 10, F_TITLE = 11, F_CONTENT = 12 };

public:
    explicit EntryEditor(AppState& s) : PanelWidget(s) {
        // Posición inicial: derecha de la pantalla (se ajusta en draw con SW())
        pos_x = -1; pos_y = TOOLBAR_H;  // -1 = no inicializado, se calcula en primer draw
        load_entries_index();
    }
    void draw(Vector2 mouse) override;
};

void draw_entry_editor(AppState& state, Vector2 mouse);