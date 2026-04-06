#include "ui/info/info_panel.hpp"
#include "ui/info/info_header.hpp"
#include "ui/info/info_description.hpp"
#include "ui/info/info_crossrefs.hpp"
#include "ui/info/info_resources.hpp"
#include "ui/info/info_text_select.hpp"
#include "data/editor/editor_io.hpp"
#include "ui/aesthetic/overlay.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/skin.hpp"
#include "ui/constants.hpp"
#include "raylib.h"
#include "ui/key_controls/kbnav_info/kbnav_info.hpp"

#include <algorithm>
#include <string>

// ── draw_info_panel ───────────────────────────────────────────────────────────

void draw_info_panel(AppState& state, Vector2 mouse) {
    kbnav_info_begin_frame();
    g_info_sel.begin_frame();       // ← reset text entries cada frame
    poll_latex_render(state);

    const Theme& th = g_theme;
    int top = g_split_y;
    int w = SW();
    int vh = SH() - top;

    constexpr int HEADER_H = 100;
    int scroll_top = top + HEADER_H;
    int scroll_h = vh - HEADER_H;

    const bool info_blocked = overlay::blocks_mouse(mouse);
    const Vector2 eff_mouse = info_blocked
        ? Vector2{ -9999.f, -9999.f }
        : mouse;

    // Fondo del panel inferior
    if (g_skin.panel.valid())
        g_skin.panel.draw(0.0f, (float)top, (float)w, (float)vh, th.bg_app);
    else
        DrawRectangle(0, top, w, vh, th.bg_app);

    DrawLine(0, top, w, top, th_alpha(th.split_vline));

    // Cabecera (breadcrumb + título + chips) — solo visual, sin input
    draw_info_header(state, top, w);
    DrawLine(0, top + HEADER_H, w, top + HEADER_H, th_alpha(th.border));

    // Scroll con rueda
    if (!info_blocked && mouse.y > scroll_top) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) {
            state.resource_scroll -= wh * 32.0f;
            if (state.resource_scroll < 0.0f) state.resource_scroll = 0.0f;
        }
    }

    // Nodo seleccionado
    MathNode* cur = state.current();
    MathNode* sel = nullptr;
    if (cur) {
        for (auto& child : cur->children) {
            if (child->code == state.selected_code) {
                sel = child.get();
                break;
            }
        }
    }

    // Refs cruzadas activas
    std::vector<std::string> active_msc, active_std;
    collect_active_codes(state, sel, active_msc, active_std);

    // Hits inversos Mathlib
    bool show_inverse = (state.mode == ViewMode::MSC2020 ||
        state.mode == ViewMode::Standard) &&
        state.mathlib_root &&
        !state.crossref_map.empty();
    std::vector<MathlibHit> inverse_hits;
    if (show_inverse) {
        std::string ctx = get_context_code(state, sel);
        auto search_codes = (state.mode == ViewMode::MSC2020)
            ? active_msc : active_std;
        if (!ctx.empty()) search_codes.push_back(ctx);
        bool use_msc = (state.mode == ViewMode::MSC2020);
        for (auto& code : search_codes) {
            for (auto& h : find_mathlib_hits(state.crossref_map, code, use_msc)) {
                bool dup = false;
                for (auto& e : inverse_hits)
                    if (e.module == h.module) { dup = true; break; }
                if (!dup) inverse_hits.push_back(h);
            }
        }
    }

    // Cache de .tex
    static std::string cached_code;
    static std::string cached_raw;
    static std::string cached_display;
    static std::unordered_map<std::string, std::string> s_index;

    std::string tex_target = sel ? sel->code
        : get_context_code(state, nullptr);

    if (tex_target != cached_code) {
        cached_code = tex_target;
        editor_io::load_index(state, s_index);
        cached_raw.clear();
        if (!tex_target.empty() && tex_target != "ROOT") {
            auto it = s_index.find(tex_target);
            if (it != s_index.end())
                cached_raw = editor_io::read_tex(state, it->second);
        }
        cached_display = cached_raw.empty() ? "" : tex_to_display(cached_raw);

        auto& job = state.latex_render;
        if (job.tex_code != tex_target) {
            if (job.tex_loaded) {
                UnloadTexture(job.texture);
                job.texture = {};
                job.tex_loaded = false;
            }
            job.state = LatexRenderState::Idle;
            job.tex_code = tex_target;
            job.error_msg.clear();
        }
    }

    // ── Área scrollable con scissor ───────────────────────────────────────────
    const int col = 18;
    const int card_w = (w - col * 2 - 10 * 2) / 3;
    const int card_h = 68;

    BeginScissorMode(0, scroll_top, w, scroll_h);
    int y = scroll_top + 14 - (int)state.resource_scroll;

    draw_sprite_preview(state, sel, w - 56 - col, y, 56);

    y = draw_description_block(state, sel, tex_target,
        cached_raw, cached_display,
        eff_mouse, col, y, w - col * 2, scroll_h);
    y += 14;
    DrawLine(col, y, w - col, y, th_alpha(th.border));
    y += 14;

    if (show_inverse)
        y = draw_inverse_block(state, sel, inverse_hits,
            col, y, card_w, eff_mouse);

    if (!active_msc.empty() || !active_std.empty())
        y = draw_crossrefs_block(state, active_msc, active_std,
            col, y, card_w, card_h, w, eff_mouse);

    y = draw_resources_block(state, sel, col, y, card_w, card_h, eff_mouse);

    float max_s = (float)std::max(0, y + (int)state.resource_scroll
        - scroll_h - scroll_top + 40);
    if (state.resource_scroll > max_s) state.resource_scroll = max_s;

    // ── Selección de texto: highlights y rect de arrastre ─────────────────────
    // Se dibuja aquí, dentro del scissor, para que el clip sea correcto.
    g_info_sel.draw();

    EndScissorMode();

    // ── Input de texto: drag, Ctrl+C, Ctrl+A ──────────────────────────────────
    // Fuera del scissor para poder leer el estado de mouse correctamente,
    // pero los efectos visuales ya se manejaron arriba.
    g_info_sel.handle(mouse, scroll_top, info_blocked);

    // ── Teclado zona Info ─────────────────────────────────────────────────────
    {
        const int SPLIT_MIN = TOOLBAR_H + 160;
        const int SPLIT_MAX = SH() - 120;
        kbnav_info_handle(state, SPLIT_MIN, SPLIT_MAX, g_split_y);
        kbnav_info_draw();
    }

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    int full_h = y + (int)state.resource_scroll - scroll_top;
    if (full_h > scroll_h) {
        float ratio = (float)scroll_h / full_h;
        float bar_h = std::max(24.0f, ratio * scroll_h);
        float max_sv = (float)(full_h - scroll_h);
        float bar_y = scroll_top +
            (state.resource_scroll / max_sv) * (scroll_h - bar_h);
        DrawRectangle(w - 6, scroll_top, 4, scroll_h, th_alpha(th.scrollbar_bg));
        DrawRectangle(w - 6, (int)bar_y, 4, (int)bar_h, th.scrollbar_thumb);
    }
}
