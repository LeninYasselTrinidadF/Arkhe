#include "toolbar.hpp"
#include "constants.hpp"
#include "raylib.h"
#include <cstring>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// ── Helpers locales ───────────────────────────────────────────────────────────

static void draw_tb_separator(int x) {
    DrawLineEx({ (float)x, 6.0f }, { (float)x, (float)(TOOLBAR_H - 6) },
        1.0f, { 55, 55, 80, 200 });
}

// Boton de toolbar: devuelve true si se hizo click
static bool draw_tb_button(const char* label, int x, int y, int w, int h,
    bool active, Vector2 mouse)
{
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = CheckCollisionPointRec(mouse, r);
    Color bg = active  ? Color{ 50, 80, 140, 255 }
             : hov     ? Color{ 40, 40, 70,  255 }
                       : Color{ 22, 22, 36,  255 };
    Color border = active ? Color{ 100, 150, 255, 255 }
                 : hov    ? Color{  70,  70, 120, 255 }
                          : Color{  45,  45,  75, 200 };
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.0f, border);
    int tw = MeasureText(label, 12);
    DrawText(label, x + (w - tw) / 2, y + (h - 12) / 2, 12,
        hov || active ? WHITE : Color{ 180, 180, 210, 220 });
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ── draw_toolbar ──────────────────────────────────────────────────────────────

bool draw_toolbar(AppState& state, Vector2 mouse) {
    bool assets_changed = false;
    int  w = SW();

    // Fondo
    DrawRectangle(0, 0, w, TOOLBAR_H, { 16, 16, 28, 255 });
    DrawLineEx({ 0, (float)TOOLBAR_H }, { (float)w, (float)TOOLBAR_H },
        1.0f, { 45, 45, 72, 255 });

    int x = 8;
    const int BH = TOOLBAR_H - 8;   // altura botones
    const int BY = 4;                // y botones

    // ── Logo / nombre ─────────────────────────────────────────────────────────
    DrawText("ARKHE", x, BY + (BH - 16) / 2, 16, { 120, 160, 255, 255 });
    x += MeasureText("ARKHE", 16) + 16;
    draw_tb_separator(x); x += 10;

    // ── (a) Carpeta de assets ─────────────────────────────────────────────────
    DrawText("Assets:", x, BY + (BH - 11) / 2, 11, { 100, 100, 140, 255 });
    x += MeasureText("Assets:", 11) + 6;

    // Campo de texto editable para la ruta
    int field_w = 220;
    Rectangle field_r = { (float)x, (float)BY, (float)field_w, (float)BH };
    bool field_hov = CheckCollisionPointRec(mouse, field_r);

    Color field_border = state.toolbar.assets_edit_active
        ? Color{ 100, 150, 255, 255 }
        : field_hov
        ? Color{  70,  70, 120, 255 }
        : Color{  50,  50,  80, 200 };

    DrawRectangleRec(field_r, { 20, 20, 34, 255 });
    DrawRectangleLinesEx(field_r, 1.0f, field_border);

    // Truncar ruta si es larga (mostrar solo el final)
    std::string shown = state.toolbar.assets_path;
    while (MeasureText(shown.c_str(), 11) > field_w - 10 && shown.size() > 3)
        shown = ".." + shown.substr(shown.size() - (shown.size() / 2));
    DrawText(shown.c_str(), (int)field_r.x + 5, BY + (BH - 11) / 2, 11, WHITE);

    // Activar edicion al hacer click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state.toolbar.assets_edit_active = CheckCollisionPointRec(mouse, field_r);
    }

    // Teclado cuando el campo esta activo
    if (state.toolbar.assets_edit_active) {
        int key = GetCharPressed();
        while (key > 0) {
            int len = (int)strlen(state.toolbar.assets_path);
            if (key >= 32 && key <= 126 && len < 510) {
                state.toolbar.assets_path[len]     = (char)key;
                state.toolbar.assets_path[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(state.toolbar.assets_path);
            if (len > 0) state.toolbar.assets_path[len - 1] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER)) {
            state.toolbar.assets_edit_active = false;
            assets_changed = true;
            state.toolbar.assets_changed = true;
        }
        // Parpadeo del cursor
        if ((int)(GetTime() * 2) % 2 == 0) {
            int txt_w = MeasureText(state.toolbar.assets_path, 11);
            int cx = (int)field_r.x + 5 + txt_w;
            if (cx < (int)(field_r.x + field_w - 4))
                DrawLine(cx, BY + 4, cx, BY + BH - 4, WHITE);
        }
    }

    x += field_w + 4;

    // Boton "Cargar" — aplica la ruta actual
    if (draw_tb_button("Cargar", x, BY, 60, BH, false, mouse)) {
        assets_changed = true;
        state.toolbar.assets_changed = true;
        state.toolbar.assets_edit_active = false;
    }
    x += 64;
    draw_tb_separator(x); x += 10;

    // Indicador de texturas cargadas
    auto loaded = state.textures.loaded_keys();
    char tex_info[48];
    snprintf(tex_info, sizeof(tex_info), "%d img", (int)loaded.size());
    DrawText(tex_info, x, BY + (BH - 10) / 2, 10, { 80, 130, 80, 200 });
    x += MeasureText(tex_info, 10) + 12;
    draw_tb_separator(x); x += 10;

    // ── (b) Documentacion rapida ──────────────────────────────────────────────
    if (draw_tb_button("Docs", x, BY, 54, BH, state.toolbar.docs_open, mouse))
        state.toolbar.docs_open = !state.toolbar.docs_open;
    x += 58;

    // ── (c) Editor de entradas ────────────────────────────────────────────────
    if (draw_tb_button("Editor", x, BY, 62, BH, state.toolbar.editor_open, mouse))
        state.toolbar.editor_open = !state.toolbar.editor_open;
    x += 66;

    draw_tb_separator(x); x += 10;

    // Modo actual (informativo)
    const char* mode_str =
        state.mode == ViewMode::Mathlib  ? "Mathlib" :
        state.mode == ViewMode::Standard ? "Estandar" : "MSC2020";
    DrawText(mode_str, x, BY + (BH - 11) / 2, 11, { 120, 190, 120, 200 });

    return assets_changed;
}

