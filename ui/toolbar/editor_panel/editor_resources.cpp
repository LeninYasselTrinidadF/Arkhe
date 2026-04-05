// ── editor_resources.cpp ─────────────────────────────────────────────────────
// Implementa: draw_resources_section (sección colapsable)
//             draw_resource_list     (helper interno)
//             draw_add_resource_form (helper interno)
// ─────────────────────────────────────────────────────────────────────────────

#include "editor_sections_internal.hpp"

namespace editor_sections {

// ─────────────────────────────────────────────────────────────────────────────
// draw_resource_list  (helper interno)
// ─────────────────────────────────────────────────────────────────────────────

static void draw_resource_list_impl(MathNode* sel,
    int lx, int lw, int py, int ph, int& y, Vector2 mouse)
{
    int to_remove = -1;
    for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
        auto& res = sel->resources[i];
        std::string line = "[" + res.kind + "] " + res.title;
        if ((int)line.size() > 54) line = line.substr(0, 53) + "...";

        DrawRectangle(lx, y, lw - 26, 20, { 18, 22, 18, 255 });
        DrawTextF(line.c_str(), lx + 4, y + 4, 10, { 170, 200, 170, 210 });

        Rectangle del_r = { (float)(lx + lw - 22), (float)y, 20.f, 20.f };
        int  del_idx = kbnav_toolbar_register(ToolbarNavKind::Button, del_r, "x");
        bool del_hov = CheckCollisionPointRec(mouse, del_r);
        DrawRectangleRec(del_r,
            (del_hov || kbnav_toolbar_is_focused(del_idx))
                ? RL{ 160, 40, 40, 255 } : RL{ 40, 20, 20, 200 });
        DrawTextF("x", (int)del_r.x + 6, (int)del_r.y + 4, 11, WHITE);
        if ((del_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            || kbnav_toolbar_is_activated(del_idx))
            to_remove = i;
        y += 24;
    }
    if (to_remove >= 0)
        sel->resources.erase(sel->resources.begin() + to_remove);
}

// ─────────────────────────────────────────────────────────────────────────────
// draw_add_resource_form  (helper interno)
// ─────────────────────────────────────────────────────────────────────────────

static void draw_add_resource_form_impl(MathNode* sel, EditState& edit,
    AppState& state, int lx, int lw, int py, int ph, int& y, Vector2 mouse)
{
    if (y + 70 >= py + ph) return;
    int& aid = state.toolbar.active_field;

    DrawTextF("+ Nuevo recurso:", lx, y, 10, { 90, 130, 90, 200 });
    y += 14;

    // Fila 1: kind + title
    {
        Rectangle kr = { (float)lx,       (float)y, 60.f,               18.f };
        Rectangle tr = { (float)(lx + 66),(float)y, (float)(lw/2 - 70), 18.f };
        kbnav_toolbar_register(ToolbarNavKind::TextField, kr, "Tipo",   F_KIND);
        kbnav_toolbar_register(ToolbarNavKind::TextField, tr, "Titulo", F_TITLE);
    }
    PanelWidget::draw_text_field(edit.new_kind,  32, lx,      y, 60,          18, 10, aid, F_KIND,  mouse);
    PanelWidget::draw_text_field(edit.new_title, 128, lx + 66, y, lw/2 - 70, 18, 10, aid, F_TITLE, mouse);
    y += 22;

    // Fila 2: content + Agregar
    const int add_bw = 54;
    {
        Rectangle cr = { (float)lx, (float)y, (float)(lw - add_bw - 4), 18.f };
        kbnav_toolbar_register(ToolbarNavKind::TextField, cr, "Contenido", F_CONTENT);
    }
    PanelWidget::draw_text_field(edit.new_content, 256,
        lx, y, lw - add_bw - 4, 18, 10, aid, F_CONTENT, mouse);

    Rectangle add_r  = { (float)(lx + lw - add_bw), (float)y, (float)add_bw, 20.f };
    int  add_idx = kbnav_toolbar_register(ToolbarNavKind::Button, add_r, "Agregar");
    bool add_hov = CheckCollisionPointRec(mouse, add_r);
    DrawRectangleRec(add_r,
        (add_hov || kbnav_toolbar_is_focused(add_idx))
            ? RL{ 40, 100, 50, 255 } : RL{ 28, 60, 34, 255 });
    DrawRectangleLinesEx(add_r, 1.f, { 60, 140, 70, 200 });
    DrawTextF("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);

    bool do_add = (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                || kbnav_toolbar_is_activated(add_idx);
    if (do_add && strlen(edit.new_title) > 0) {
        Resource r;
        r.kind    = edit.new_kind;
        r.title   = edit.new_title;
        r.content = edit.new_content;
        sel->resources.push_back(r);
        edit.new_title[0]   = '\0';
        edit.new_content[0] = '\0';
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// draw_resources_section  (pública)
// ─────────────────────────────────────────────────────────────────────────────

void draw_resources_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    SaveFn on_save)
{
    edit.update_dirty_flags(sel);

    bool save_clicked = false;
    if (!draw_section_bar("RECURSOS", edit.sec_res_open,
                          lx, lw, y, mouse,
                          edit.resources_dirty, true, &save_clicked))
        return;

    std::size_t before = sel->resources.size();

    DrawTextF("En memoria:", lx, y, 10, { 100, 130, 100, 200 });
    y += 16;

    draw_resource_list_impl(sel, lx, lw, py, ph, y, mouse);
    draw_add_resource_form_impl(sel, edit, state, lx, lw, py, ph, y, mouse);

    // Detectar cambio de tamaño para activar dirty
    if (sel->resources.size() != before)
        edit.resources_dirty = true;

    if (save_clicked && on_save)
        on_save(sel, edit, state);

    y += 4;
}

} // namespace editor_sections
