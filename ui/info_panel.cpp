#include "info_panel.hpp"
#include "raylib.h"
#include <algorithm>
#include <string>
#include <vector>

// ── Word-wrap simple ──────────────────────────────────────────────────────────
static int draw_wrapped_text(const char* text, int x, int y, int max_w,
    int font_size, Color color) {
    std::string s(text);
    int line_y = y;
    while (!s.empty()) {
        int chars = 1;
        while (chars < (int)s.size() &&
            MeasureText(s.substr(0, chars + 1).c_str(), font_size) < max_w)
            chars++;
        // No cortar palabras si es posible
        if (chars < (int)s.size() && s[chars] != ' ') {
            int space = (int)s.rfind(' ', chars);
            if (space > 0) chars = space;
        }
        DrawText(s.substr(0, chars).c_str(), x, line_y, font_size, color);
        s = s.substr(chars);
        if (!s.empty() && s[0] == ' ') s = s.substr(1);
        line_y += font_size + 4;
    }
    return line_y;
}

// ── Descripcion por defecto segun nivel del nodo ──────────────────────────────
static std::string default_description(const MathNode* node) {
    if (!node || node->code.empty()) return "";
    switch (node->level) {
    case NodeLevel::Macro:
        return "Gran area de las matematicas. Haz click en las sub-areas "
            "para explorar las secciones especificas que la componen.";
    case NodeLevel::Section:
        return "Seccion de la clasificacion MSC2020. Contiene sub-areas "
            "especializadas y temas de investigacion activa en Mathlib.";
    case NodeLevel::Subsection:
        return "Sub-area especializada. Los temas listados corresponden a "
            "entradas concretas de la clasificacion MSC2020 con "
            "codigos de referencia para busqueda en MathSciNet y zbMATH.";
    case NodeLevel::Topic:
        return "Tema especifico de investigacion matematica con "
            "representacion en Mathlib4.";
    default:
        return "Selecciona una burbuja para ver su informacion detallada.";
    }
}

// ── Chip de etiqueta ──────────────────────────────────────────────────────────
static void draw_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureText(text, 11);
    DrawRectangle(x, y, tw + 14, 20, bg);
    DrawRectangle(x, y, tw + 14, 20, { 255,255,255,12 });
    DrawText(text, x + 7, y + 5, 11, fg);
}

