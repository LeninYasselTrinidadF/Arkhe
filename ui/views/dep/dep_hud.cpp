#include "ui/views/dep/dep_hud.hpp"
#include "ui/views/dep/dep_sim.hpp"
#include "ui/views/dep/dep_draw.hpp"
#include "ui/views/dep/dep_pivot.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "raylib.h"
#include <cstdio>

void dep_hud_draw(const AppState& state, bool shift_held) {
    const Theme& th  = g_theme;
    const int    cw  = CANVAS_W();
    const DepGraph& use_graph = get_dep_graph_for_const(state);

    // ── Barra superior ────────────────────────────────────────────────────────
    DrawRectangle(0, UI_TOP(), cw, 48, ColorAlpha(th.bg_app, 0.88f));
    DrawLineEx({ 0.f, (float)(UI_TOP() + 48) }, { (float)cw, (float)(UI_TOP() + 48) },
        1.f, ColorAlpha(th.ctrl_border, 0.4f));

    // Título: foco actual (+ pivote si está activo)
    {
        const DepNode* fn  = use_graph.get(s_focus_id);
        std::string title  = fn
            ? dep_safe_trunc(fn->label, 36) : dep_safe_trunc(s_focus_id, 36);
        std::string full   = s_focus_id + "  \xE2\x80\x94  " + title;

        if (g_dep_pivot.active) {
            const DepNode* pn = use_graph.get(g_dep_pivot.pivot_id);
            std::string plbl  = pn
                ? dep_safe_trunc(pn->label, 24)
                : dep_safe_trunc(g_dep_pivot.pivot_id, 24);
            full += "   |   \xF0\x9F\x94\xB4 " + g_dep_pivot.pivot_id + " " + plbl;
        }

        int tw     = MeasureTextF(full.c_str(), 15);
        int text_x = 170 + (cw - 170 - 190 - tw) / 2;
        DrawTextF(full.c_str(), text_x, UI_TOP() + 15, 15, th.ctrl_text);
    }

    // ── Leyenda de colores ────────────────────────────────────────────────────
    {
        int lx = cw - 180, ly = UI_TOP() + 8;
        auto dot = [&](Color c, const char* label, int row) {
            DrawCircle(lx + 7, ly + 7 + row * 18, 5.f, c);
            DrawTextF(label, lx + 16, ly + row * 18, 12,
                ColorAlpha(th.ctrl_text, 0.75f));
        };
        dot(th.success,                       "Upstream",   0);
        dot(th.accent,                        "Downstream", 1);
        dot(ColorAlpha(th.ctrl_border, 0.8f), "Otro",       2);
        if (g_dep_pivot.active)
            dot({ 220, 50, 50, 230 },         "Pivote",     3);
    }

    // ── Barra de simulación ───────────────────────────────────────────────────
    if (s_sim_step < SIM_MAX) {
        float pct = (float)s_sim_step / SIM_MAX;
        int bw = 120, bh = 6, bx = 12, by = g_split_y - 20;
        DrawRectangle(bx, by, bw, bh, ColorAlpha(th.ctrl_bg, 0.7f));
        DrawRectangle(bx, by, (int)(bw * pct), bh, ColorAlpha(th.accent, 0.8f));
        DrawTextF("simulando...", bx, by - 14, 11, ColorAlpha(th.text_dim, 0.6f));
    }

    // ── Contador de nodos ─────────────────────────────────────────────────────
    {
        char info[80];
        snprintf(info, sizeof(info), "%d nodos totales  |  %zu en vista",
            (int)use_graph.size(), s_phys.size());
        int iw = MeasureTextF(info, 12);
        DrawTextF(info, cw - iw - 10, g_split_y - 18, 12,
            ColorAlpha(th.text_dim, 0.55f));
    }

    // ── Hint de controles ─────────────────────────────────────────────────────
    {
        const char* hint;
        if (state.position_mode_active) {
            hint = "Modo posicion: arrastra nodos  |  Clic en 'Posicion' para salir";
        } else if (g_dep_pivot.active && !shift_held) {
            static char pivot_hint[140];
            int slots = 0;
            for (auto& r : g_dep_pivot.rings)
                if (r.ring_idx == g_dep_pivot.cur_ring) { slots = (int)r.slots.size(); break; }
            snprintf(pivot_hint, sizeof(pivot_hint),
                "CTRL — Anillo %+d | Slot %d/%d  |  Flechas: navegar  "
                "|  Enter: re-pivotar  |  Suelta Ctrl: salir",
                g_dep_pivot.cur_ring, g_dep_pivot.cur_slot + 1, slots);
            hint = pivot_hint;
        } else if (shift_held) {
            hint = "SHIFT — Flechas: mover camara  |  Ctrl+Shift: pivote visual sin navegar";
        } else if (g_kbnav.in(FocusZone::Canvas)) {
            hint = "Flechas: navegar  |  Shift: mover camara  |  Ctrl: pivote  |  Enter: re-enfocar";
        } else {
            hint = "Clic: re-enfocar  |  Shift+Flechas: mover camara  |  Rueda: zoom  |  Ctrl: pivote";
        }
        int hw = MeasureTextF(hint, 11);
        DrawTextF(hint, cw / 2 - hw / 2, g_split_y - 18, 11,
            ColorAlpha(th.text_dim, 0.4f));
    }
}
