#include "ubicaciones_panel.hpp"
#include "constants.hpp"

void UbicacionesPanel::draw(Vector2 mouse) {
    if (!state.toolbar.ubicaciones_open) return;

    if (draw_window_frame(PX, PY, PW, PH,
            "UBICACIONES", { 120, 160, 255, 255 },
            { 50, 80, 160, 220 }, mouse))
    {
        state.toolbar.ubicaciones_open = false;
        return;
    }

    auto& tb = state.toolbar;
    const int lx = PX + 16, fw = PW - 80, fh = 22, spacing = 56;

    struct Row { const char* label; const char* hint; char* buf; int buf_len; int field_id; };
    Row rows[] = {
        { "Raiz de Assets",  "Carpeta base con JSON y sub-carpetas",    tb.assets_path,   512, 0 },
        { "Entradas (.tex)", "Carpeta con archivos .tex por nodo",       tb.entries_path,  512, 1 },
        { "Graficos",        "Carpeta con imagenes .png/.jpg",           tb.graphics_path, 512, 2 },
        { "pdflatex.exe",    "Ejecutable de pdflatex (TeXLive)",         tb.latex_path,    512, 3 },
        { "pdftoppm.exe",    "Ejecutable de pdftoppm (para PNG)",        tb.pdftoppm_path, 512, 4 },
    };

    int y = PY + 38;
    bool any_confirmed = false;

    for (auto& row : rows) {
        DrawText(row.label, lx, y + (fh - 11) / 2, 11, { 140, 150, 185, 230 });
        DrawText(row.hint,  lx, y + fh + 2,          9, { 80,  85,  120, 180 });

        any_confirmed |= draw_text_field(row.buf, row.buf_len,
            lx + 150, y, fw, fh, 11,
            tb.active_field, row.field_id, mouse);

        // Botón "..." (placeholder para dialogo nativo)
        // Ajustamos la posición X para que esté pegada al borde derecho del campo de texto
        Rectangle br = { (float)(lx + 150 + fw + 10), (float)y, 32.0f, (float)fh };
        bool bhov = CheckCollisionPointRec(mouse, br);

        // Mejora el feedback visual cambiando el color del borde si hay hover
        DrawRectangleRec(br, bhov ? Color{ 60, 70, 120, 255 } : Color{ 30, 30, 50, 255 });
        DrawRectangleLinesEx(br, 1.0f, bhov ? SKYBLUE : Color{ 50, 60, 110, 200 });
        DrawText("...", (int)br.x + 5, (int)br.y + (fh - 10) / 2, 10, { 160, 170, 210, 220 });

        y += spacing;
    }

    // Botón Aplicar
    const int apply_y = PY + PH - 44;
    draw_h_line(lx, apply_y, PX + PW - lx);
    Rectangle apply_r = { (float)(PX + PW - 110), (float)(apply_y + 10), 94.0f, 26.0f };
    bool ahov = CheckCollisionPointRec(mouse, apply_r);
    DrawRectangleRec(apply_r, ahov ? Color{ 40, 80, 180, 255 } : Color{ 28, 50, 120, 255 });
    DrawRectangleLinesEx(apply_r, 1.0f, { 70, 130, 255, 220 });
    DrawText("Aplicar", (int)apply_r.x + 14, (int)apply_r.y + 7, 12, WHITE);

    if ((ahov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || any_confirmed) {
        tb.assets_changed   = true;
        tb.ubicaciones_open = false;
        tb.active_field     = -1;
    }

    DrawText("Tip: presiona Enter en cualquier campo o haz click en Aplicar para recargar.",
        lx, PY + PH - 14, 9, { 70, 80, 130, 180 });
}

void draw_ubicaciones_panel(AppState& state, Vector2 mouse) {
    UbicacionesPanel panel(state);
    panel.draw(mouse);
}