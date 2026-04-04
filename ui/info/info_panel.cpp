#include "ui/info/info_panel.hpp"
#include "ui/info/info_header.hpp"
#include "ui/info/info_description.hpp"
#include "ui/info/info_crossrefs.hpp"
#include "ui/info/info_resources.hpp"
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
    kbnav_info_begin_frame();   // ← NEW: resetea lista de ítems del frame anterior
    // 1) Polling del render LaTeX (debe ser lo primero del frame)
    poll_latex_render(state);

    const Theme& th = g_theme;
    int top = g_split_y;
    int w = SW();
    int vh = SH() - top;

    constexpr int HEADER_H = 100;
    int scroll_top = top + HEADER_H;
    int scroll_h = vh - HEADER_H;

    // 2) Fondo del panel inferior
    if (g_skin.panel.valid())
        g_skin.panel.draw(0.0f, (float)top, (float)w, (float)vh, th.bg_app);
    else
        DrawRectangle(0, top, w, vh, th.bg_app);

    DrawLine(0, top, w, top, th_alpha(th.split_vline));

    // 3) Cabecera (breadcrumb + título + chips)
    draw_info_header(state, top, w);
    DrawLine(0, top + HEADER_H, w, top + HEADER_H, th_alpha(th.border));

    // 4) Scroll con rueda
    if (mouse.y > scroll_top) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) {
            state.resource_scroll -= wh * 32.0f;
            if (state.resource_scroll < 0.0f) state.resource_scroll = 0.0f;
        }
    }

    // 5) Nodo seleccionado
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

    // 6) Refs cruzadas activas
    std::vector<std::string> active_msc, active_std;
    collect_active_codes(state, sel, active_msc, active_std);

    // 7) Hits inversos Mathlib
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

    // 8) Cache de .tex
    // ─────────────────────────────────────────────────────────────────────────
    // Problema anterior: load_entry_tex buscaba "<entries_path>/<code>.tex"
    // directamente, pero el editor guarda con safe_filename() y registra el
    // mapeo en entries_index.json. Ahora consultamos el índice primero.
    // ─────────────────────────────────────────────────────────────────────────
    static std::string cached_code;
    static std::string cached_raw;
    static std::string cached_display;
    // El índice se recarga solo cuando cambia el nodo (no cada frame).
    static std::unordered_map<std::string, std::string> s_index;

    std::string tex_target = sel ? sel->code
        : get_context_code(state, nullptr);

    if (tex_target != cached_code) {
        cached_code = tex_target;

        // Recargar índice para obtener el filename real
        editor_io::load_index(state, s_index);

        cached_raw.clear();
        if (!tex_target.empty() && tex_target != "ROOT") {
            auto it = s_index.find(tex_target);
            if (it != s_index.end()) {
                // Filename registrado por el editor (safe_filename)
                cached_raw = editor_io::read_tex(state, it->second);
            }
            // Si no está en el índice, el nodo no tiene .tex asociado → vacío
        }

        cached_display = cached_raw.empty() ? "" : tex_to_display(cached_raw);

        // Reset del job LaTeX al cambiar de nodo
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

    // 9) Área scrollable con scissor
    const int col = 18;
    const int card_w = (w - col * 2 - 10 * 2) / 3;
    const int card_h = 68;

    BeginScissorMode(0, scroll_top, w, scroll_h);
    int y = scroll_top + 14 - (int)state.resource_scroll;

    // Sprite preview (esquina derecha)
    draw_sprite_preview(state, sel, w - 56 - col, y, 56);

    // Descripción / LaTeX
    y = draw_description_block(state, sel, tex_target,
        cached_raw, cached_display,
        mouse, col, y, w - col * 2, scroll_h);
    y += 14;
    DrawLine(col, y, w - col, y, th_alpha(th.border));
    y += 14;

    // Proceso inverso
    if (show_inverse)
        y = draw_inverse_block(state, sel, inverse_hits, col, y, card_w, mouse);

    // Crossrefs
    if (!active_msc.empty() || !active_std.empty())
        y = draw_crossrefs_block(state, active_msc, active_std,
            col, y, card_w, card_h, w, mouse);

    // Recursos
    y = draw_resources_block(state, sel, col, y, card_w, card_h, mouse);

    // Clamp scroll
    float max_s = (float)std::max(0, y + (int)state.resource_scroll
        - scroll_h - scroll_top + 40);
    if (state.resource_scroll > max_s) state.resource_scroll = max_s;

    EndScissorMode();

    // ── Teclado zona Info ─────────────────────────────────────────────────────
    {
        const int SPLIT_MIN = TOOLBAR_H + 160;
        const int SPLIT_MAX = SH() - 120;
        kbnav_info_handle(state, SPLIT_MIN, SPLIT_MAX, g_split_y);
        kbnav_info_draw();
    }

    // 10) Scrollbar
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