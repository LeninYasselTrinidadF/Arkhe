#include "docs_panel.hpp"
#include "constants.hpp"

void DocsPanel::draw(Vector2 mouse) {
    if (!state.toolbar.docs_open) return;

    const int pw = 480, ph = 380;
    const int px = SW() / 2 - pw / 2, py = TOOLBAR_H;

    if (draw_window_frame(px, py, pw, ph,
            "DOCUMENTACION RAPIDA", { 140, 170, 255, 255 },
            { 80, 100, 180, 255 }, mouse))
    {
        state.toolbar.docs_open  = false;
        state.toolbar.active_tab = ToolbarTab::None;
        return;
    }

    const int lx = px + 14, lw = pw - 28;
    int y = py + 40;

    // ── Modos de vista ────────────────────────────────────────────────────
    DrawText("MODOS DE VISTA", lx, y, 11, { 100, 130, 200, 255 }); y += 18;

    struct ModeEntry { const char* key, *desc; Color kc; };
    ModeEntry modes[] = {
        { "MSC2020",  "Classification Mathematics Subject (AMS). Areas de investigacion.", { 140, 200, 255, 255 } },
        { "Mathlib",  "Libreria formal Lean4. Carpetas, archivos y declaraciones.",        { 140, 255, 160, 255 } },
        { "Estandar", "Temario academico personalizado. Conecta MSC2020 y Mathlib.",       { 255, 200, 120, 255 } },
    };
    for (auto& e : modes) {
        DrawRectangle(lx, y, lw, 28, { 18, 18, 32, 200 });
        DrawText(e.key, lx + 8, y + 7, 12, e.kc);
        DrawText(e.desc, lx + MeasureText(e.key, 12) + 14, y + 8, 10, { 160, 165, 185, 200 });
        y += 32;
    }
    y += 6;

    // ── Navegacion ────────────────────────────────────────────────────────
    DrawText("NAVEGACION", lx, y, 11, { 100, 130, 200, 255 }); y += 18;

    struct NavEntry { const char* key, *desc; };
    NavEntry nav[] = {
        { "Click burbuja",      "Navegar hacia adentro (si tiene hijos)" },
        { "Drag espacio vacio", "Mover camara (pan)" },
        { "Rueda raton",        "Zoom / mover divisor (cerca del divisor)" },
        { "ESC",                "Volver un nivel atras" },
        { "Flechas switcher",   "Cambiar MSC2020 / Mathlib / Estandar" },
    };
    for (auto& e : nav) {
        int kw = MeasureText(e.key, 11);
        DrawRectangle(lx, y, kw + 12, 18, { 30, 35, 55, 200 });
        DrawText(e.key,  lx + 6,       y + 3, 11, { 120, 190, 255, 220 });
        DrawText(e.desc, lx + kw + 18, y + 3, 11, { 160, 165, 185, 200 });
        y += 22;
    }
    y += 6;

    // ── Entradas LaTeX ────────────────────────────────────────────────────
    DrawText("ENTRADAS LATEX", lx, y, 11, { 100, 130, 200, 255 }); y += 16;
    DrawText("Coloca  <entries_path>/<code>.tex  con la descripcion del nodo.", lx, y, 10, { 150, 160, 180, 220 }); y += 14;
    DrawText("Ejemplo:  assets/entries/34A12.tex",                              lx, y, 10, { 120, 160, 130, 200 }); y += 14;
    DrawText("Configura las rutas en el panel Ubicaciones.",                    lx, y, 10, { 100, 110, 160, 200 });
}


void draw_docs_panel(AppState& state, Vector2 mouse) {
    DocsPanel panel(state);
    panel.draw(mouse);
}