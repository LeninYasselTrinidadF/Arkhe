// ── docs_panel.cpp ───────────────────────────────────────────────────────────
// Panel de documentación paginada.
//
// Unity entry point: incluye helpers y páginas directamente.
// Sub-archivos (NO compilar por separado):
//   docs_draw_helpers.cpp   — primitivos de dibujo comunes
//   docs_page_overview.cpp  — Página 1: Visión general
//   docs_page_navigation.cpp    — Página 2: Navegación y atajos
//   docs_page_entry_editor.cpp  — Página 3: Editor de Entradas
//   docs_page_search.cpp        — Página 4: Búsqueda
//   docs_page_ubicaciones.cpp   — Página 5: Ubicaciones y generadores
//   docs_page_data_formats.cpp  — Página 6: Formatos de datos
//   docs_page_architecture.cpp  — Página 7: Arquitectura
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/toolbar/docs_panel.hpp"
#include "ui/toolbar/docs_panel/docs_page.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// ── Helpers y páginas (unity includes) ───────────────────────────────────────
// unity includes (from ui/toolbar/docs_panel/)
#include "ui/toolbar/docs_panel/docs_draw_helpers.cpp"
#include "ui/toolbar/docs_panel/docs_page_overview.cpp"
#include "ui/toolbar/docs_panel/docs_page_navigation.cpp"
#include "ui/toolbar/docs_panel/docs_page_entry_editor.cpp"
#include "ui/toolbar/docs_panel/docs_page_search.cpp"
#include "ui/toolbar/docs_panel/docs_page_ubicaciones.cpp"
#include "ui/toolbar/docs_panel/docs_page_data_formats.cpp"
#include "ui/toolbar/docs_panel/docs_page_architecture.cpp"

// ── Tabla de páginas ──────────────────────────────────────────────────────────

static const DocsPage s_pages[] = {
    { "General",      "G", docs_page_overview_draw       },
    { "Navegacion",   "N", docs_page_navigation_draw     },
    { "Editor",       "E", docs_page_entry_editor_draw   },
    { "Busqueda",     "B", docs_page_search_draw         },
    { "Ubicaciones",  "U", docs_page_ubicaciones_draw    },
    { "Datos",        "D", docs_page_data_formats_draw   },
    { "Arquitectura", "A", docs_page_architecture_draw   },
};
static constexpr int PAGE_COUNT =
    (int)(sizeof(s_pages) / sizeof(s_pages[0]));

// ── Constantes de layout ──────────────────────────────────────────────────────

static constexpr int TAB_H      = 26;   // altura de la barra de tabs
static constexpr int CONTENT_PAD = 10;  // padding lateral del contenido
static constexpr int SB_W        = 6;   // ancho de la scrollbar interna

// ── Helpers locales de la barra de tabs ──────────────────────────────────────

static void draw_tab_bar(int px, int py, int pw,
                          int current_page, int& out_page,
                          Vector2 mouse)
{
    const int tab_w = pw / PAGE_COUNT;
    const int bar_y = py;

    // Fondo de la barra
    DrawRectangle(px, bar_y, pw, TAB_H, { 12, 14, 26, 255 });
    DrawRectangle(px, bar_y + TAB_H - 1, pw, 1, docs_col::border_acc);

    for (int i = 0; i < PAGE_COUNT; i++) {
        int tx = px + i * tab_w;
        int tw = (i == PAGE_COUNT - 1) ? (pw - i * tab_w) : tab_w;
        Rectangle tr = { (float)tx, (float)bar_y, (float)tw, (float)TAB_H };

        bool sel = (i == current_page);
        bool hov = !sel && CheckCollisionPointRec(mouse, tr);

        Color bg = sel ? docs_col::bg_card
                 : hov ? Color{ 22, 28, 50, 255 }
                        : Color{ 12, 14, 26, 255 };
        DrawRectangleRec(tr, bg);

        // Borde derecho separador (excepto el último)
        if (i < PAGE_COUNT - 1)
            DrawRectangle(tx + tw - 1, bar_y, 1, TAB_H, docs_col::border);

        // Indicador inferior de tab activo
        if (sel)
            DrawRectangle(tx, bar_y + TAB_H - 2, tw, 2, docs_col::accent);

        // Etiqueta — en espacio reducido muestra solo el icono (1 letra)
        const char* lbl = tw >= 58 ? s_pages[i].title.c_str()
                                   : s_pages[i].icon.c_str();
        int fs = tw >= 58 ? 10 : 11;
        int lw_px = MeasureTextF(lbl, fs);
        Color tc = sel ? docs_col::accent
                 : hov ? docs_col::text_body
                        : docs_col::text_dim;
        DrawTextF(lbl, tx + (tw - lw_px) / 2,
                  bar_y + (TAB_H - fs) / 2, fs, tc);

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            out_page = i;
    }
}

// ── Scrollbar interna del contenido ──────────────────────────────────────────

