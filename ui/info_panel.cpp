#include "info_panel.hpp"
#include "raylib.h"
#include <algorithm>
#include <queue>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <cstring>

namespace fs = std::filesystem;

// ══════════════════════════════════════════════════════════════════════════════
// Helpers de texto
// ══════════════════════════════════════════════════════════════════════════════

static int draw_wrapped_text(const char* text, int x, int y, int max_w,
    int font_size, Color color)
{
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
    case NodeLevel::Macro:      return "Gran area matematica. Navega hacia las sub-areas para explorar.";
    case NodeLevel::Section:    return "Seccion de la clasificacion. Contiene sub-areas especializadas.";
    case NodeLevel::Subsection: return "Sub-area especializada con temas de investigacion concretos.";
    case NodeLevel::Topic:      return "Tema especifico de investigacion matematica con representacion en Mathlib4.";
    default:                    return "Selecciona una burbuja para ver su informacion detallada.";
    }
}

static void draw_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureText(text, 11);
    DrawRectangle(x, y, tw + 14, 20, bg);
    DrawText(text, x + 7, y + 5, 11, fg);
}

// ══════════════════════════════════════════════════════════════════════════════
// Busqueda de nodo
// ══════════════════════════════════════════════════════════════════════════════

static std::shared_ptr<MathNode> find_node_by_code(
    const std::shared_ptr<MathNode>& root, const std::string& code)
{
    if (!root || code.empty()) return nullptr;
    std::shared_ptr<MathNode> best;
    size_t best_depth = 0;
    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (n->code == code) return n;
        if (n->code.size() < code.size() &&
            code.substr(0, n->code.size()) == n->code &&
            n->code.size() > best_depth)
        { best = n; best_depth = n->code.size(); }
        for (auto& c : n->children) q.push(c);
    }
    return best;
}

// ══════════════════════════════════════════════════════════════════════════════
// Carga y conversion de archivos .tex
// ══════════════════════════════════════════════════════════════════════════════