void draw_info_panel(AppState& state, Vector2 mouse) {
    int top = TOP_H();
    int w = SW();
    int vh = SH() - top;

    DrawRectangle(0, top, w, vh, { 10,10,18,255 });
    DrawLine(0, top, w, top, { 50,50,75,255 });

    // ── Cabecera fija ─────────────────────────────────────────────────────────
    constexpr int HEADER_H = 100;
    int px = 18;

    // Breadcrumb
    int tx = px;
    for (int i = 1; i < (int)state.nav_stack.size(); i++) {
        auto& n = state.nav_stack[i];
        std::string crumb = n->label;
        if ((int)crumb.size() > 22) crumb = crumb.substr(0, 21) + ".";
        Color cc = (i == (int)state.nav_stack.size() - 1)
            ? Color{ 120,190,255,255 } : Color{ 80,110,160,200 };
        DrawText(crumb.c_str(), tx, top + 8, 11, cc);
        tx += MeasureText(crumb.c_str(), 11);
        if (i < (int)state.nav_stack.size() - 1) {
            DrawText("  ›  ", tx, top + 8, 11, { 55,55,80,255 });
            tx += MeasureText("  ›  ", 11);
        }
        if (tx > w - 200) { DrawText("...", tx, top + 8, 11, { 70,70,100,255 }); break; }
    }

    if (!state.selected_code.empty()) {
        // Titulo grande
        std::string title = state.selected_label;
        if ((int)title.size() > 55) title = title.substr(0, 54) + "...";
        DrawText(title.c_str(), px, top + 24, 20, { 220, 225, 240, 255 });

        // Codigo como chip
        draw_chip(state.selected_code.c_str(),
            px, top + 52,
            { 30,60,100,220 }, { 100,190,255,255 });

        // Chip de nivel
        const char* level_str = "—";
        MathNode* cur = state.current();
        if (cur) for (auto& child : cur->children)
            if (child->code == state.selected_code) {
                switch (child->level) {
                case NodeLevel::Macro:      level_str = "Macro-area";  break;
                case NodeLevel::Section:    level_str = "Seccion";     break;
                case NodeLevel::Subsection: level_str = "Sub-area";    break;
                case NodeLevel::Topic:      level_str = "Tema";        break;
                default: break;
                }
                break;
            }
        int chip_w = MeasureText(state.selected_code.c_str(), 11) + 14 + 8;
        draw_chip(level_str, px + chip_w, top + 52, { 28,28,45,220 }, { 130,130,170,255 });
    }
    else {
        DrawText("Selecciona una burbuja", px, top + 28, 18, { 70,75,100,255 });
        DrawText("para ver su informacion detallada.",
            px, top + 52, 12, { 55,58,80,255 });
    }

    DrawLine(0, top + HEADER_H, w, top + HEADER_H, { 35,35,55,255 });

    // ── Zona scrollable ───────────────────────────────────────────────────────
    int scroll_top = top + HEADER_H;
    int scroll_h = vh - HEADER_H;

    if (mouse.y > scroll_top) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            state.resource_scroll -= wheel * 32.0f;
            if (state.resource_scroll < 0.0f) state.resource_scroll = 0.0f;
        }
    }

    MathNode* cur = state.current();
    MathNode* selected_node = nullptr;
    if (cur)
        for (auto& child : cur->children)
            if (child->code == state.selected_code) {
                selected_node = child.get(); break;
            }

    BeginScissorMode(0, scroll_top, w, scroll_h);

    int y = scroll_top + 14 - (int)state.resource_scroll;
    int col = 18;
    int text_w = w / 2 - 30;  // columna de texto (mitad izquierda)

    // ── Seccion: Descripcion ──────────────────────────────────────────────────
    DrawText("DESCRIPCION", col, y, 11, { 80,80,120,255 });
    y += 18;

    std::string desc = selected_node && !selected_node->note.empty()
        ? selected_node->note
        : default_description(selected_node);

    y = draw_wrapped_text(desc.c_str(), col, y, text_w, 13, { 190,195,215,255 });
    y += 18;

    DrawLine(col, y, w - col, y, { 30,30,50,200 });
    y += 14;

    // ── Seccion: Recursos ─────────────────────────────────────────────────────
    DrawText("RECURSOS", col, y, 11, { 80,80,120,255 });
    y += 18;

    const std::vector<Resource>* resources =
        selected_node ? &selected_node->resources : nullptr;
    bool has_resources = resources && !resources->empty();

    if (!has_resources) {
        // Placeholders con aspecto de tarjeta
        struct PlaceholderRes {
            const char* kind, * title, * desc, * tag;
            Color tag_col;
        } ph[] = {
            {"book",        "Introduction",        "Texto introductorio al area",
             "LIBRO",       {200,140,40,255}},
            {"link",        "MathSciNet",          "mathscinet.ams.org",
             "WEB",         {60,150,220,255}},
            {"explanation", "Nota tecnica",        "Descripcion formal del area",
             "NOTA",        {140,140,200,255}},
            {"link",        "zbMATH Open",         "zbmath.org",
             "WEB",         {60,150,220,255}},
            {"book",        "Survey article",      "Revision del estado del arte",
             "PAPER",       {180,100,200,255}},
            {"latex",       "Definicion formal",   "Formulacion en Lean/Mathlib",
             "LEAN",        {80,200,120,255}},
        };

        int card_w = (w - col * 2 - 10 * 5) / 3;  // 3 columnas
        int card_h = 68;
        for (int i = 0; i < 6; i++) {
            int ci = i % 3, ri = i / 3;
            int rx = col + ci * (card_w + 10);
            int ry = y + ri * (card_h + 10);

            // Fondo tarjeta
            DrawRectangle(rx, ry, card_w, card_h, { 17,17,30,255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, { 38,38,62,180 });

            // Tag en la esquina
            int tw2 = MeasureText(ph[i].tag, 9);
            DrawRectangle(rx + card_w - tw2 - 12, ry + 6, tw2 + 10, 16,
                { ph[i].tag_col.r,ph[i].tag_col.g,ph[i].tag_col.b,40 });
            DrawText(ph[i].tag, rx + card_w - tw2 - 7, ry + 8, 9, ph[i].tag_col);

            // Titulo
            DrawText(ph[i].title, rx + 10, ry + 10, 13, { 210,215,230,255 });
            // Descripcion
            DrawText(ph[i].desc, rx + 10, ry + 30, 11, { 110,115,145,200 });
            // Tipo icono
            const char* icon = ph[i].kind[0] == 'b' ? "[L]" :
                ph[i].kind[0] == 'l' && ph[i].kind[1] == 'i' ? "[W]" :
                ph[i].kind[0] == 'l' ? "[M]" : "[N]";
            DrawText(icon, rx + 10, ry + 50, 10, { 70,75,110,200 });
        }
        y += 2 * (card_h + 10) + 10;
        float max_s = (float)std::max(0, (y + (int)state.resource_scroll) - scroll_h - scroll_top + 40);
        if (state.resource_scroll > max_s) state.resource_scroll = max_s;

    }
    else {
        int card_w = (w - col * 2 - 10 * 2) / 3;
        int card_h = 68;
        int total = (int)resources->size();
        for (int i = 0; i < total; i++) {
            auto& res = (*resources)[i];
            int ci = i % 3, ri = i / 3;
            int rx = col + ci * (card_w + 10);
            int ry = y + ri * (card_h + 10);

            Color kc = { 80,80,130,255 };
            const char* tag = "RES";
            if (res.kind == "book") { kc = { 200,140,40,255 }; tag = "LIBRO"; }
            if (res.kind == "link") { kc = { 60,150,220,255 }; tag = "WEB"; }
            if (res.kind == "latex") { kc = { 80,200,120,255 }; tag = "LEAN"; }
            if (res.kind == "explanation") { kc = { 140,140,200,255 }; tag = "NOTA"; }

            DrawRectangle(rx, ry, card_w, card_h, { 17,17,30,255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, { 38,38,62,180 });

            int tw2 = MeasureText(tag, 9);
            DrawRectangle(rx + card_w - tw2 - 12, ry + 6, tw2 + 10, 16, { kc.r,kc.g,kc.b,40 });
            DrawText(tag, rx + card_w - tw2 - 7, ry + 8, 9, kc);

            DrawText(res.title.c_str(), rx + 10, ry + 10, 13, { 210,215,230,255 });
            DrawText(res.content.substr(0, 42).c_str(), rx + 10, ry + 30, 11, { 110,115,145,200 });

            if (res.kind == "link" &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                mouse.x > rx && mouse.x<rx + card_w &&
                mouse.y>ry && mouse.y < ry + card_h)
                OpenURL(res.content.c_str());
        }
        int rows = (total + 2) / 3;
        y += rows * (card_h + 10) + 10;
        float max_s = (float)std::max(0, (y + (int)state.resource_scroll) - scroll_h - scroll_top + 40);
        if (state.resource_scroll > max_s) state.resource_scroll = max_s;
    }

    EndScissorMode();

    // Scrollbar derecho
    int full_h = y + (int)state.resource_scroll - scroll_top;
    if (full_h > scroll_h) {
        float ratio = (float)scroll_h / full_h;
        float bar_h = std::max(24.0f, ratio * scroll_h);
        float max_s = (float)(full_h - scroll_h);
        float bar_y = scroll_top + (state.resource_scroll / max_s) * (scroll_h - bar_h);
        DrawRectangle(w - 6, scroll_top, 4, scroll_h, { 18,18,32,200 });
        DrawRectangle(w - 6, (int)bar_y, 4, (int)bar_h, { 70,70,120,255 });
    }
}