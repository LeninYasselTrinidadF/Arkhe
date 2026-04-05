#include "editor_sections_internal.hpp"
#include "data/editor/editor_io.hpp"

namespace editor_sections {

// ─────────────────────────────────────────────────────────────────────────────
// draw_file_manager
// ─────────────────────────────────────────────────────────────────────────────

void draw_file_manager(MathNode* sel, EditState& edit,
    AppState& state,
    int panel_x, int panel_y,
    Vector2 mouse,
    bool& show_file_manager,
    std::vector<std::string>& tex_files,
    float& file_list_scroll,
    bool& files_stale,
    std::unordered_map<std::string, std::string>& entries_index)
{
    if (!show_file_manager) return;

    const int fm_w = 260, fm_h = 230;
    const int fm_x = std::max(10, panel_x - fm_w - 6);
    const int fm_y = panel_y + 30;

    // Sombra + fondo
    DrawRectangle(fm_x + 3, fm_y + 3, fm_w, fm_h, { 0,0,0,100 });
    DrawRectangle(fm_x, fm_y, fm_w, fm_h, { 12,12,22,252 });
    DrawRectangleLinesEx({ (float)fm_x,(float)fm_y,(float)fm_w,(float)fm_h },
        1.0f, { 60,80,160,220 });

    // Header
    DrawRectangle(fm_x, fm_y, fm_w, 26, { 18,22,42,255 });
    DrawTextF("Archivos .tex", fm_x + 10, fm_y + 7, 11, { 140,170,255,255 });

    // Botón refrescar
    Rectangle ref_r = { (float)(fm_x + fm_w - 60),(float)(fm_y + 4), 52.0f, 18.0f };
    int  ref_idx = kbnav_toolbar_register(ToolbarNavKind::Button, ref_r, "Refrescar");
    bool ref_hov = CheckCollisionPointRec(mouse, ref_r);
    DrawRectangleRec(ref_r, ref_hov ? RL{ 40,60,120,255 } : RL{ 25,30,55,255 });
    DrawRectangleLinesEx(ref_r, 1.0f, { 50,80,160,200 });
    DrawTextF("Refrescar", (int)ref_r.x + 3, (int)ref_r.y + 3, 9, WHITE);
    if ((ref_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        || kbnav_toolbar_is_activated(ref_idx))
        tex_files = editor_io::list_tex_files(state);

    // Lista scrolleable
    const int item_h    = 22;
    const int list_y    = fm_y + 30;
    const int visible_h = fm_h - 56;

    Rectangle list_r = { (float)fm_x,(float)list_y,(float)fm_w,(float)visible_h };
    if (CheckCollisionPointRec(mouse, list_r))
        file_list_scroll -= GetMouseWheelMove() * item_h * 2;

    int max_scr = std::max(0, (int)tex_files.size() * item_h - visible_h);
    file_list_scroll = std::clamp(file_list_scroll, 0.0f, (float)max_scr);

    BeginScissorMode((int)list_r.x, (int)list_r.y,
        (int)list_r.width, (int)list_r.height);
    for (int i = 0; i < (int)tex_files.size(); i++) {
        int  iy    = list_y + i * item_h - (int)file_list_scroll;
        if (iy + item_h <= list_y || iy >= list_y + visible_h) continue;

        bool is_cur = (tex_files[i] == edit.tex_file);
        Rectangle ir = { (float)fm_x,(float)iy,(float)fm_w,(float)item_h };

        int  fi_idx   = kbnav_toolbar_register(ToolbarNavKind::FileItem, ir,
                                               tex_files[i].c_str());
        bool hov      = CheckCollisionPointRec(mouse, ir);
        bool kb_sel   = kbnav_toolbar_is_focused(fi_idx);
        bool activated = kbnav_toolbar_is_activated(fi_idx);

        DrawRectangleRec(ir,
            is_cur            ? RL{ 30, 50,100,255 }
            : (hov || kb_sel) ? RL{ 25, 30, 55,255 }
                              : RL{  0,  0,  0,  0 });
        DrawTextF(tex_files[i].c_str(), fm_x + 10, iy + 5, 10,
            is_cur ? RL{ 150,200,255,255 } : RL{ 180,185,210,220 });

        if ((hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || activated) {
            std::string body = editor_io::read_tex(state, tex_files[i]);
            if (body.size() > 8191) body.resize(8191);
            strncpy(edit.body_buf, body.c_str(), 8191);
            edit.body_buf[8191] = '\0';
            edit.tex_file       = tex_files[i];
            edit.body_dirty     = false;
            if (sel) {
                entries_index[sel->code] = tex_files[i];
                editor_io::save_index(state, entries_index);
            }
            show_file_manager = false;
            files_stale       = true;
        }
    }
    if (tex_files.empty())
        DrawTextF("(carpeta vacia)", fm_x + 10, list_y + 8, 10, { 80,85,120,180 });
    EndScissorMode();

    // Botón «Nuevo .tex»
    Rectangle new_r = { (float)(fm_x + 8),(float)(fm_y + fm_h - 22),
                        (float)(fm_w - 16), 18.0f };
    int  new_idx = kbnav_toolbar_register(ToolbarNavKind::Button, new_r, "Nuevo .tex");
    bool new_hov = CheckCollisionPointRec(mouse, new_r);
    bool new_kb  = kbnav_toolbar_is_focused(new_idx);
    DrawRectangleRec(new_r, (new_hov || new_kb) ? RL{ 30,70,40,255 } : RL{ 20,45,28,255 });
    DrawRectangleLinesEx(new_r, 1.0f, { 50,120,60,200 });
    DrawTextF("Nuevo .tex para este nodo", (int)new_r.x + 10, (int)new_r.y + 3, 10, WHITE);
    if (((new_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
         || kbnav_toolbar_is_activated(new_idx)) && sel)
    {
        edit.tex_file     = editor_io::safe_filename(sel->code);
        edit.body_buf[0]  = '\0';
        edit.body_dirty   = false;
        show_file_manager = false;
        files_stale       = true;
    }
}

} // namespace editor_sections