static std::string load_entry_tex(const std::string& entries_path, const std::string& code) {
    if (code.empty() || code == "ROOT") return "";
    std::string path = entries_path;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += code + ".tex";
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static std::string tex_to_display(const std::string& tex) {
    std::string out; out.reserve(tex.size());
    int brace = 0;
    for (size_t i = 0; i < tex.size(); i++) {
        char c = tex[i];
        if (c == '%') { while (i < tex.size() && tex[i] != '\n') i++; out += '\n'; continue; }
        if (c == '\\') {
            size_t j = i + 1;
            while (j < tex.size() && isalpha((unsigned char)tex[j])) j++;
            std::string cmd = tex.substr(i + 1, j - i - 1);
            if (cmd == "section" || cmd == "subsection" || cmd == "subsubsection")
                { out += "\n\n"; i = j - 1; continue; }
            if (cmd == "textbf" || cmd == "textit" || cmd == "emph" ||
                cmd == "text"   || cmd == "mathrm")
                { i = j - 1; continue; }
            if (cmd == "newline" || cmd == "\\") { out += '\n'; i = j - 1; continue; }
            if (cmd == "item")  { out += "\n  * "; i = j - 1; continue; }
            if (cmd == "begin" || cmd == "end") { i = j - 1; continue; }
            i = j - 1; continue;
        }
        if (c == '{') { brace++; out += c; continue; }
        if (c == '}') { if (brace > 0) { brace--; out += c; } continue; }
        if (c == '$') continue;
        out += c;
    }
    return out;
}

// ══════════════════════════════════════════════════════════════════════════════
// Sistema de render LaTeX (pdflatex + pdftoppm)
// ══════════════════════════════════════════════════════════════════════════════

static std::string temp_dir() {
    const char* tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "/tmp";
    return std::string(tmp) + "/arkhe_latex/";
}

static std::string wrap_tex(const std::string& raw) {
    if (raw.find("\\documentclass") != std::string::npos) return raw;
    return "\\documentclass[12pt]{article}\n"
           "\\usepackage{amsmath,amssymb,amsthm}\n"
           "\\usepackage[margin=1.5cm]{geometry}\n"
           "\\pagestyle{empty}\n"
           "\\begin{document}\n" + raw + "\n\\end{document}\n";
}

static void launch_latex_render(AppState& state, const std::string& code,
    const std::string& raw_tex)
{
    auto& job = state.latex_render;
    if (job.tex_loaded) { UnloadTexture(job.texture); job.texture = {}; job.tex_loaded = false; }
    job.tex_code = code; job.state = LatexRenderState::Compiling;
    job.error_msg.clear(); job.thread_done = false;

    std::string td       = temp_dir();
    std::string src      = td + "entry.tex";
    std::string png_out  = td + "entry";
    job.png_path         = td + "entry-1.png";

    std::string latex_exe    = state.toolbar.latex_path;
    std::string pdftoppm_exe = state.toolbar.pdftoppm_path;
    std::string wrapped      = wrap_tex(raw_tex);

    std::thread([=, &job]() {
        try {
            fs::create_directories(td);
            { std::ofstream f(src); if (!f) { job.error_msg = "No se pudo escribir " + src; job.state = LatexRenderState::Failed; job.thread_done = true; return; } f << wrapped; }
            int r1 = std::system(("\"" + latex_exe + "\" -interaction=nonstopmode -output-directory=\"" + td + "\" \"" + src + "\" > nul 2>&1").c_str());
            if (!fs::exists(td + "entry.pdf")) { job.error_msg = "pdflatex fallo (codigo " + std::to_string(r1) + "). Verifica la ruta en Ubicaciones."; job.state = LatexRenderState::Failed; job.thread_done = true; return; }
            std::system(("\"" + pdftoppm_exe + "\" -r 144 -png \"" + td + "entry.pdf\" \"" + png_out + "\" > nul 2>&1").c_str());
            if (!fs::exists(job.png_path)) { job.error_msg = "pdftoppm no genero el PNG. Verifica la ruta en Ubicaciones."; job.state = LatexRenderState::Failed; job.thread_done = true; return; }
            job.state = LatexRenderState::Ready;
        } catch (std::exception& e) { job.error_msg = e.what(); job.state = LatexRenderState::Failed; }
        job.thread_done = true;
    }).detach();
}

static void poll_latex_render(AppState& state) {
    auto& job = state.latex_render;
    if (job.state == LatexRenderState::Ready && job.thread_done && !job.tex_loaded) {
        if (fs::exists(job.png_path)) {
            job.texture   = LoadTexture(job.png_path.c_str());
            job.tex_loaded = (job.texture.id != 0);
            if (!job.tex_loaded) job.error_msg = "LoadTexture fallo: " + job.png_path;
        }
    }
}

// Retorna nuevo y tras dibujar el widget (textura, spinner o error).
static int draw_latex_widget(AppState& state, int x, int y, int max_w, int max_h) {
    auto& job = state.latex_render;
    if (job.state == LatexRenderState::Compiling) {
        double t = GetTime(); int dots = (int)(t * 2.0) % 4;
        std::string msg = "Compilando LaTeX"; for (int d = 0; d < dots; d++) msg += '.';
        DrawText(msg.c_str(), x, y, 12, { 120,180,120,200 });
        float bar_w = 180.0f, fill = (float)(fmod(t * 0.5, 1.0)) * bar_w;
        DrawRectangle(x, y + 18, (int)bar_w, 4, { 30,30,50,200 });
        DrawRectangle(x, y + 18, (int)fill,  4, { 80,160,80,255 });
        return y + 30;
    }
    if (job.state == LatexRenderState::Failed) {
        DrawText("Error al compilar LaTeX:", x, y, 11, { 220,80,80,255 }); y += 16;
        y = draw_wrapped_text((job.error_msg.empty() ? "Error desconocido." : job.error_msg).c_str(),
            x, y, max_w, 10, { 180,80,80,220 }); y += 6;
        DrawText("Verifica rutas en Ubicaciones > pdflatex / pdftoppm.", x, y, 10, { 100,100,150,200 });
        return y + 16;
    }
    if (job.state == LatexRenderState::Ready && job.tex_loaded) {
        Texture2D& tex = job.texture;
        float scale = std::min({ (float)max_w / tex.width, (float)max_h / tex.height, 1.0f });
        int dw = (int)(tex.width * scale), dh = (int)(tex.height * scale);
        DrawRectangle(x - 2, y - 2, dw + 4, dh + 4, { 20,25,40,255 });
        DrawRectangleLinesEx({ (float)(x-2),(float)(y-2),(float)(dw+4),(float)(dh+4) }, 1.0f, { 60,80,140,220 });
        DrawTexturePro(tex, { 0,0,(float)tex.width,(float)tex.height },
            { (float)x,(float)y,(float)dw,(float)dh }, { 0,0 }, 0.0f, WHITE);
        return y + dh + 8;
    }
    return y;
}

// ══════════════════════════════════════════════════════════════════════════════
// Referencias cruzadas e inverso
// ══════════════════════════════════════════════════════════════════════════════

struct MathlibHit { std::string module, match_kind, matched_code; };

static std::vector<MathlibHit> find_mathlib_hits(
    const std::unordered_map<std::string, CrossRef>& cmap,
    const std::string& code, bool use_msc)
{
    std::vector<MathlibHit> hits;
    if (code.empty()) return hits;
    for (auto& [mod, ref] : cmap) {
        auto& vec = use_msc ? ref.msc : ref.standard;
        for (auto& mc : vec) {
            if (mc == code ||
                (mc.size() >= code.size() && mc.substr(0, code.size()) == code) ||
                (code.size() >= mc.size() && code.substr(0, mc.size()) == mc))
            { hits.push_back({ mod, use_msc ? "MSC" : "STD", mc }); break; }
        }
    }
    return hits;
}

static void collect_active_codes(AppState& state, MathNode* sel,
    std::vector<std::string>& out_msc, std::vector<std::string>& out_std)
{
    auto try_node = [&](MathNode* node) -> bool {
        if (!node) return false;
        if (!node->msc_refs.empty() || !node->standard_refs.empty())
            { out_msc = node->msc_refs; out_std = node->standard_refs; return true; }
        auto it = state.crossref_map.find(node->code);
        if (it != state.crossref_map.end() &&
            (!it->second.msc.empty() || !it->second.standard.empty()))
            { out_msc = it->second.msc; out_std = it->second.standard; return true; }
        return false;
    };
    bool found = try_node(sel);
    for (int i = (int)state.nav_stack.size() - 1; i >= 0 && !found; i--)
        found = try_node(state.nav_stack[i].get());

    if (!found && !state.crossref_map.empty()) {
        std::vector<std::string> cands;
        if (sel) cands.push_back(sel->code);
        for (int i = (int)state.nav_stack.size() - 1; i >= 0; i--)
            cands.push_back(state.nav_stack[i]->code);
        for (auto& cand : cands) {
            if (cand.empty()) continue;
            for (auto& [key, ref] : state.crossref_map) {
                bool match = (key.size() >= cand.size() && key.substr(0, cand.size()) == cand) ||
                             (cand.size() >= key.size() && cand.substr(0, key.size()) == key);
                if (match && (!ref.msc.empty() || !ref.standard.empty()))
                    { out_msc = ref.msc; out_std = ref.standard; found = true; break; }
            }
            if (found) break;
        }
    }
}

static std::string get_context_code(AppState& state, MathNode* sel) {
    if (sel && !sel->code.empty() && sel->code != "ROOT") return sel->code;
    for (int i = (int)state.nav_stack.size() - 1; i >= 0; i--) {
        auto& n = state.nav_stack[i];
        if (n && !n->code.empty() && n->code != "ROOT") return n->code;
    }
    return "";
}

// ══════════════════════════════════════════════════════════════════════════════
// Sub-secciones del panel
// ══════════════════════════════════════════════════════════════════════════════

// ── Cabecera (breadcrumb + titulo + chips) ────────────────────────────────────

static void draw_panel_header(AppState& state, int top, int w) {
    const int px = 18;

    // Breadcrumb
    int tx = px;
    for (int i = 1; i < (int)state.nav_stack.size(); i++) {
        std::string crumb = state.nav_stack[i]->label;
        if ((int)crumb.size() > 22) crumb = crumb.substr(0, 21) + ".";
        Color cc = (i == (int)state.nav_stack.size() - 1)
            ? Color{ 120,190,255,255 } : Color{ 80,110,160,200 };
        DrawText(crumb.c_str(), tx, top + 8, 11, cc);
        tx += MeasureText(crumb.c_str(), 11);
        if (i < (int)state.nav_stack.size() - 1) {
            DrawText("  \xc2\xbb  ", tx, top + 8, 11, { 55,55,80,255 });
            tx += MeasureText("  >  ", 11);
        }
        if (tx > w - 200) { DrawText("...", tx, top + 8, 11, { 70,70,100,255 }); break; }
    }

    if (state.selected_code.empty()) {
        DrawText("Selecciona una burbuja", px, top + 28, 18, { 70,75,100,255 });
        DrawText("para ver su informacion detallada.", px, top + 52, 12, { 55,58,80,255 });
        return;
    }

    std::string title = state.selected_label;
    if ((int)title.size() > 55) title = title.substr(0, 54) + "...";
    DrawText(title.c_str(), px, top + 24, 20, { 220,225,240,255 });

    draw_chip(state.selected_code.c_str(), px, top + 52, { 30,60,100,220 }, { 100,190,255,255 });

    const char* level_str = "-";
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
    int chip_w = MeasureText(state.selected_code.c_str(), 11) + 22;
    draw_chip(level_str, px + chip_w, top + 52, { 28,28,45,220 }, { 130,130,170,255 });

    const char* mode_badge = state.mode == ViewMode::Mathlib  ? "MATHLIB"  :
                             state.mode == ViewMode::Standard ? "ESTANDAR" : "MSC2020";
    Color mode_col         = state.mode == ViewMode::Mathlib  ? Color{ 100,220,130,255 } :
                             state.mode == ViewMode::Standard ? Color{ 255,200,100,255 } :
                                                                Color{ 100,190,255,255 };
    draw_chip(mode_badge, px + chip_w + MeasureText(level_str, 11) + 30,
        top + 52, { 20,20,35,220 }, mode_col);
}

// ── Sprite preview (pequeño, esquina superior derecha) ────────────────────────

static void draw_sprite_preview(AppState& state, MathNode* node, int x, int y, int size) {
    if (!node || node->texture_key.empty()) return;
    Texture2D tex = state.textures.get(node->texture_key);
    if (tex.id == 0) return;
    float scale = (float)size / std::max(tex.width, tex.height);
    int dw = (int)(tex.width * scale), dh = (int)(tex.height * scale);
    DrawRectangle(x, y, size, size, { 10,10,20,200 });
    DrawRectangleLinesEx({ (float)x,(float)y,(float)size,(float)size }, 1, { 60,60,100,180 });
    DrawTexturePro(tex, { 0,0,(float)tex.width,(float)tex.height },
        { (float)(x + (size-dw)/2),(float)(y + (size-dh)/2),(float)dw,(float)dh },
        { 0,0 }, 0.0f, WHITE);
}

// ── Bloque descripcion / LaTeX ────────────────────────────────────────────────

static int draw_description_block(AppState& state, MathNode* sel,
    const std::string& tex_target,
    const std::string& cached_tex_raw, const std::string& cached_tex_display,
    Vector2 mouse, int x, int y, int w, int scroll_h)
{
    DrawText("DESCRIPCION", x, y, 11, { 80,80,120,255 }); y += 18;

    bool has_tex = !cached_tex_display.empty();
    if (!has_tex) {
        std::string desc = (sel && !sel->note.empty()) ? sel->note : default_description(sel);
        y = draw_wrapped_text(desc.c_str(), x, y, w / 2 - 30, 13, { 190,195,215,255 });
    } else {
        draw_chip(".tex", x, y, { 25,50,25,200 }, { 80,200,100,255 });

        auto& job = state.latex_render;
        bool can_render = (job.state == LatexRenderState::Idle   ||
                           job.state == LatexRenderState::Failed ||
                           job.state == LatexRenderState::Ready);
        const char* render_label = (job.state == LatexRenderState::Ready && job.tex_loaded)
            ? "Re-render LaTeX" : "Render LaTeX";
        int btn_x = x + MeasureText(".tex", 11) + 22;
        int btn_w = MeasureText(render_label, 10) + 16;
        Rectangle btn_r = { (float)btn_x,(float)y,(float)btn_w,20.0f };
        bool btn_hov = CheckCollisionPointRec(mouse, btn_r);
        if (can_render) {
            DrawRectangleRec(btn_r, btn_hov ? Color{ 40,90,50,255 } : Color{ 25,55,30,255 });
            DrawRectangleLinesEx(btn_r, 1.0f, { 60,150,70,200 });
            DrawText(render_label, btn_x + 8, y + 5, 10, WHITE);
            if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                launch_latex_render(state, tex_target, cached_tex_raw);
        }
        y += 24;

        if (job.tex_code == tex_target &&
            (job.state == LatexRenderState::Ready    ||
             job.state == LatexRenderState::Compiling ||
             job.state == LatexRenderState::Failed))
        {
            y = draw_latex_widget(state, x, y, w - 20, scroll_h - 60);
            y += 10;
        }

        DrawText("TEXTO FUENTE (.tex):", x, y, 10, { 60,70,100,200 }); y += 14;
        std::string preview = cached_tex_display;
        if ((int)preview.size() > 600) preview = preview.substr(0, 597) + "...";
        y = draw_wrapped_text(preview.c_str(), x, y, w - 30, 11, { 170,180,165,220 });
    }
    return y;
}

// ── Bloque proceso inverso (Mathlib hits) ─────────────────────────────────────

static void draw_mathlib_hit_card(AppState& state, const MathlibHit& hit,
    int rx, int ry, int cw, int ch, Vector2 mouse)
{
    bool hov = mouse.x > rx && mouse.x < rx + cw && mouse.y > ry && mouse.y < ry + ch;
    DrawRectangle(rx, ry, cw, ch, hov ? Color{ 28,45,28,255 } : Color{ 15,25,15,255 });
    DrawRectangleLinesEx({ (float)rx,(float)ry,(float)cw,(float)ch }, 1.0f,
        hov ? Color{ 80,200,100,255 } : Color{ 35,75,40,200 });
    draw_chip("ML", rx + cw - 34, ry + 5, { 20,70,30,200 }, { 100,220,120,255 });

    std::string mod_short = hit.module;
    auto dot = mod_short.rfind('.');
    if (dot != std::string::npos) mod_short = mod_short.substr(dot + 1);
    if ((int)mod_short.size() > 22) mod_short = mod_short.substr(0, 21) + ".";
    DrawText(mod_short.c_str(), rx + 8, ry + 7, 12, { 140,230,160,255 });

    std::string mod_full = hit.module;
    if ((int)mod_full.size() > 36) mod_full = ".." + mod_full.substr(mod_full.size() - 34);
    DrawText(mod_full.c_str(), rx + 8, ry + 24, 9, { 80,150,95,200 });
    DrawText(("via " + hit.matched_code).c_str(), rx + 8, ry + 37, 9, { 70,120,80,180 });
    if (ch > 52) DrawText("[ click para navegar ]", rx + 8, ry + 50, 8, { 60,110,70,160 });

    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        auto found = find_node_by_code(state.mathlib_root, hit.module);
        if (found) state.pending_nav = { true, ViewMode::Mathlib,
            found->code, found->label, state.mathlib_root, found };
    }
}