// ── Panel de documentacion rapida ─────────────────────────────────────────────

void draw_docs_panel(AppState& state, Vector2 mouse) {
    if (!state.toolbar.docs_open) return;

    int pw = 480, ph = 340;
    int px = SW() / 2 - pw / 2;
    int py = TOOLBAR_H + 8;

    // Fondo con sombra
    DrawRectangle(px + 4, py + 4, pw, ph, { 0, 0, 0, 120 });
    DrawRectangle(px, py, pw, ph, { 14, 14, 24, 250 });
    DrawRectangleLinesEx({ (float)px, (float)py, (float)pw, (float)ph },
        1.5f, { 80, 100, 180, 255 });

    // Titulo
    DrawRectangle(px, py, pw, 32, { 22, 30, 58, 255 });
    DrawText("DOCUMENTACION RAPIDA", px + 12, py + 9, 13, { 160, 190, 255, 255 });

    // Boton cerrar
    Rectangle close_r = { (float)(px + pw - 28), (float)(py + 6), 22.0f, 22.0f };
    bool close_hov = CheckCollisionPointRec(mouse, close_r);
    DrawRectangleRec(close_r, close_hov ? Color{ 180, 60, 60, 255 } : Color{ 50, 40, 60, 255 });
    DrawText("x", (int)close_r.x + 7, (int)close_r.y + 5, 13, WHITE);
    if (close_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        state.toolbar.docs_open = false;

    int y = py + 44;
    int lx = px + 14;
    int lw = pw - 28;

    // Seccion: modos de vista
    DrawText("MODOS DE VISTA", lx, y, 11, { 100, 130, 200, 255 });
    y += 18;

    struct DocEntry { const char* key; const char* desc; Color kc; };
    DocEntry modes[] = {
        { "MSC2020",   "Classification Mathematics Subject (AMS). Navega areas de investigacion.", {140,200,255,255} },
        { "Mathlib",   "Libreria formal Lean4. Carpetas, archivos y declaraciones.",               {140,255,160,255} },
        { "Estandar",  "Temario academico personalizado. Conecta con MSC2020 y Mathlib.",          {255,200,120,255} },
    };
    for (auto& e : modes) {
        DrawRectangle(lx, y, lw, 28, { 18, 18, 32, 200 });
        DrawText(e.key, lx + 8, y + 7, 12, e.kc);
        int kw = MeasureText(e.key, 12);
        DrawText(e.desc, lx + kw + 14, y + 8, 10, { 160, 165, 185, 200 });
        y += 32;
    }
    y += 6;

    // Seccion: navegacion
    DrawText("NAVEGACION", lx, y, 11, { 100, 130, 200, 255 });
    y += 18;

    struct NavEntry { const char* key; const char* desc; };
    NavEntry nav[] = {
        { "Click burbuja",       "Navegar hacia adentro (si tiene hijos)" },
        { "Click + drag vacio",  "Mover camara (pan)" },
        { "Rueda raton",         "Zoom / mover divisor (cerca del divisor)" },
        { "ESC",                 "Volver un nivel atras" },
        { "Boton < Atras",       "Volver al nivel anterior" },
        { "Flechas del switcher","Cambiar entre MSC2020 / Mathlib / Estandar" },
    };
    for (auto& e : nav) {
        int kw = MeasureText(e.key, 11);
        DrawRectangle(lx, y, kw + 12, 18, { 30, 35, 55, 200 });
        DrawText(e.key, lx + 6, y + 3, 11, { 120, 190, 255, 220 });
        DrawText(e.desc, lx + kw + 18, y + 3, 11, { 160, 165, 185, 200 });
        y += 22;
    }
    y += 6;

    // Seccion: assets/graphics
    DrawText("SPRITES / IMAGENES", lx, y, 11, { 100, 130, 200, 255 });
    y += 16;
    DrawText("Pon imagenes en  assets/graphics/nombre.png", lx, y, 10, { 150, 160, 180, 220 });
    y += 14;
    DrawText("En el JSON del nodo:  \"texture_key\": \"nombre\"", lx, y, 10, { 150, 160, 180, 220 });
}

// ── Editor de entradas ────────────────────────────────────────────────────────
// Permite editar en tiempo real el note, resources y crossrefs del nodo
// seleccionado, y guarda los cambios en memoria (no persiste en disco todavia).

static void draw_editor_field(const char* label, char* buf, int buf_len,
    int x, int y, int w, int font,
    bool& active_flag, bool is_active,
    Vector2 mouse, const char* id_tag)
{
    (void)id_tag;
    int lh = font + 4;
    DrawText(label, x, y, 10, { 100, 110, 150, 220 });
    y += 14;
    Rectangle fr = { (float)x, (float)y, (float)w, (float)lh + 2 };
    bool fhov = CheckCollisionPointRec(mouse, fr);
    DrawRectangleRec(fr, { 18, 18, 32, 255 });
    DrawRectangleLinesEx(fr, 1.0f,
        is_active  ? Color{ 100, 150, 255, 255 }
        : fhov     ? Color{  70,  70, 120, 255 }
                   : Color{  45,  45,  75, 180 });
    DrawText(buf, x + 4, y + 3, font, WHITE);
    if (is_active && (int)(GetTime() * 2) % 2 == 0) {
        int cw = MeasureText(buf, font);
        DrawLine(x + 4 + cw, y + 3, x + 4 + cw, y + lh - 1, WHITE);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        active_flag = fhov;
    if (is_active) {
        int key = GetCharPressed();
        while (key > 0) {
            int len = (int)strlen(buf);
            if (key >= 32 && key <= 126 && len < buf_len - 1) {
                buf[len] = (char)key; buf[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(buf);
            if (len > 0) buf[len - 1] = '\0';
        }
    }
}

void draw_entry_editor(AppState& state, Vector2 mouse) {
    if (!state.toolbar.editor_open) return;

    // Buscar nodo seleccionado
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children)
            if (child->code == state.selected_code) { sel = child.get(); break; }
        if (!sel && cur->code == state.selected_code) sel = cur;
    }

    int pw = 520, ph = 460;
    int px = SW() - pw - 10;
    int py = TOOLBAR_H + 8;

    DrawRectangle(px + 4, py + 4, pw, ph, { 0, 0, 0, 120 });
    DrawRectangle(px, py, pw, ph, { 13, 13, 22, 252 });
    DrawRectangleLinesEx({ (float)px, (float)py, (float)pw, (float)ph },
        1.5f, { 80, 120, 80, 255 });

    // Titulo
    DrawRectangle(px, py, pw, 32, { 20, 40, 20, 255 });
    DrawText("EDITOR DE ENTRADAS", px + 12, py + 9, 13, { 140, 220, 140, 255 });

    Rectangle close_r = { (float)(px + pw - 28), (float)(py + 6), 22.0f, 22.0f };
    bool close_hov = CheckCollisionPointRec(mouse, close_r);
    DrawRectangleRec(close_r, close_hov ? Color{ 180, 60, 60, 255 } : Color{ 50, 40, 60, 255 });
    DrawText("x", (int)close_r.x + 7, (int)close_r.y + 5, 13, WHITE);
    if (close_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        state.toolbar.editor_open = false;

    int y  = py + 40;
    int lx = px + 14;
    int lw = pw - 28;

    if (!sel) {
        DrawText("Selecciona una burbuja para editar sus datos.", lx, y + 10, 12,
            { 120, 120, 160, 200 });
        return;
    }

    // ── Info del nodo ─────────────────────────────────────────────────────────
    DrawText("Nodo:", lx, y, 11, { 90, 100, 140, 200 });
    std::string node_info = sel->code + "  |  " + sel->label;
    if ((int)node_info.size() > 60) node_info = node_info.substr(0, 59) + "…";
    DrawText(node_info.c_str(), lx + 46, y, 11, { 160, 200, 160, 230 });
    y += 20;

    DrawLine(lx, y, lx + lw, y, { 35, 45, 35, 200 });
    y += 10;

    // ── Campo: Note ───────────────────────────────────────────────────────────
    static bool note_active = false;
    static char note_buf[512] = {};
    static std::string last_code;
    if (sel->code != last_code) {
        last_code = sel->code;
        strncpy(note_buf, sel->note.c_str(), 511);
        note_buf[511] = '\0';
    }
    draw_editor_field("Nota / descripcion:", note_buf, 512,
        lx, y, lw, 11, note_active, note_active, mouse, "note");
    if (last_code == sel->code) sel->note = note_buf;
    y += 38;

    // ── Campo: texture_key ────────────────────────────────────────────────────
    static bool tex_active = false;
    static char tex_buf[128] = {};
    static std::string last_tex_code;
    if (sel->code != last_tex_code) {
        last_tex_code = sel->code;
        strncpy(tex_buf, sel->texture_key.c_str(), 127);
        tex_buf[127] = '\0';
    }
    draw_editor_field("Clave de imagen (texture_key):", tex_buf, 128,
        lx, y, lw, 11, tex_active, tex_active, mouse, "tex");
    sel->texture_key = tex_buf;
    y += 38;

    // ── Referencias cruzadas MSC ──────────────────────────────────────────────
    DrawText("MSC refs (separadas por coma):", lx, y, 10, { 100, 110, 150, 220 });
    y += 14;

    static bool msc_active = false;
    static char msc_buf[512] = {};
    static std::string last_msc_code;
    if (sel->code != last_msc_code) {
        last_msc_code = sel->code;
        std::string joined;
        for (int i = 0; i < (int)sel->msc_refs.size(); i++) {
            if (i > 0) joined += ",";
            joined += sel->msc_refs[i];
        }
        strncpy(msc_buf, joined.c_str(), 511);
        msc_buf[511] = '\0';
    }
    {
        Rectangle fr = { (float)lx, (float)y, (float)lw, 18.0f };
        bool fhov = CheckCollisionPointRec(mouse, fr);
        DrawRectangleRec(fr, { 18, 18, 32, 255 });
        DrawRectangleLinesEx(fr, 1.0f,
            msc_active ? Color{ 100, 150, 255, 255 }
            : fhov     ? Color{  70,  70, 120, 255 }
                       : Color{  45,  45,  75, 180 });
        DrawText(msc_buf, lx + 4, y + 3, 11, WHITE);
        if (msc_active && (int)(GetTime() * 2) % 2 == 0) {
            int cw = MeasureText(msc_buf, 11);
            DrawLine(lx + 4 + cw, y + 3, lx + 4 + cw, y + 15, WHITE);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) msc_active = fhov;
        if (msc_active) {
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(msc_buf);
                if (key >= 32 && key <= 126 && len < 510) {
                    msc_buf[len] = (char)key; msc_buf[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = (int)strlen(msc_buf);
                if (len > 0) msc_buf[len - 1] = '\0';
            }
        }
        // Parsear y actualizar refs en caliente
        sel->msc_refs.clear();
        std::string s(msc_buf);
        size_t pos = 0;
        while ((pos = s.find(',')) != std::string::npos) {
            std::string tok = s.substr(0, pos);
            if (!tok.empty()) sel->msc_refs.push_back(tok);
            s = s.substr(pos + 1);
        }
        if (!s.empty()) sel->msc_refs.push_back(s);
    }
    y += 28;

    // ── Recursos ──────────────────────────────────────────────────────────────
    DrawLine(lx, y, lx + lw, y, { 35, 45, 35, 200 });
    y += 8;
    DrawText("RECURSOS (en memoria)", lx, y, 11, { 100, 130, 100, 220 });
    y += 18;

    // Listar recursos existentes con boton de eliminar
    int to_remove = -1;
    for (int i = 0; i < (int)sel->resources.size() && y + 22 < py + ph - 60; i++) {
        auto& res = sel->resources[i];
        std::string line = "[" + res.kind + "] " + res.title;
        if ((int)line.size() > 54) line = line.substr(0, 53) + "…";
        DrawRectangle(lx, y, lw - 26, 20, { 18, 22, 18, 255 });
        DrawText(line.c_str(), lx + 4, y + 4, 10, { 170, 200, 170, 210 });
        Rectangle del_r = { (float)(lx + lw - 22), (float)y, 20.0f, 20.0f };
        bool del_hov = CheckCollisionPointRec(mouse, del_r);
        DrawRectangleRec(del_r, del_hov ? Color{ 160, 40, 40, 255 } : Color{ 40, 20, 20, 200 });
        DrawText("x", (int)del_r.x + 6, (int)del_r.y + 4, 11, WHITE);
        if (del_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) to_remove = i;
        y += 24;
    }
    if (to_remove >= 0) sel->resources.erase(sel->resources.begin() + to_remove);

    // Agregar recurso nuevo
    static char new_kind[32]    = "link";
    static char new_title[128]  = {};
    static char new_content[256] = {};
    static bool nk_active = false, nt_active = false, nc_active = false;

    if (y + 90 < py + ph) {
        DrawText("+ Nuevo recurso:", lx, y, 10, { 90, 130, 90, 200 });
        y += 14;

        // kind
        {
            Rectangle fr = { (float)lx, (float)y, 60.0f, 18.0f };
            bool fhov = CheckCollisionPointRec(mouse, fr);
            DrawRectangleRec(fr, { 20, 28, 20, 255 });
            DrawRectangleLinesEx(fr, 1.0f, nk_active ? Color{ 100,150,255,255 } : Color{ 45,65,45,180 });
            DrawText(new_kind, lx + 4, y + 3, 10, WHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) nk_active = fhov;
            if (nk_active) {
                int key = GetCharPressed();
                while (key > 0) {
                    int len = (int)strlen(new_kind);
                    if (key >= 32 && key <= 126 && len < 30) {
                        new_kind[len] = (char)key; new_kind[len+1] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = (int)strlen(new_kind);
                    if (len > 0) new_kind[len-1] = '\0';
                }
            }
        }
        // title
        {
            int fx = lx + 66;
            Rectangle fr = { (float)fx, (float)y, (float)(lw / 2 - 70), 18.0f };
            bool fhov = CheckCollisionPointRec(mouse, fr);
            DrawRectangleRec(fr, { 18, 18, 32, 255 });
            DrawRectangleLinesEx(fr, 1.0f, nt_active ? Color{ 100,150,255,255 } : Color{ 45,45,75,180 });
            DrawText(new_title, fx + 4, y + 3, 10, WHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) nt_active = fhov;
            if (nt_active) {
                int key = GetCharPressed();
                while (key > 0) {
                    int len = (int)strlen(new_title);
                    if (key >= 32 && key <= 126 && len < 126) {
                        new_title[len] = (char)key; new_title[len+1] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = (int)strlen(new_title);
                    if (len > 0) new_title[len-1] = '\0';
                }
            }
        }
        y += 22;
        // content
        {
            Rectangle fr = { (float)lx, (float)y, (float)(lw - 60), 18.0f };
            bool fhov = CheckCollisionPointRec(mouse, fr);
            DrawRectangleRec(fr, { 18, 18, 32, 255 });
            DrawRectangleLinesEx(fr, 1.0f, nc_active ? Color{ 100,150,255,255 } : Color{ 45,45,75,180 });
            DrawText(new_content, lx + 4, y + 3, 10, WHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) nc_active = fhov;
            if (nc_active) {
                int key = GetCharPressed();
                while (key > 0) {
                    int len = (int)strlen(new_content);
                    if (key >= 32 && key <= 126 && len < 254) {
                        new_content[len] = (char)key; new_content[len+1] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    int len = (int)strlen(new_content);
                    if (len > 0) new_content[len-1] = '\0';
                }
            }
        }
        // Boton agregar
        {
            Rectangle add_r = { (float)(lx + lw - 56), (float)y, 54.0f, 20.0f };
            bool add_hov = CheckCollisionPointRec(mouse, add_r);
            DrawRectangleRec(add_r, add_hov ? Color{ 40,100,50,255 } : Color{ 28,60,34,255 });
            DrawRectangleLinesEx(add_r, 1.0f, { 60, 140, 70, 200 });
            DrawText("Agregar", (int)add_r.x + 4, (int)add_r.y + 4, 10, WHITE);
            if (add_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (strlen(new_title) > 0) {
                    Resource r;
                    r.kind    = new_kind;
                    r.title   = new_title;
                    r.content = new_content;
                    sel->resources.push_back(r);
                    new_title[0] = '\0';
                    new_content[0] = '\0';
                }
            }
        }
    }
}