static float draw_docs_scrollbar(
    int sb_x, int sb_y, int sb_h,
    float content_h, float scroll,
    Vector2 mouse, bool in_area,
    bool& dragging, float& drag_start_y, float& drag_start_off)
{
    const float max_off = std::max(0.f, content_h - (float)sb_h);
    if (max_off <= 0.f) return 0.f;

    DrawRectangle(sb_x, sb_y, SB_W, sb_h, { 16, 18, 32, 200 });

    float ratio   = std::clamp((float)sb_h / content_h, 0.05f, 1.f);
    int   thumb_h = std::max(20, (int)((float)sb_h * ratio));
    float t       = std::clamp(scroll / max_off, 0.f, 1.f);
    int   thumb_y = sb_y + (int)(t * (float)(sb_h - thumb_h));

    Rectangle thumb  = { (float)sb_x, (float)thumb_y, (float)SB_W, (float)thumb_h };
    bool hov_thumb   = CheckCollisionPointRec(mouse, thumb);

    DrawRectangleRec(thumb, (hov_thumb || dragging)
        ? docs_col::accent : docs_col::border_acc);

    if (hov_thumb && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging       = true;
        drag_start_y   = mouse.y;
        drag_start_off = scroll;
    }
    if (dragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float scale = max_off / (float)std::max(1, sb_h - thumb_h);
            scroll = std::clamp(drag_start_off + (mouse.y - drag_start_y) * scale,
                                0.f, max_off);
        } else {
            dragging = false;
        }
    }
    if (in_area) {
        float wh = GetMouseWheelMove();
        if (wh != 0.f)
            scroll = std::clamp(scroll - wh * 40.f, 0.f, max_off);
    }
    return scroll;
}

// ── DocsPanel::draw ───────────────────────────────────────────────────────────

void DocsPanel::draw(Vector2 mouse) {
    if (!state.toolbar.docs_open) return;

    // ── Marco del panel (drag, resize, botón de cierre) ───────────────────────
    if (draw_window_frame(PW, PH,
            "DOCUMENTACION", docs_col::accent,
            docs_col::border_acc, mouse))
    {
        state.toolbar.docs_open  = false;
        state.toolbar.active_tab = ToolbarTab::None;
        return;
    }

    const int pw   = cur_pw;
    const int ph   = cur_ph;
    const int px   = pos_x;
    const int py   = pos_y;

    // Fondo propio del panel (independiente del tema global)
    DrawRectangle(px, py, pw, ph, docs_col::bg);

    // ── Barra de pestañas ────────────────────────────────────────────────────
    const int tab_bar_y = py + 30;   // bajo el título del frame
    int new_page = current_page;
    draw_tab_bar(px, tab_bar_y, pw, current_page, new_page, mouse);
    if (new_page != current_page) {
        current_page = new_page;
        page_scroll  = 0.f;
    }

    // ── Área de contenido ────────────────────────────────────────────────────
    const int content_x     = px + CONTENT_PAD;
    const int content_w     = pw - CONTENT_PAD * 2 - SB_W - 4;
    const int content_top   = tab_bar_y + TAB_H + 6;
    const int content_bot   = py + ph - 8;
    const int content_h_vis = content_bot - content_top;

    if (content_h_vis <= 0) return;

    // Clip al área visible
    BeginScissorMode(px, content_top, pw, content_h_vis);

    // Medir la altura total del contenido renderizando "en seco" con scissor ya activo
    // Usamos y como cursor desplazado por el scroll actual
    int y_cursor = content_top - (int)page_scroll;

    float dt = GetFrameTime();
    const DocsPage& page = s_pages[current_page];
    if (page.draw) {
        // Llamamos el draw de la página; y_cursor se incrementa
        page.draw(content_x, content_w,
                  y_cursor, content_bot + (int)page_scroll + 2000,
                  mouse, page_scroll, dt);
    }

    EndScissorMode();

    // Altura total del contenido (aproximada por lo que dibujó)
    float content_total_h = (float)(y_cursor - (content_top - (int)page_scroll));

    // ── Scrollbar ────────────────────────────────────────────────────────────
    const int sb_x = px + pw - SB_W - 2;

    bool in_content = CheckCollisionPointRec(mouse,
        { (float)px, (float)content_top, (float)pw, (float)content_h_vis });

    page_scroll = draw_docs_scrollbar(
        sb_x, content_top, content_h_vis,
        content_total_h, page_scroll,
        mouse, in_content,
        sb_drag, sb_dy, sb_doff);
    page_scroll = std::clamp(page_scroll, 0.f,
        std::max(0.f, content_total_h - (float)content_h_vis));

    // ── Indicador de página en la esquina inferior ────────────────────────────
    char pg_info[32];
    snprintf(pg_info, sizeof(pg_info), "%d / %d", current_page + 1, PAGE_COUNT);
    int info_w = MeasureTextF(pg_info, 9);
    DrawTextF(pg_info, px + pw - info_w - SB_W - 8, content_bot - 12,
              9, docs_col::text_dim);

    // ── Atajo de teclado: ← → para cambiar página ────────────────────────────
    if (state.toolbar.docs_open && state.toolbar.active_field < 0) {
        if (IsKeyPressed(KEY_RIGHT) && current_page < PAGE_COUNT - 1) {
            current_page++;
            page_scroll = 0.f;
        }
        if (IsKeyPressed(KEY_LEFT) && current_page > 0) {
            current_page--;
            page_scroll = 0.f;
        }
    }
}

// ── Wrapper de compatibilidad ─────────────────────────────────────────────────

void draw_docs_panel(AppState& state, Vector2 mouse) {
    DocsPanel panel(state);
    panel.draw(mouse);
}