static int draw_inverse_block(AppState& state, MathNode* sel,
    const std::vector<MathlibHit>& hits,
    int col, int y, int card_w, Vector2 mouse)
{
    Color hdr_col = (state.mode == ViewMode::MSC2020)
        ? Color{ 100,200,120,255 } : Color{ 200,180,100,255 };
    const char* lbl = (state.mode == ViewMode::MSC2020)
        ? "MODULOS MATHLIB RELACIONADOS (via MSC)"
        : "MODULOS MATHLIB RELACIONADOS (via Estandar)";
    DrawText(lbl, col, y, 11, hdr_col); y += 18;

    if (hits.empty()) {
        DrawText("No hay referencias cruzadas para esta area.", col, y, 11, { 80,90,110,200 });
        DrawText("Agrega entradas en crossref.json con los codigos de este nodo.",
            col, y + 16, 10, { 65,75,95,180 });
        y += 42;
    } else {
        const int ch = 64;
        int shown = std::min((int)hits.size(), 9);
        for (int i = 0; i < shown; i++)
            draw_mathlib_hit_card(state, hits[i],
                col + (i % 3) * (card_w + 10),
                y   + (i / 3) * (ch + 8),
                card_w, ch, mouse);
        y += ((shown + 2) / 3) * (ch + 8) + 4;
        if ((int)hits.size() > shown) {
            char more[64]; snprintf(more, sizeof(more), "... y %d modulos mas", (int)hits.size() - shown);
            DrawText(more, col, y, 10, { 80,120,90,200 }); y += 18;
        }
    }
    DrawLine(col, y, col + card_w * 3 + 20, y, { 30,30,50,200 }); y += 14;
    return y;
}

