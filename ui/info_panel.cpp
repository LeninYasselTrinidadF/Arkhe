#include "info_panel.hpp"
#include "raylib.h"
#include <algorithm>
#include <queue>

// ── Helpers ───────────────────────────────────────────────────────────────────

static int draw_wrapped_text(const char* text, int x, int y, int max_w,
    int font_size, Color color) {
    std::string s(text);
    int line_y = y;
    while (!s.empty()) {
        int chars = 1;
        while (chars < (int)s.size() &&
            MeasureText(s.substr(0, chars + 1).c_str(), font_size) < max_w)
            chars++;
        if (chars < (int)s.size() && s[chars] != ' ') {
            int sp = (int)s.rfind(' ', chars);
            if (sp > 0) chars = sp;
        }
        DrawText(s.substr(0, chars).c_str(), x, line_y, font_size, color);
        s = s.substr(chars);
        if (!s.empty() && s[0] == ' ') s = s.substr(1);
        line_y += font_size + 4;
    }
    return line_y;
}

static std::string default_description(const MathNode* node) {
    if (!node) return "";
    switch (node->level) {
    case NodeLevel::Macro:
        return "Gran area matematica. Navega hacia las sub-areas para explorar las secciones especificas.";
    case NodeLevel::Section:
        return "Seccion de la clasificacion. Contiene sub-areas especializadas y temas de investigacion.";
    case NodeLevel::Subsection:
        return "Sub-area especializada. Los temas corresponden a entradas concretas con codigos de referencia.";
    case NodeLevel::Topic:
        return "Tema especifico de investigacion matematica con representacion en Mathlib4.";
    default:
        return "Selecciona una burbuja para ver su informacion detallada.";
    }
}

static void draw_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureText(text, 11);
    DrawRectangle(x, y, tw + 14, 20, bg);
    DrawText(text, x + 7, y + 5, 11, fg);
}

// BFS con match exacto primero, luego ancestro mas profundo por prefijo.
static std::shared_ptr<MathNode> find_node_by_code(
    const std::shared_ptr<MathNode>& root, const std::string& code)
{
    if (!root || code.empty()) return nullptr;

    std::shared_ptr<MathNode> best_match;
    size_t best_depth = 0;

    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (n->code == code) return n;
        if (n->code.size() < code.size() &&
            code.substr(0, n->code.size()) == n->code &&
            n->code.size() > best_depth)
        {
            best_match = n;
            best_depth = n->code.size();
        }
        for (auto& c : n->children) q.push(c);
    }
    return best_match;
}

// ── Proceso inverso: buscar modulos Mathlib relacionados con un codigo MSC/Std ─
// Recorre crossref_map y devuelve todos los modulos cuyo campo `msc` (o
// `standard`) contenga el codigo buscado, exacto o por prefijo.
struct MathlibHit {
    std::string module;
    std::string match_kind;  // "MSC" o "STD"
    std::string matched_code;
};

static std::vector<MathlibHit> find_mathlib_for_msc(
    const std::unordered_map<std::string, CrossRef>& cmap,
    const std::string& msc_code)
{
    std::vector<MathlibHit> hits;
    if (msc_code.empty()) return hits;
    for (auto& [mod, ref] : cmap) {
        for (auto& mc : ref.msc) {
            // Match exacto o por prefijo (ej: "34A" matchea "34A12")
            if (mc == msc_code ||
                (mc.size() >= msc_code.size() &&
                 mc.substr(0, msc_code.size()) == msc_code) ||
                (msc_code.size() >= mc.size() &&
                 msc_code.substr(0, mc.size()) == mc))
            {
                hits.push_back({ mod, "MSC", mc });
                break;
            }
        }
    }
    return hits;
}

