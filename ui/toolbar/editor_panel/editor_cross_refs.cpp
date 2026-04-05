// ── editor_cross_refs.cpp ─────────────────────────────────────────────────────
// Implementa: draw_cross_refs_section
// ─────────────────────────────────────────────────────────────────────────────

#include "editor_sections_internal.hpp"

namespace editor_sections {

// ── Pill de relación ──────────────────────────────────────────────────────────
// Dibuja un botón de selección de tipo de relación.
// Retorna true si fue seleccionado.
static bool draw_relation_pill(
    const char* label, bool selected,
    int x, int y, Vector2 mouse)
{
    int w = MeasureTextF(label, 9) + 10;
    Rectangle r = { (float)x, (float)y, (float)w, 16.f };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color bg = selected  ? RL{ 50, 100, 160, 255 }
             : hov       ? RL{ 35,  60, 100, 255 }
                         : RL{ 22,  30,  55, 200 };
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.f, selected ? RL{ 80,160,255,220 } : RL{ 45,60,100,160 });
    DrawTextF(label, x + 5, y + 2, 9, selected ? WHITE : RL{ 160,175,220,200 });
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ─────────────────────────────────────────────────────────────────────────────
// draw_cross_refs_section
// ─────────────────────────────────────────────────────────────────────────────

void draw_cross_refs_section(
    MathNode* sel, EditState& edit, AppState& state,
    int lx, int lw, int py, int ph,
    int& y, Vector2 mouse,
    const std::vector<std::string>& hint_codes,
    SaveFn on_save)
{
    edit.update_dirty_flags(sel);

    bool save_clicked = false;
    if (!draw_section_bar("REFERENCIAS CRUZADAS", edit.sec_xrefs_open,
                          lx, lw, y, mouse,
                          edit.xrefs_dirty, true, &save_clicked))
        return;

    int& aid = state.toolbar.active_field;

    // ── Lista de refs existentes ──────────────────────────────────────────────
    int to_remove = -1;
    for (int i = 0; i < (int)edit.cross_refs.size() && y + 22 < py + ph - 80; i++) {
        auto& cr = edit.cross_refs[i];

        // Etiqueta de relación (pill pequeño, no interactivo aquí)
        const char* rel_lbl = xref_relation_label(cr.relation);
        int rw = MeasureTextF(rel_lbl, 9) + 10;
        DrawRectangle(lx, y, rw, 20, { 28, 40, 80, 200 });
        DrawRectangleLinesEx({ (float)lx,(float)y,(float)rw, 20.f }, 1.f, { 50,80,160,180 });
        DrawTextF(rel_lbl, lx + 5, y + 4, 9, { 130, 170, 240, 220 });

        // Código destino
        int cx = lx + rw + 6;
        int cw = lw - rw - 30;
        DrawRectangle(cx, y, cw, 20, { 18, 22, 40, 230 });
        DrawTextF(cr.target_code.c_str(), cx + 4, y + 4, 10, { 190, 210, 250, 220 });

        // Botón eliminar
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
        edit.cross_refs.erase(edit.cross_refs.begin() + to_remove);

    if (edit.cross_refs.empty()) {
        DrawTextF("(sin referencias)", lx + 4, y, 10, { 80, 90, 130, 160 });
        y += 16;
    }

    // ── Separador ─────────────────────────────────────────────────────────────
    DrawLine(lx, y + 2, lx + lw, y + 2, { 30, 35, 65, 150 });
    y += 8;

    // ── Formulario de añadir ──────────────────────────────────────────────────
    DrawTextF("+ Nueva ref cruzada:", lx, y, 10, { 90, 110, 170, 200 });
    y += 14;

    // Campo de código
    {
        Rectangle fr = { (float)lx, (float)y, (float)(lw - 4), 18.f };
        kbnav_toolbar_register(ToolbarNavKind::TextField, fr, "Codigo destino", F_XREF_CODE);
    }
    PanelWidget::draw_text_field(edit.new_xref_code, 63,
        lx, y, lw - 4, 18, 10, aid, F_XREF_CODE, mouse);
    y += 22;

    // ── Autocomplete ──────────────────────────────────────────────────────────
    // Muestra hasta 5 códigos que contengan el prefijo escrito.
    // El dropdown se dibuja encima del contenido siguiente (no empuja y).
    if (aid == F_XREF_CODE && edit.new_xref_code[0] != '\0') {
        std::string prefix = edit.new_xref_code;
        int hitem = 18, shown = 0, hy = y;
        for (auto& code : hint_codes) {
            if (shown >= 5) break;
            if (code.find(prefix) == std::string::npos) continue;
            Rectangle hr = { (float)lx, (float)hy, (float)(lw - 4), (float)hitem };
            bool hhov = CheckCollisionPointRec(mouse, hr);
            DrawRectangleRec(hr, hhov ? RL{ 40, 55, 100, 240 } : RL{ 25, 32, 60, 230 });
            DrawRectangleLinesEx(hr, 1.f, { 50, 70, 130, 200 });
            DrawTextF(code.c_str(), lx + 6, hy + 3, 10, { 180, 200, 255, 230 });
            if (hhov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                strncpy(edit.new_xref_code, code.c_str(), 63);
                edit.new_xref_code[63] = '\0';
                aid = -1;  // cerrar autocomplete
            }
            hy += hitem;
            shown++;
        }
    }

    // ── Pills de relación ─────────────────────────────────────────────────────
    {
        int px = lx;
        for (int i = 0; i < XREF_RELATION_COUNT; i++) {
            bool sel_rel = (strcmp(edit.new_xref_rel, XREF_RELATIONS[i]) == 0);
            const char* lbl = xref_relation_label(XREF_RELATIONS[i]);
            int pw = MeasureTextF(lbl, 9) + 10;
            if (px + pw > lx + lw - 4) { px = lx; y += 20; }
            if (draw_relation_pill(lbl, sel_rel, px, y, mouse)) {
                strncpy(edit.new_xref_rel, XREF_RELATIONS[i], 31);
            }
            px += pw + 4;
        }
        y += 22;
    }

    // ── Botón Agregar ─────────────────────────────────────────────────────────
    const int add_bw = 60;
    Rectangle add_r = { (float)(lx + lw - add_bw), (float)y, (float)add_bw, 20.f };
    int  add_idx = kbnav_toolbar_register(ToolbarNavKind::Button, add_r, "Agregar ref");
    bool add_hov = CheckCollisionPointRec(mouse, add_r);
    DrawRectangleRec(add_r,
        (add_hov || kbnav_toolbar_is_focused(add_idx))
            ? RL{ 40, 100, 50, 255 } : RL{ 28, 60, 34, 255 });
    DrawRectangleLinesEx(add_r, 1.f, { 60, 140, 70, 200 });
    DrawTextF("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);

    bool do_add = (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                || kbnav_toolbar_is_activated(add_idx);
    if (do_add && edit.new_xref_code[0] != '\0') {
        EditorCrossRef cr;
        cr.target_code = edit.new_xref_code;
        cr.relation    = edit.new_xref_rel;
        edit.cross_refs.push_back(cr);
        edit.new_xref_code[0] = '\0';
    }
    y += 26;

    // ── Guardar JSON ──────────────────────────────────────────────────────────
    if (save_clicked && on_save)
        on_save(sel, edit, state);
}

} // namespace editor_sections
