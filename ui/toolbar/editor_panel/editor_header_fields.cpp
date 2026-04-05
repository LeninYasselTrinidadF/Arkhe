#include "editor_sections_internal.hpp"

namespace editor_sections {

// ─────────────────────────────────────────────────────────────────────────────
// draw_node_header
// ─────────────────────────────────────────────────────────────────────────────

void draw_node_header(MathNode* sel, int lx, int lw, int& y)
{
    std::string ninfo = sel->code + "  |  " + sel->label;
    if ((int)ninfo.size() > 62) ninfo = ninfo.substr(0, 61) + "...";

    DrawTextF("Nodo:", lx, y, 11, { 90, 100, 140, 200 });
    DrawTextF(ninfo.c_str(), lx + 46, y, 11, { 160, 200, 160, 230 });
    y += 20;

    DrawLine(lx, y, lx + lw, y, { 30, 30, 50, 200 });
    y += 10;
}

// ─────────────────────────────────────────────────────────────────────────────
// draw_node_fields
// ─────────────────────────────────────────────────────────────────────────────

void draw_node_fields(MathNode* sel, EditState& edit,
    AppState& state,
    int lx, int lw, int& y, Vector2 mouse)
{
    int& aid = state.toolbar.active_field;

    // ── Nota ─────────────────────────────────────────────────────────────────
    {
        Rectangle fr = { (float)lx, (float)(y + LABELED_FIELD_OFFSET),
                         (float)lw, (float)LABELED_FIELD_H };
        kbnav_toolbar_register(ToolbarNavKind::TextField, fr, "Nota", F_NOTE);
    }
    PanelWidget::draw_labeled_field(
        "Nota / descripcion:", edit.note_buf, 512,
        lx, y, lw, 11, aid, F_NOTE, mouse);
    sel->note = edit.note_buf;
    y += 38;

    // ── Texture key ───────────────────────────────────────────────────────────
    {
        Rectangle fr = { (float)lx, (float)(y + LABELED_FIELD_OFFSET),
                         (float)lw, (float)LABELED_FIELD_H };
        kbnav_toolbar_register(ToolbarNavKind::TextField, fr, "Imagen", F_TEX);
    }
    PanelWidget::draw_labeled_field(
        "Clave de imagen (texture_key):", edit.tex_buf, 128,
        lx, y, lw, 11, aid, F_TEX, mouse);
    sel->texture_key = edit.tex_buf;
    y += 38;

    // ── MSC refs ──────────────────────────────────────────────────────────────
    {
        Rectangle fr = { (float)lx, (float)(y + LABELED_FIELD_OFFSET),
                         (float)lw, (float)LABELED_FIELD_H };
        kbnav_toolbar_register(ToolbarNavKind::TextField, fr, "MSC refs", F_MSC);
    }
    PanelWidget::draw_labeled_field(
        "MSC refs (coma separados):", edit.msc_buf, 512,
        lx, y, lw, 11, aid, F_MSC, mouse);

    sel->msc_refs.clear();
    std::string ms(edit.msc_buf);
    size_t pos;
    while ((pos = ms.find(',')) != std::string::npos) {
        if (!ms.substr(0, pos).empty())
            sel->msc_refs.push_back(ms.substr(0, pos));
        ms = ms.substr(pos + 1);
    }
    if (!ms.empty()) sel->msc_refs.push_back(ms);
    y += 38;
}

} // namespace editor_sections

// Implementación de draw_basic_info_section reutilizando draw_node_fields.
namespace editor_sections {

void draw_basic_info_section(MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int& y, Vector2 mouse, SaveFn on_save)
{
    edit.update_dirty_flags(sel);

    bool save_clicked = false;
    if (!draw_section_bar("BASIC INFO", edit.sec_basic_open,
                          lx, lw, y, mouse,
                          edit.basic_info_dirty, true, &save_clicked))
        return;

    draw_node_fields(sel, edit, state, lx, lw, y, mouse);

    if (save_clicked && on_save) on_save(sel, edit, state);
}

} // namespace editor_sections