static std::vector<MathlibHit> find_mathlib_for_std(
    const std::unordered_map<std::string, CrossRef>& cmap,
    const std::string& std_code)
{
    std::vector<MathlibHit> hits;
    if (std_code.empty()) return hits;
    for (auto& [mod, ref] : cmap) {
        for (auto& sc : ref.standard) {
            if (sc == std_code ||
                (sc.size() >= std_code.size() &&
                 sc.substr(0, std_code.size()) == std_code) ||
                (std_code.size() >= sc.size() &&
                 std_code.substr(0, sc.size()) == sc))
            {
                hits.push_back({ mod, "STD", sc });
                break;
            }
        }
    }
    return hits;
}

// Recopila todos los codigos MSC/STD "activos" dado el contexto de navegacion
static void collect_active_codes(
    AppState& state, MathNode* sel,
    std::vector<std::string>& out_msc,
    std::vector<std::string>& out_std)
{
    auto try_node = [&](MathNode* node) -> bool {
        if (!node) return false;
        if (!node->msc_refs.empty() || !node->standard_refs.empty()) {
            out_msc = node->msc_refs;
            out_std = node->standard_refs;
            return true;
        }
        auto it = state.crossref_map.find(node->code);
        if (it != state.crossref_map.end() &&
            (!it->second.msc.empty() || !it->second.standard.empty())) {
            out_msc = it->second.msc;
            out_std = it->second.standard;
            return true;
        }
        return false;
    };

    bool found = try_node(sel);
    if (!found)
        for (int i = (int)state.nav_stack.size() - 1; i >= 0 && !found; i--)
            found = try_node(state.nav_stack[i].get());

    if (!found && !state.crossref_map.empty()) {
        std::vector<std::string> candidates;
        if (sel) candidates.push_back(sel->code);
        for (int i = (int)state.nav_stack.size() - 1; i >= 0; i--)
            candidates.push_back(state.nav_stack[i]->code);

        for (auto& cand : candidates) {
            if (cand.empty()) continue;
            for (auto& [key, ref] : state.crossref_map) {
                bool match = (key.size() >= cand.size() &&
                    key.substr(0, cand.size()) == cand) ||
                    (cand.size() >= key.size() &&
                        cand.substr(0, key.size()) == key);
                if (match && (!ref.msc.empty() || !ref.standard.empty())) {
                    out_msc = ref.msc;
                    out_std = ref.standard;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
}

// Obtiene el codigo "raiz" del nodo o stack de navegacion actual
static std::string get_current_context_code(AppState& state, MathNode* sel) {
    if (sel && !sel->code.empty() && sel->code != "ROOT") return sel->code;
    for (int i = (int)state.nav_stack.size() - 1; i >= 0; i--) {
        auto& n = state.nav_stack[i];
        if (n && !n->code.empty() && n->code != "ROOT") return n->code;
    }
    return "";
}

// ── Dibujar tarjeta Mathlib inversa ───────────────────────────────────────────
static void draw_mathlib_hit_card(
    AppState& state, const MathlibHit& hit,
    int rx, int ry, int cw, int ch,
    Vector2 mouse)
{
    bool hov = mouse.x > rx && mouse.x < rx + cw &&
               mouse.y > ry && mouse.y < ry + ch;

    DrawRectangle(rx, ry, cw, ch,
        hov ? Color{ 28, 45, 28, 255 } : Color{ 15, 25, 15, 255 });
    DrawRectangleLinesEx({ (float)rx, (float)ry, (float)cw, (float)ch },
        1.0f, hov ? Color{ 80, 200, 100, 255 } : Color{ 35, 75, 40, 200 });

    draw_chip("ML", rx + cw - 34, ry + 5, { 20, 70, 30, 200 }, { 100, 220, 120, 255 });

    // Mostrar solo la parte final del modulo (mas legible)
    std::string mod = hit.module;
    auto dot = mod.rfind('.');
    std::string mod_short = (dot != std::string::npos) ? mod.substr(dot + 1) : mod;
    if ((int)mod_short.size() > 22) mod_short = mod_short.substr(0, 21) + ".";
    DrawText(mod_short.c_str(), rx + 8, ry + 7, 12, { 140, 230, 160, 255 });

    // Ruta del modulo truncada
    std::string mod_full = hit.module;
    if ((int)mod_full.size() > 36) mod_full = ".." + mod_full.substr(mod_full.size() - 34);
    DrawText(mod_full.c_str(), rx + 8, ry + 24, 9, { 80, 150, 95, 200 });

    DrawText(("via " + hit.matched_code).c_str(), rx + 8, ry + 37, 9, { 70, 120, 80, 180 });
    if (ch > 52)
        DrawText("[ click para navegar ]", rx + 8, ry + 50, 8, { 60, 110, 70, 160 });

    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        auto found = find_node_by_code(state.mathlib_root, hit.module);
        if (found) {
            state.pending_nav = {
                true,
                ViewMode::Mathlib,
                found->code,
                found->label,
                state.mathlib_root,
                found
            };
        }
    }
}

// ── Dibujar sprite de textura en una burbuja ──────────────────────────────────
// Se usa desde bubble_view si se quiere, pero tambien desde info_panel
// para mostrar un preview de la imagen del nodo seleccionado.
static void draw_node_sprite_preview(AppState& state, MathNode* node,
    int x, int y, int size)
{
    if (!node || node->texture_key.empty()) return;
    Texture2D tex = state.textures.get(node->texture_key);
    if (tex.id == 0) return;

    // Escalar manteniendo aspect ratio
    float scale = (float)size / (float)std::max(tex.width, tex.height);
    int dw = (int)(tex.width  * scale);
    int dh = (int)(tex.height * scale);
    int dx = x + (size - dw) / 2;
    int dy = y + (size - dh) / 2;

    DrawRectangle(x, y, size, size, { 10, 10, 20, 200 });
    DrawRectangleLinesEx({ (float)x,(float)y,(float)size,(float)size }, 1, { 60,60,100,180 });
    DrawTexturePro(tex,
        { 0, 0, (float)tex.width, (float)tex.height },
        { (float)dx, (float)dy, (float)dw, (float)dh },
        { 0, 0 }, 0.0f, WHITE);
}

// ── Panel info ────────────────────────────────────────────────────────────────

void draw_info_panel(AppState& state, Vector2 mouse) {
    int top = TOP_H();
    int w   = SW();
    int vh  = SH() - top;

    DrawRectangle(0, top, w, vh, { 10, 10, 18, 255 });
    DrawLine(0, top, w, top, { 50, 50, 75, 255 });

    // ── Cabecera fija ─────────────────────────────────────────────────────────
    constexpr int HEADER_H = 100;
    int px = 18;

    int tx = px;
    for (int i = 1; i < (int)state.nav_stack.size(); i++) {
        auto& n = state.nav_stack[i];
        std::string crumb = n->label;
        if ((int)crumb.size() > 22) crumb = crumb.substr(0, 21) + ".";
        Color cc = (i == (int)state.nav_stack.size() - 1)
            ? Color{ 120, 190, 255, 255 } : Color{ 80, 110, 160, 200 };
        DrawText(crumb.c_str(), tx, top + 8, 11, cc);
        tx += MeasureText(crumb.c_str(), 11);
        if (i < (int)state.nav_stack.size() - 1) {
            DrawText("  ›  ", tx, top + 8, 11, { 55, 55, 80, 255 });
            tx += MeasureText("  ›  ", 11);
        }
        if (tx > w - 200) { DrawText("...", tx, top + 8, 11, { 70, 70, 100, 255 }); break; }
    }

    if (!state.selected_code.empty()) {
        std::string title = state.selected_label;
        if ((int)title.size() > 55) title = title.substr(0, 54) + "...";
        DrawText(title.c_str(), px, top + 24, 20, { 220, 225, 240, 255 });

        draw_chip(state.selected_code.c_str(), px, top + 52,
            { 30, 60, 100, 220 }, { 100, 190, 255, 255 });

        const char* level_str = "—";
        MathNode* cur = state.current();
        if (cur) for (auto& child : cur->children)
            if (child->code == state.selected_code) {
                switch (child->level) {
                case NodeLevel::Macro:      level_str = "Macro-area"; break;
                case NodeLevel::Section:    level_str = "Seccion";    break;
                case NodeLevel::Subsection: level_str = "Sub-area";   break;
                case NodeLevel::Topic:      level_str = "Tema";       break;
                default: break;
                }
                break;
            }
        int chip_w = MeasureText(state.selected_code.c_str(), 11) + 14 + 8;
        draw_chip(level_str, px + chip_w, top + 52, { 28, 28, 45, 220 }, { 130, 130, 170, 255 });

        // ── Badge de modo actual en la cabecera ───────────────────────────────
        const char* mode_badge =
            state.mode == ViewMode::Mathlib  ? "MATHLIB"  :
            state.mode == ViewMode::Standard ? "ESTANDAR" : "MSC2020";
        Color mode_col =
            state.mode == ViewMode::Mathlib  ? Color{ 100, 220, 130, 255 } :
            state.mode == ViewMode::Standard ? Color{ 255, 200, 100, 255 } :
                                               Color{ 100, 190, 255, 255 };
        draw_chip(mode_badge, px + chip_w + MeasureText(level_str, 11) + 30,
            top + 52, { 20, 20, 35, 220 }, mode_col);
    }
    else {
        DrawText("Selecciona una burbuja", px, top + 28, 18, { 70, 75, 100, 255 });
        DrawText("para ver su informacion detallada.", px, top + 52, 12, { 55, 58, 80, 255 });
    }

    DrawLine(0, top + HEADER_H, w, top + HEADER_H, { 35, 35, 55, 255 });

    // ── Zona scrollable ───────────────────────────────────────────────────────
    int scroll_top = top + HEADER_H;
    int scroll_h   = vh - HEADER_H;

    if (mouse.y > scroll_top) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) {
            state.resource_scroll -= wh * 32.0f;
            if (state.resource_scroll < 0.0f) state.resource_scroll = 0.0f;
        }
    }

    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) for (auto& child : cur->children)
        if (child->code == state.selected_code) { sel = child.get(); break; }

    // ── Recopilar referencias cruzadas activas ────────────────────────────────
    std::vector<std::string> active_msc_refs;
    std::vector<std::string> active_std_refs;
    collect_active_codes(state, sel, active_msc_refs, active_std_refs);

    bool has_refs = !active_msc_refs.empty() || !active_std_refs.empty();

    // ── Proceso INVERSO: buscar Mathlib desde MSC2020 o Estandar ─────────────
    // Solo se activa cuando el modo actual es MSC2020 o Standard
    std::vector<MathlibHit> inverse_hits;
    bool show_inverse = (state.mode == ViewMode::MSC2020 || state.mode == ViewMode::Standard)
                        && state.mathlib_root
                        && !state.crossref_map.empty();

    if (show_inverse) {
        std::string ctx_code = get_current_context_code(state, sel);
        if (!ctx_code.empty()) {
            if (state.mode == ViewMode::MSC2020) {
                // Buscar por cualquiera de los codigos MSC activos + el codigo del nodo
                std::vector<std::string> to_search = active_msc_refs;
                to_search.push_back(ctx_code);
                for (auto& code : to_search) {
                    auto hits = find_mathlib_for_msc(state.crossref_map, code);
                    for (auto& h : hits) {
                        // Evitar duplicados
                        bool dup = false;
                        for (auto& existing : inverse_hits)
                            if (existing.module == h.module) { dup = true; break; }
                        if (!dup) inverse_hits.push_back(h);
                    }
                }
            } else {
                // ViewMode::Standard
                std::vector<std::string> to_search = active_std_refs;
                to_search.push_back(ctx_code);
                for (auto& code : to_search) {
                    auto hits = find_mathlib_for_std(state.crossref_map, code);
                    for (auto& h : hits) {
                        bool dup = false;
                        for (auto& existing : inverse_hits)
                            if (existing.module == h.module) { dup = true; break; }
                        if (!dup) inverse_hits.push_back(h);
                    }
                }
            }
        }
    }

    BeginScissorMode(0, scroll_top, w, scroll_h);

    int y    = scroll_top + 14 - (int)state.resource_scroll;
    int col  = 18;
    int card_w = (w - col * 2 - 10 * 2) / 3;
    int card_h = 68;

    // ── Preview de imagen del nodo (si tiene texture_key) ────────────────────
    if (sel && !sel->texture_key.empty()) {
        int sprite_size = 56;
        // Dibujar el preview a la derecha del titulo (en zona scrollable)
        draw_node_sprite_preview(state, sel, w - sprite_size - col, y, sprite_size);
    }

    // ── Descripcion ───────────────────────────────────────────────────────────
    DrawText("DESCRIPCION", col, y, 11, { 80, 80, 120, 255 });
    y += 18;
    std::string desc = (sel && !sel->note.empty()) ? sel->note : default_description(sel);
    int desc_max_w = (sel && !sel->texture_key.empty()) ? w / 2 - 30 : w / 2 - 30;
    y = draw_wrapped_text(desc.c_str(), col, y, desc_max_w, 13, { 190, 195, 215, 255 });
    y += 14;
    DrawLine(col, y, w - col, y, { 30, 30, 50, 200 });
    y += 14;

    // ── PROCESO INVERSO: modulos Mathlib relacionados ─────────────────────────
    if (show_inverse) {
        Color inv_hdr_col = (state.mode == ViewMode::MSC2020)
            ? Color{ 100, 200, 120, 255 } : Color{ 200, 180, 100, 255 };
        const char* inv_label = (state.mode == ViewMode::MSC2020)
            ? "MODULOS MATHLIB RELACIONADOS (via MSC)"
            : "MODULOS MATHLIB RELACIONADOS (via Estandar)";
        DrawText(inv_label, col, y, 11, inv_hdr_col);
        y += 18;

        if (inverse_hits.empty()) {
            DrawText("No hay referencias cruzadas para esta area.",
                col, y, 11, { 80, 90, 110, 200 });
            DrawText("Agrega entradas en crossref.json con los codigos MSC de este nodo.",
                col, y + 16, 10, { 65, 75, 95, 180 });
            y += 42;
        } else {
            int inv_cw = (w - col * 2 - 10 * 2) / 3;
            int inv_ch = 64;

            int total = (int)inverse_hits.size();
            int shown = std::min(total, 9); // max 3 filas
            for (int i = 0; i < shown; i++) {
                int rx = col + (i % 3) * (inv_cw + 10);
                int ry = y   + (i / 3) * (inv_ch + 8);
                draw_mathlib_hit_card(state, inverse_hits[i], rx, ry, inv_cw, inv_ch, mouse);
            }
            int rows = (shown + 2) / 3;
            y += rows * (inv_ch + 8) + 4;

            if (total > shown) {
                char more[48];
                snprintf(more, sizeof(more), "... y %d modulos mas (ampliar crossref.json)", total - shown);
                DrawText(more, col, y, 10, { 80, 120, 90, 200 });
                y += 18;
            }
        }

        DrawLine(col, y, w - col, y, { 30, 30, 50, 200 });
        y += 14;
    }

    // ── Referencias cruzadas (salida de Mathlib hacia MSC/Std) ────────────────
    if (has_refs) {
        DrawText("REFERENCIAS CRUZADAS", col, y, 11, { 80, 80, 120, 255 });
        y += 18;

        int ci = 0;

        for (auto& msc_code : active_msc_refs) {
            int rx = col + (ci % 3) * (card_w + 10);
            int ry = y   + (ci / 3) * (card_h + 10);

            bool hovered = mouse.x > rx && mouse.x < rx + card_w &&
                mouse.y > ry && mouse.y < ry + card_h;

            DrawRectangle(rx, ry, card_w, card_h,
                hovered ? Color{ 25, 45, 80, 255 } : Color{ 15, 25, 45, 255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, hovered ? Color{ 100, 150, 255, 255 } : Color{ 40, 60, 110, 200 });

            draw_chip("MSC", rx + card_w - 44, ry + 6, { 30, 60, 120, 200 }, { 100, 160, 255, 255 });
            DrawText(msc_code.c_str(), rx + 10, ry + 10, 14, { 140, 200, 255, 255 });
            DrawText("Ver en MSC2020", rx + 10, ry + 32, 10, { 90, 130, 180, 200 });
            DrawText("[ click para navegar ]", rx + 10, ry + 48, 9, { 70, 100, 150, 180 });

            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                auto found = find_node_by_code(state.msc_root, msc_code);
                if (found) {
                    state.pending_nav = {
                        true,
                        ViewMode::MSC2020,
                        found->code,
                        found->label,
                        state.msc_root,
                        found
                    };
                }
            }
            ci++;
        }

        for (auto& std_code : active_std_refs) {
            int rx = col + (ci % 3) * (card_w + 10);
            int ry = y   + (ci / 3) * (card_h + 10);

            bool hovered = mouse.x > rx && mouse.x < rx + card_w &&
                mouse.y > ry && mouse.y < ry + card_h;

            DrawRectangle(rx, ry, card_w, card_h,
                hovered ? Color{ 30, 50, 35, 255 } : Color{ 15, 28, 18, 255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, hovered ? Color{ 100, 200, 120, 255 } : Color{ 40, 90, 55, 200 });

            draw_chip("STD", rx + card_w - 44, ry + 6, { 30, 80, 45, 200 }, { 100, 200, 120, 255 });
            std::string label = std_code;
            auto dot = std_code.rfind('.');
            if (dot != std::string::npos) label = std_code.substr(dot + 1);
            DrawText(label.c_str(), rx + 10, ry + 10, 13, { 140, 220, 160, 255 });
            DrawText(std_code.c_str(), rx + 10, ry + 28, 9, { 90, 160, 110, 190 });
            DrawText("Ver en Estandar", rx + 10, ry + 42, 10, { 70, 130, 90, 200 });
            DrawText("[ click para navegar ]", rx + 10, ry + 54, 9, { 60, 110, 75, 180 });

            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.standard_root) {
                auto found = find_node_by_code(state.standard_root, std_code);
                if (found) {
                    state.pending_nav = {
                        true,
                        ViewMode::Standard,
                        found->code,
                        found->label,
                        state.standard_root,
                        found
                    };
                }
            }
            ci++;
        }

        int rows = (ci + 2) / 3;
        y += rows * (card_h + 10) + 14;
        DrawLine(col, y, w - col, y, { 30, 30, 50, 200 });
        y += 14;
    }

    // ── Recursos ──────────────────────────────────────────────────────────────
    DrawText("RECURSOS", col, y, 11, { 80, 80, 120, 255 });
    y += 18;

    const std::vector<Resource>* resources = sel ? &sel->resources : nullptr;
    bool has_res = resources && !resources->empty();

    if (!has_res) {
        struct Ph { const char* tag, * title, * desc; Color kc; } ph[] = {
            {"LIBRO",  "Introduction",     "Texto introductorio",  {200, 140,  40, 255}},
            {"WEB",    "MathSciNet",        "mathscinet.ams.org",   { 60, 150, 220, 255}},
            {"NOTA",   "Nota tecnica",      "Descripcion formal",   {140, 140, 200, 255}},
            {"WEB",    "zbMATH Open",       "zbmath.org",           { 60, 150, 220, 255}},
            {"PAPER",  "Survey article",    "Estado del arte",      {180, 100, 200, 255}},
            {"LEAN",   "Definicion formal", "Formulacion en Lean",  { 80, 200, 120, 255}},
        };
        for (int i = 0; i < 6; i++) {
            int rx = col + (i % 3) * (card_w + 10);
            int ry = y   + (i / 3) * (card_h + 10);
            DrawRectangle(rx, ry, card_w, card_h, { 17, 17, 30, 255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, { 38, 38, 62, 180 });
            int tw = MeasureText(ph[i].tag, 9);
            DrawRectangle(rx + card_w - tw - 12, ry + 6, tw + 10, 16,
                { ph[i].kc.r, ph[i].kc.g, ph[i].kc.b, 40 });
            DrawText(ph[i].tag, rx + card_w - tw - 7, ry + 8, 9, ph[i].kc);
            DrawText(ph[i].title, rx + 10, ry + 10, 13, { 210, 215, 230, 255 });
            DrawText(ph[i].desc,  rx + 10, ry + 30, 11, { 110, 115, 145, 200 });
        }
        y += 2 * (card_h + 10) + 10;
    }
    else {
        int total = (int)resources->size();
        for (int i = 0; i < total; i++) {
            auto& res = (*resources)[i];
            int rx = col + (i % 3) * (card_w + 10);
            int ry = y   + (i / 3) * (card_h + 10);

            Color kc = { 80, 80, 130, 255 }; const char* tag = "RES";
            if (res.kind == "book")        { kc = { 200, 140,  40, 255 }; tag = "LIBRO"; }
            if (res.kind == "link")        { kc = {  60, 150, 220, 255 }; tag = "WEB";   }
            if (res.kind == "latex")       { kc = {  80, 200, 120, 255 }; tag = "LEAN";  }
            if (res.kind == "explanation") { kc = { 140, 140, 200, 255 }; tag = "NOTA";  }

            bool hovered = mouse.x > rx && mouse.x < rx + card_w &&
                mouse.y > ry && mouse.y < ry + card_h;
            DrawRectangle(rx, ry, card_w, card_h,
                hovered ? Color{ 22, 22, 40, 255 } : Color{ 17, 17, 30, 255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h },
                1, hovered ? kc : Color{ 38, 38, 62, 180 });

            int tw = MeasureText(tag, 9);
            DrawRectangle(rx + card_w - tw - 12, ry + 6, tw + 10, 16, { kc.r, kc.g, kc.b, 40 });
            DrawText(tag, rx + card_w - tw - 7, ry + 8, 9, kc);
            DrawText(res.title.c_str(), rx + 10, ry + 10, 13, { 210, 215, 230, 255 });
            DrawText(res.content.substr(0, 42).c_str(), rx + 10, ry + 30, 11, { 110, 115, 145, 200 });

            if (res.kind == "link" && hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                OpenURL(res.content.c_str());
        }
        y += ((total + 2) / 3) * (card_h + 10) + 10;
    }

    float max_s = (float)std::max(0, y + (int)state.resource_scroll - scroll_h - scroll_top + 40);
    if (state.resource_scroll > max_s) state.resource_scroll = max_s;

    EndScissorMode();

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    int full_h = y + (int)state.resource_scroll - scroll_top;
    if (full_h > scroll_h) {
        float ratio = (float)scroll_h / full_h;
        float bar_h = std::max(24.0f, ratio * scroll_h);
        float max_sv = (float)(full_h - scroll_h);
        float bar_y  = scroll_top + (state.resource_scroll / max_sv) * (scroll_h - bar_h);
        DrawRectangle(w - 6, scroll_top, 4, scroll_h, { 18, 18, 32, 200 });
        DrawRectangle(w - 6, (int)bar_y, 4, (int)bar_h, { 70, 70, 120, 255 });
    }
}
