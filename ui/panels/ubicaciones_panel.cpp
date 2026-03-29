#include "ubicaciones_panel.hpp"
#include "../constants.hpp"
#include "tinyfiledialogs.h"
#include <cstring>

// Abre selector de carpeta y copia resultado al buffer si el usuario confirmó
static void browse_folder(char* buf, int buf_len, const char* title) {
    const char* result = tinyfd_selectFolderDialog(title, buf);
    if (result) {
        strncpy(buf, result, buf_len - 1);
        buf[buf_len - 1] = '\0';
        // Asegura trailing slash
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] != '/' && buf[len - 1] != '\\') {
            if (len < buf_len - 2) { buf[len] = '/'; buf[len + 1] = '\0'; }
        }
    }
}

// Abre selector de archivo ejecutable
static void browse_file(char* buf, int buf_len, const char* title) {
    const char* filters[] = { "*.exe" };
    const char* result = tinyfd_openFileDialog(title, buf, 1, filters, "Ejecutable", 0);
    if (result) {
        strncpy(buf, result, buf_len - 1);
        buf[buf_len - 1] = '\0';
    }
}

void UbicacionesPanel::draw(Vector2 mouse) {
    if (!state.toolbar.ubicaciones_open) return;

    if (draw_window_frame(PW, PH,
        "UBICACIONES", { 120, 160, 255, 255 },
        { 50, 80, 160, 220 }, mouse))
    {
        state.toolbar.ubicaciones_open = false;
        return;
    }

    auto& tb = state.toolbar;
    const int lx = pos_x + 16;
    const int field_w = PW - 200;  // deja espacio para label + botón
    const int label_w = 130;
    const int btn_w = 36;
    const int fh = 22;
    const int spacing = 52;

    struct Row {
        const char* label;
        const char* title_dialog;
        char* buf;
        int         buf_len;
        int         field_id;
        bool        is_file;    // true → openFile, false → selectFolder
    };

    Row rows[] = {
        { "Raiz de Assets",   "Seleccionar carpeta de assets",   tb.assets_path,   512, 0, false },
        { "Entradas (.tex)",  "Seleccionar carpeta de entradas", tb.entries_path,  512, 1, false },
        { "Graficos",         "Seleccionar carpeta de graficos", tb.graphics_path, 512, 2, false },
        { "pdflatex.exe",     "Seleccionar pdflatex",           tb.latex_path,    512, 3, true  },
        { "pdftoppm.exe",     "Seleccionar pdftoppm",           tb.pdftoppm_path, 512, 4, true  },
    };

    int y = pos_y + 40;
    bool any_confirmed = false;

    for (auto& row : rows) {
        // Label
        DrawText(row.label, lx, y + (fh - 11) / 2, 11, { 140, 150, 185, 230 });

        // Campo de texto editable
        int fx = lx + label_w;
        any_confirmed |= draw_text_field(row.buf, row.buf_len,
            fx, y, field_w, fh, 11,
            tb.active_field, row.field_id, mouse);

        // Botón "..."
        Rectangle br = { (float)(fx + field_w + 6), (float)y, (float)btn_w, (float)fh };
        bool bhov = CheckCollisionPointRec(mouse, br);
        DrawRectangleRec(br, bhov ? Color{ 60, 80, 150, 255 } : Color{ 30, 35, 60, 255 });
        DrawRectangleLinesEx(br, 1.0f, bhov ? Color{ 100, 140, 255, 220 } : Color{ 50, 60, 110, 200 });
        DrawText("...", (int)br.x + 8, (int)br.y + (fh - 10) / 2, 10, { 160, 170, 210, 220 });

        if (bhov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // tinyfd bloquea el hilo, pero raylib sigue bien porque no hay BeginDrawing activo aquí
            if (row.is_file)
                browse_file(row.buf, row.buf_len, row.title_dialog);
            else
                browse_folder(row.buf, row.buf_len, row.title_dialog);
        }

        y += spacing;
    }

    // Botón Aplicar
    const int apply_y = pos_y + PH - 42;
    draw_h_line(lx, apply_y, pos_x + PW - lx);
    Rectangle apply_r = { (float)(pos_x + PW - 110), (float)(apply_y + 8), 94.0f, 26.0f };
    bool ahov = CheckCollisionPointRec(mouse, apply_r);
    DrawRectangleRec(apply_r, ahov ? Color{ 40, 80, 180, 255 } : Color{ 28, 50, 120, 255 });
    DrawRectangleLinesEx(apply_r, 1.0f, { 70, 130, 255, 220 });
    DrawText("Aplicar", (int)apply_r.x + 14, (int)apply_r.y + 7, 12, WHITE);

    if ((ahov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || any_confirmed) {
        tb.assets_changed = true;
        tb.ubicaciones_open = false;
        tb.active_field = -1;
    }

    DrawText("Tip: Enter en cualquier campo o click en Aplicar para recargar.",
        lx, pos_y + PH - 12, 9, { 70, 80, 130, 180 });
}

void draw_ubicaciones_panel(AppState& state, Vector2 mouse) {
    UbicacionesPanel panel(state);
    panel.draw(mouse);
}