// ── Bloque referencias cruzadas (MSC / STD) ───────────────────────────────────

static int draw_crossrefs_block(AppState& state,
    const std::vector<std::string>& msc_refs,
    const std::vector<std::string>& std_refs,
    int col, int y, int card_w, int card_h, int w, Vector2 mouse)
{
    DrawText("REFERENCIAS CRUZADAS", col, y, 11, { 80,80,120,255 }); y += 18;
    int ci = 0;

    for (auto& code : msc_refs) {
        int rx = col + (ci % 3) * (card_w + 10), ry = y + (ci / 3) * (card_h + 10);
        bool hov = mouse.x > rx && mouse.x < rx + card_w && mouse.y > ry && mouse.y < ry + card_h;
        DrawRectangle(rx, ry, card_w, card_h, hov ? Color{ 25,45,80,255 } : Color{ 15,25,45,255 });
        DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h }, 1,
            hov ? Color{ 100,150,255,255 } : Color{ 40,60,110,200 });
        draw_chip("MSC", rx + card_w - 44, ry + 6, { 30,60,120,200 }, { 100,160,255,255 });
        DrawText(code.c_str(), rx + 10, ry + 10, 14, { 140,200,255,255 });
        DrawText("Ver en MSC2020", rx + 10, ry + 32, 10, { 90,130,180,200 });
        DrawText("[ click para navegar ]", rx + 10, ry + 48, 9, { 70,100,150,180 });
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto found = find_node_by_code(state.msc_root, code);
            if (found) state.pending_nav = { true, ViewMode::MSC2020,
                found->code, found->label, state.msc_root, found };
        }
        ci++;
    }
    for (auto& code : std_refs) {
        int rx = col + (ci % 3) * (card_w + 10), ry = y + (ci / 3) * (card_h + 10);
        bool hov = mouse.x > rx && mouse.x < rx + card_w && mouse.y > ry && mouse.y < ry + card_h;
        DrawRectangle(rx, ry, card_w, card_h, hov ? Color{ 30,50,35,255 } : Color{ 15,28,18,255 });
        DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h }, 1,
            hov ? Color{ 100,200,120,255 } : Color{ 40,90,55,200 });
        draw_chip("STD", rx + card_w - 44, ry + 6, { 30,80,45,200 }, { 100,200,120,255 });
        auto dot = code.rfind('.');
        std::string label = (dot != std::string::npos) ? code.substr(dot + 1) : code;
        DrawText(label.c_str(), rx + 10, ry + 10, 13, { 140,220,160,255 });
        DrawText(code.c_str(), rx + 10, ry + 28, 9, { 90,160,110,190 });
        DrawText("Ver en Estandar", rx + 10, ry + 42, 10, { 70,130,90,200 });
        DrawText("[ click para navegar ]", rx + 10, ry + 54, 9, { 60,110,75,180 });
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.standard_root) {
            auto found = find_node_by_code(state.standard_root, code);
            if (found) state.pending_nav = { true, ViewMode::Standard,
                found->code, found->label, state.standard_root, found };
        }
        ci++;
    }
    int rows = (ci + 2) / 3;
    y += rows * (card_h + 10) + 14;
    DrawLine(col, y, col + card_w * 3 + 20, y, { 30,30,50,200 }); y += 14;
    return y;
}

