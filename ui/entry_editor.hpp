#pragma once
#include "panel_widget.hpp"

class EntryEditor : public PanelWidget {
    // Buffers estáticos y estado de edición
    struct EditState {
        char note_buf[512]    = {};
        char tex_buf[128]     = {};
        char msc_buf[512]     = {};
        char new_kind[32]     = "link";
        char new_title[128]   = {};
        char new_content[256] = {};
        std::string last_code;

        void sync(MathNode* sel);
    };
    EditState edit;

    void draw_node_fields(MathNode* sel, int lx, int lw, int& y, Vector2 mouse);
    void draw_resource_list(MathNode* sel, int lx, int lw, int pw, int ph, int py, int& y, Vector2 mouse);
    void draw_add_resource_form(MathNode* sel, int lx, int lw, int pw, int ph, int py, int& y, Vector2 mouse);

    // IDs de campo (comparten active_field con toolbar)
    enum FieldId { F_NOTE=0, F_TEX=1, F_MSC=2, F_KIND=10, F_TITLE=11, F_CONTENT=12 };

public:
    explicit EntryEditor(AppState& s) : PanelWidget(s) {}
    void draw(Vector2 mouse) override;
};

void draw_entry_editor(AppState& state, Vector2 mouse);