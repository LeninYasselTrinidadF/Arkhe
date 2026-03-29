#include "info_description.hpp"
#include "../core/theme.hpp"
#include "../core/skin.hpp"
#include "../constants.hpp"
#include "../data/math_node.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

// ── Helpers de texto ──────────────────────────────────────────────────────────

static int wrapped_text(const char* text, int x, int y,
                        int max_w, int font_size, Color color)
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
    case NodeLevel::Macro:
        return "Gran area matematica. Navega hacia las sub-areas para explorar.";
    case NodeLevel::Section:
        return "Seccion de la clasificacion. Contiene sub-areas especializadas.";
    case NodeLevel::Subsection:
        return "Sub-area especializada con temas de investigacion concretos.";
    case NodeLevel::Topic:
        return "Tema especifico de investigacion matematica con representacion en Mathlib4.";
    default:
        return "Selecciona una burbuja para ver su informacion detallada.";
    }
}

// ── IO de .tex ────────────────────────────────────────────────────────────────

std::string load_entry_tex(const std::string& entries_path,
                           const std::string& code)
{
    if (code.empty() || code == "ROOT") return "";
    std::string path = entries_path;
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
        path += '/';
    path += code + ".tex";
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string tex_to_display(const std::string& tex) {
    std::string out;
    out.reserve(tex.size());
    int brace = 0;
    for (size_t i = 0; i < tex.size(); i++) {
        char c = tex[i];
        if (c == '%') {
            while (i < tex.size() && tex[i] != '\n') i++;
            out += '\n';
            continue;
        }
        if (c == '\\') {
            size_t j = i + 1;
            while (j < tex.size() && isalpha((unsigned char)tex[j])) j++;
            std::string cmd = tex.substr(i + 1, j - i - 1);
            if (cmd == "section" || cmd == "subsection" || cmd == "subsubsection") { out += "\n\n"; i = j - 1; continue; }
            if (cmd == "textbf" || cmd == "textit" || cmd == "emph" || cmd == "text" || cmd == "mathrm") { i = j - 1; continue; }
            if (cmd == "newline" || cmd == "\\") { out += '\n'; i = j - 1; continue; }
            if (cmd == "item") { out += "\n  * "; i = j - 1; continue; }
            if (cmd == "begin" || cmd == "end") { i = j - 1; continue; }
            i = j - 1;
            continue;
        }
        if (c == '{') { brace++; out += c; continue; }
        if (c == '}') { if (brace > 0) { brace--; out += c; } continue; }
        if (c == '$') continue;
        out += c;
    }
    return out;
}

// ── LaTeX render ──────────────────────────────────────────────────────────────

static std::string temp_dir() {
    const char* t = getenv("TEMP");
    if (!t) t = getenv("TMP");
    if (!t) t = "/tmp";
    return std::string(t) + "/arkhe_latex/";
}

static std::string wrap_tex(const std::string& raw) {
    if (raw.find("\\documentclass") != std::string::npos) return raw;
    return "\\documentclass[12pt]{article}\n"
           "\\usepackage{amsmath,amssymb,amsthm}\n"
           "\\usepackage[margin=1.5cm]{geometry}\n"
           "\\pagestyle{empty}\n"
           "\\begin{document}\n"
           + raw +
           "\n\\end{document}\n";
}

void launch_latex_render(AppState& state,
                         const std::string& code,
                         const std::string& raw_tex)
{
    auto& job = state.latex_render;
    if (job.tex_loaded) {
        UnloadTexture(job.texture);
        job.texture    = {};
        job.tex_loaded = false;
    }
    job.tex_code    = code;
    job.state       = LatexRenderState::Compiling;
    job.error_msg.clear();
    job.thread_done = false;

    std::string td      = temp_dir();
    std::string src     = td + "entry.tex";
    std::string png_out = td + "entry";
    job.png_path = td + "entry-1.png";

    std::string lx = state.toolbar.latex_path;
    std::string pp = state.toolbar.pdftoppm_path;
    std::string w  = wrap_tex(raw_tex);

    std::thread([=, &job]() {
        try {
            fs::create_directories(td);
            {
                std::ofstream f(src);
                if (!f) {
                    job.error_msg = "No se pudo escribir " + src;
                    job.state     = LatexRenderState::Failed;
                    job.thread_done = true;
                    return;
                }
                f << w;
            }
            std::system(("\"" + lx + "\" -interaction=nonstopmode "
                         "-output-directory=\"" + td + "\" \"" + src +
                         "\" > nul 2>&1").c_str());
            if (!fs::exists(td + "entry.pdf")) {
                job.error_msg   = "pdflatex fallo. Verifica la ruta en Ubicaciones.";
                job.state       = LatexRenderState::Failed;
                job.thread_done = true;
                return;
            }
            std::system(("\"" + pp + "\" -r 144 -png \"" + td + "entry.pdf\" \"" +
                         png_out + "\" > nul 2>&1").c_str());
            if (!fs::exists(job.png_path)) {
                job.error_msg   = "pdftoppm no genero el PNG.";
                job.state       = LatexRenderState::Failed;
                job.thread_done = true;
                return;
            }
            job.state = LatexRenderState::Ready;
        } catch (std::exception& e) {
            job.error_msg = e.what();
            job.state     = LatexRenderState::Failed;
        }
        job.thread_done = true;
    }).detach();
}

void poll_latex_render(AppState& state) {
    auto& job = state.latex_render;
    if (job.state == LatexRenderState::Ready &&
        job.thread_done && !job.tex_loaded)
    {
        if (fs::exists(job.png_path)) {
            job.texture    = LoadTexture(job.png_path.c_str());
            job.tex_loaded = (job.texture.id != 0);
            if (!job.tex_loaded)
                job.error_msg = "LoadTexture fallo: " + job.png_path;
        }
    }
}

// ── Widget de LaTeX inline ────────────────────────────────────────────────────

static int draw_latex_widget(AppState& state, int x, int y,
                             int max_w, int max_h)
{
    const Theme& th = g_theme;
    auto& job = state.latex_render;

    if (job.state == LatexRenderState::Compiling) {
        double t   = GetTime();
        int    dots = (int)(t * 2.0) % 4;
        std::string msg = "Compilando LaTeX";
        for (int d = 0; d < dots; d++) msg += '.';
        DrawText(msg.c_str(), x, y, 12, th.success);
        float bw   = 180.0f;
        float fill = (float)(fmod(t * 0.5, 1.0)) * bw;
        DrawRectangle(x, y + 18, (int)bw, 4, th_alpha(th.bg_button));
        DrawRectangle(x, y + 18, (int)fill, 4, th.success);
        return y + 30;
    }
    if (job.state == LatexRenderState::Failed) {
        DrawText("Error al compilar LaTeX:", x, y, 11, { 220, 80, 80, 255 });
        y += 16;
        y = wrapped_text(
            (job.error_msg.empty() ? "Error desconocido." : job.error_msg).c_str(),
            x, y, max_w, 10, { 180, 80, 80, 220 });
        y += 6;
        DrawText("Verifica rutas en Ubicaciones > pdflatex / pdftoppm.",
                 x, y, 10, th_alpha(th.text_dim));
        return y + 16;
    }
    if (job.state == LatexRenderState::Ready && job.tex_loaded) {
        Texture2D& tex = job.texture;
        float scale = std::min({ (float)max_w / tex.width,
                                 (float)max_h / tex.height, 1.0f });
        int dw = (int)(tex.width  * scale);
        int dh = (int)(tex.height * scale);
        DrawRectangle(x - 2, y - 2, dw + 4, dh + 4, th.bg_panel);
        DrawRectangleLinesEx({ (float)(x - 2), (float)(y - 2),
                               (float)(dw + 4), (float)(dh + 4) },
                             1.0f, th_alpha(th.border_panel));
        DrawTexturePro(tex,
            { 0, 0, (float)tex.width, (float)tex.height },
            { (float)x, (float)y, (float)dw, (float)dh },
            { 0, 0 }, 0.0f, WHITE);
        return y + dh + 8;
    }
    return y;
}

// ── draw_description_block ────────────────────────────────────────────────────

int draw_description_block(AppState& state,
                           MathNode* sel,
                           const std::string& tex_target,
                           const std::string& cached_raw,
                           const std::string& cached_display,
                           Vector2 mouse,
                           int x, int y, int w, int scroll_h)
{
    const Theme& th = g_theme;

    DrawText("DESCRIPCION", x, y, 11, th_alpha(th.text_dim));
    y += 18;

    bool has_tex = !cached_display.empty();

    if (!has_tex) {
        std::string desc = (sel && !sel->note.empty())
                         ? sel->note
                         : default_description(sel);
        y = wrapped_text(desc.c_str(), x, y, w / 2 - 30, 13, th.text_primary);
    } else {
        // Chip ".tex"
        {
            int tw = MeasureText(".tex", 11);
            DrawRectangle(x, y, tw + 14, 20,
                          { th.success.r, th.success.g, th.success.b, 40 });
            DrawText(".tex", x + 7, y + 5, 11, th.success);
        }

        // Botón "Render LaTeX"
        auto& job = state.latex_render;
        bool can_render = (job.state == LatexRenderState::Idle   ||
                           job.state == LatexRenderState::Failed ||
                           job.state == LatexRenderState::Ready);
        const char* render_label =
            (job.state == LatexRenderState::Ready && job.tex_loaded)
            ? "Re-render LaTeX" : "Render LaTeX";

        int btn_x = x + MeasureText(".tex", 11) + 22;
        int btn_w = MeasureText(render_label, 10) + 16;
        Rectangle btn_r = { (float)btn_x, (float)y, (float)btn_w, 20.0f };
        bool btn_hov = CheckCollisionPointRec(mouse, btn_r);

        if (can_render) {
            Color bg_btn = btn_hov
                ? Color{ th.success.r, th.success.g, th.success.b, 255 }
                : Color{ th.success.r, th.success.g, th.success.b, 80 };
            if (g_skin.button.valid())
                g_skin.button.draw(btn_r, bg_btn);
            else {
                DrawRectangleRec(btn_r, bg_btn);
                DrawRectangleLinesEx(btn_r, 1.0f, th_alpha(th.success));
            }
            DrawText(render_label, btn_x + 8, y + 5, 10, th.text_primary);
            if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                launch_latex_render(state, tex_target, cached_raw);
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

        DrawText("TEXTO FUENTE (.tex):", x, y, 10, th_alpha(th.text_dim));
        y += 14;
        std::string preview = cached_display;
        if ((int)preview.size() > 600) preview = preview.substr(0, 597) + "...";
        y = wrapped_text(preview.c_str(), x, y, w - 30, 11, th.text_secondary);
    }
    return y;
}