// ── Bloque recursos ───────────────────────────────────────────────────────────

struct CardStyle { const char* tag; Color kc; };
static CardStyle resource_style(const std::string& kind) {
    if (kind == "book")        return { "LIBRO", { 200,140,40,255  } };
    if (kind == "link")        return { "WEB",   { 60,150,220,255  } };
    if (kind == "latex")       return { "LEAN",  { 80,200,120,255  } };
    if (kind == "explanation") return { "NOTA",  { 140,140,200,255 } };
    return { "RES", { 80,80,130,255 } };
}

static void draw_resource_card(int rx, int ry, int cw, int ch,
    const char* title, const char* subtitle,
    CardStyle style, bool clickable, Vector2 mouse,
    bool& clicked)
{
    bool hov = mouse.x > rx && mouse.x < rx + cw && mouse.y > ry && mouse.y < ry + ch;
    DrawRectangle(rx, ry, cw, ch, hov ? Color{ 22,22,40,255 } : Color{ 17,17,30,255 });
    DrawRectangleLinesEx({ (float)rx,(float)ry,(float)cw,(float)ch }, 1,
        (hov && clickable) ? style.kc : Color{ 38,38,62,180 });
    int tw = MeasureText(style.tag, 9);
    DrawRectangle(rx + cw - tw - 12, ry + 6, tw + 10, 16, { style.kc.r,style.kc.g,style.kc.b,40 });
    DrawText(style.tag, rx + cw - tw - 7, ry + 8, 9, style.kc);
    DrawText(title,    rx + 10, ry + 10, 13, { 210,215,230,255 });
    DrawText(subtitle, rx + 10, ry + 30, 11, { 110,115,145,200 });
    clicked = hov && clickable && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static int draw_resources_block(AppState& state, MathNode* sel,
    int col, int y, int card_w, int card_h, Vector2 mouse)
{
    DrawText("RECURSOS", col, y, 11, { 80,80,120,255 }); y += 18;

    const std::vector<Resource>* resources = sel ? &sel->resources : nullptr;
    bool has_res = resources && !resources->empty();

    if (!has_res) {
        // Placeholders grises
        struct Ph { const char* tag, *title, *desc; Color kc; } ph[] = {
            {"LIBRO","Introduction",   "Texto introductorio",    {200,140,40,255}},
            {"WEB",  "MathSciNet",     "mathscinet.ams.org",     {60,150,220,255}},
            {"NOTA", "Nota tecnica",   "Descripcion formal",     {140,140,200,255}},
            {"WEB",  "zbMATH Open",    "zbmath.org",             {60,150,220,255}},
            {"PAPER","Survey article", "Estado del arte",        {180,100,200,255}},
            {"LEAN", "Def. formal",    "Formulacion en Lean",    {80,200,120,255}},
        };
        for (int i = 0; i < 6; i++) {
            int rx = col + (i % 3) * (card_w + 10), ry = y + (i / 3) * (card_h + 10);
            DrawRectangle(rx, ry, card_w, card_h, { 17,17,30,255 });
            DrawRectangleLinesEx({ (float)rx,(float)ry,(float)card_w,(float)card_h }, 1, { 38,38,62,180 });
            int tw = MeasureText(ph[i].tag, 9);
            DrawRectangle(rx + card_w - tw - 12, ry + 6, tw + 10, 16, { ph[i].kc.r,ph[i].kc.g,ph[i].kc.b,40 });
            DrawText(ph[i].tag,   rx + card_w - tw - 7, ry + 8,  9, ph[i].kc);
            DrawText(ph[i].title, rx + 10, ry + 10, 13, { 210,215,230,255 });
            DrawText(ph[i].desc,  rx + 10, ry + 30, 11, { 110,115,145,200 });
        }
        y += 2 * (card_h + 10) + 10;
    } else {
        int total = (int)resources->size();
        for (int i = 0; i < total; i++) {
            auto& res = (*resources)[i];
            auto style = resource_style(res.kind);
            int rx = col + (i % 3) * (card_w + 10), ry = y + (i / 3) * (card_h + 10);
            bool clicked = false;
            draw_resource_card(rx, ry, card_w, card_h,
                res.title.c_str(), res.content.substr(0, 42).c_str(),
                style, res.kind == "link", mouse, clicked);
            if (clicked) OpenURL(res.content.c_str());
        }
        y += ((total + 2) / 3) * (card_h + 10) + 10;
    }
    return y;
}

// ══════════════════════════════════════════════════════════════════════════════
// Panel principal
// ══════════════════════════════════════════════════════════════════════════════

void draw_info_panel(AppState& state, Vector2 mouse) {
    poll_latex_render(state);

    int top = g_split_y, w = SW(), vh = SH() - top;
    constexpr int HEADER_H = 100;
    int scroll_top = top + HEADER_H, scroll_h = vh - HEADER_H;

    // Fondo y divisor
    DrawRectangle(0, top, w, vh, { 10,10,18,255 });
    DrawLine(0, top, w, top, { 50,50,75,255 });

    draw_panel_header(state, top, w);
    DrawLine(0, top + HEADER_H, w, top + HEADER_H, { 35,35,55,255 });

    // Scroll con rueda
    if (mouse.y > scroll_top) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) {
            state.resource_scroll -= wh * 32.0f;
            if (state.resource_scroll < 0.0f) state.resource_scroll = 0.0f;
        }
    }

    // Nodo seleccionado
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) for (auto& child : cur->children)
        if (child->code == state.selected_code) { sel = child.get(); break; }

    // Referencias cruzadas e inverso
    std::vector<std::string> active_msc, active_std;
    collect_active_codes(state, sel, active_msc, active_std);

    bool show_inverse = (state.mode == ViewMode::MSC2020 || state.mode == ViewMode::Standard)
        && state.mathlib_root && !state.crossref_map.empty();

    std::vector<MathlibHit> inverse_hits;
    if (show_inverse) {
        std::string ctx = get_context_code(state, sel);
        auto search_codes = (state.mode == ViewMode::MSC2020) ? active_msc : active_std;
        if (!ctx.empty()) search_codes.push_back(ctx);
        bool use_msc = (state.mode == ViewMode::MSC2020);
        for (auto& code : search_codes) {
            for (auto& h : find_mathlib_hits(state.crossref_map, code, use_msc)) {
                bool dup = false;
                for (auto& e : inverse_hits) if (e.module == h.module) { dup = true; break; }
                if (!dup) inverse_hits.push_back(h);
            }
        }
    }

    // Cache de .tex
    static std::string cached_code, cached_raw, cached_display;
    std::string tex_target = sel ? sel->code : get_context_code(state, nullptr);
    if (tex_target != cached_code) {
        cached_code    = tex_target;
        cached_raw     = load_entry_tex(state.toolbar.entries_path, tex_target);
        cached_display = cached_raw.empty() ? "" : tex_to_display(cached_raw);
        auto& job = state.latex_render;
        if (job.tex_code != tex_target) {
            if (job.tex_loaded) { UnloadTexture(job.texture); job.texture = {}; job.tex_loaded = false; }
            job.state = LatexRenderState::Idle; job.tex_code = tex_target; job.error_msg.clear();
        }
    }

    // Layout
    const int col    = 18;
    const int card_w = (w - col * 2 - 10 * 2) / 3;
    const int card_h = 68;

    BeginScissorMode(0, scroll_top, w, scroll_h);
    int y = scroll_top + 14 - (int)state.resource_scroll;

    draw_sprite_preview(state, sel, w - 56 - col, y, 56);

    y = draw_description_block(state, sel, tex_target, cached_raw, cached_display,
            mouse, col, y, w - col * 2, scroll_h);

    y += 14;
    DrawLine(col, y, w - col, y, { 30,30,50,200 }); y += 14;

    if (show_inverse)
        y = draw_inverse_block(state, sel, inverse_hits, col, y, card_w, mouse);

    if (!active_msc.empty() || !active_std.empty())
        y = draw_crossrefs_block(state, active_msc, active_std, col, y, card_w, card_h, w, mouse);

    y = draw_resources_block(state, sel, col, y, card_w, card_h, mouse);

    // Clamp scroll
    float max_s = (float)std::max(0, y + (int)state.resource_scroll - scroll_h - scroll_top + 40);
    if (state.resource_scroll > max_s) state.resource_scroll = max_s;
    EndScissorMode();

    // Scrollbar vertical
    int full_h = y + (int)state.resource_scroll - scroll_top;
    if (full_h > scroll_h) {
        float ratio = (float)scroll_h / full_h;
        float bar_h = std::max(24.0f, ratio * scroll_h);
        float max_sv = (float)(full_h - scroll_h);
        float bar_y  = scroll_top + (state.resource_scroll / max_sv) * (scroll_h - bar_h);
        DrawRectangle(w - 6, scroll_top, 4, scroll_h, { 18,18,32,200 });
        DrawRectangle(w - 6, (int)bar_y, 4, (int)bar_h, { 70,70,120,255 });
    }
}
