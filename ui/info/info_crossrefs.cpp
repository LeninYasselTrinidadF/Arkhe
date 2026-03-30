#include "info_crossrefs.hpp"
#include "../core/font_manager.hpp"
#include "../core/theme.hpp"
#include "../core/skin.hpp"
#include "../constants.hpp"

#include <queue>
#include <algorithm>

// ── find_node_by_code (local) ─────────────────────────────────────────────────

static std::shared_ptr<MathNode>
find_node_by_code_local(const std::shared_ptr<MathNode>& root,
                        const std::string& code)
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
        {
            best = n; best_depth = n->code.size();
        }
        for (auto& c : n->children) q.push(c);
    }
    return best;
}

// ── find_mathlib_hits ─────────────────────────────────────────────────────────

std::vector<MathlibHit>
find_mathlib_hits(const std::unordered_map<std::string, CrossRef>& cmap,
                  const std::string& code,
                  bool use_msc)
{
    std::vector<MathlibHit> hits;
    if (code.empty()) return hits;
    for (auto& [mod, ref] : cmap) {
        auto& vec = use_msc ? ref.msc : ref.standard;
        for (auto& mc : vec) {
            bool match = (mc == code)
                || (mc.size() >= code.size() && mc.substr(0, code.size()) == code)
                || (code.size() >= mc.size() && code.substr(0, mc.size()) == mc);
            if (match) { hits.push_back({ mod, use_msc ? "MSC" : "STD", mc }); break; }
        }
    }
    return hits;
}

// ── collect_active_codes ──────────────────────────────────────────────────────

void collect_active_codes(AppState& state, MathNode* sel,
                          std::vector<std::string>& om,
                          std::vector<std::string>& os)
{
    auto try_node = [&](MathNode* n) -> bool {
        if (!n) return false;
        if (!n->msc_refs.empty() || !n->standard_refs.empty()) {
            om = n->msc_refs; os = n->standard_refs; return true;
        }
        auto it = state.crossref_map.find(n->code);
        if (it != state.crossref_map.end() &&
            (!it->second.msc.empty() || !it->second.standard.empty()))
        {
            om = it->second.msc; os = it->second.standard; return true;
        }
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
                bool m = (key.size() >= cand.size() &&
                          key.substr(0, cand.size()) == cand) ||
                         (cand.size() >= key.size() &&
                          cand.substr(0, key.size()) == key);
                if (m && (!ref.msc.empty() || !ref.standard.empty())) {
                    om = ref.msc; os = ref.standard; found = true; break;
                }
            }
            if (found) break;
        }
    }
}

// ── get_context_code ──────────────────────────────────────────────────────────

std::string get_context_code(AppState& state, MathNode* sel) {
    if (sel && !sel->code.empty() && sel->code != "ROOT") return sel->code;
    for (int i = (int)state.nav_stack.size() - 1; i >= 0; i--) {
        auto& n = state.nav_stack[i];
        if (n && !n->code.empty() && n->code != "ROOT") return n->code;
    }
    return "";
}

// ── Chip helper local ─────────────────────────────────────────────────────────

static void xref_chip(const char* text, int x, int y, Color bg, Color fg) {
    int tw = MeasureTextF(text, 11);
    int cw = tw + 14, ch = 20;
    if (g_skin.card.valid())
        g_skin.card.draw((float)x, (float)y, (float)cw, (float)ch, bg);
    else
        DrawRectangle(x, y, cw, ch, th_alpha(bg));
    DrawTextF(text, x + 7, y + 5, 11, fg);
}

// ── draw_mathlib_hit_card (privado) ───────────────────────────────────────────

static void draw_mathlib_hit_card(AppState& state,
                                  const MathlibHit& hit,
                                  int rx, int ry, int cw, int ch,
                                  Vector2 mouse)
{
    const Theme& th = g_theme;
    bool hov = (mouse.x > rx && mouse.x < rx + cw &&
                mouse.y > ry && mouse.y < ry + ch);

    Color bg     = hov ? Color{ th.success.r, th.success.g, th.success.b, 60 }
                       : th.bg_card;
    Color border = hov ? th.success : th_alpha(th.success_dim);

    if (g_skin.card.valid()) {
        g_skin.card.draw((float)rx, (float)ry, (float)cw, (float)ch, bg);
        DrawRectangleLinesEx({ (float)rx,(float)ry,(float)cw,(float)ch },
                             1.0f, border);
    } else {
        DrawRectangle(rx, ry, cw, ch, bg);
        DrawRectangleLinesEx({ (float)rx,(float)ry,(float)cw,(float)ch },
                             1.0f, border);
    }

    xref_chip("ML", rx + cw - 34, ry + 5,
               { th.success.r, th.success.g, th.success.b, 40 },
               th.success);

    std::string ms = hit.module;
    auto dot = ms.rfind('.');
    if (dot != std::string::npos) ms = ms.substr(dot + 1);
    if ((int)ms.size() > 22) ms = ms.substr(0, 21) + ".";
    DrawTextF(ms.c_str(), rx + 8, ry + 7, 12, th.success);

    std::string mf = hit.module;
    if ((int)mf.size() > 36) mf = ".." + mf.substr(mf.size() - 34);
    DrawTextF(mf.c_str(), rx + 8, ry + 24, 9, th_alpha(th.success_dim));

    DrawTextF(("via " + hit.matched_code).c_str(),
             rx + 8, ry + 37, 9, th_alpha(th.text_dim));
    if (ch > 52)
        DrawTextF("[ click para navegar ]", rx + 8, ry + 50, 8,
                 th_alpha(th.text_dim));

    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        auto found = find_node_by_code_local(state.mathlib_root, hit.module);
        if (found)
            state.pending_nav = { true, ViewMode::Mathlib,
                                  found->code, found->label,
                                  state.mathlib_root, found };
    }
}

// ── draw_inverse_block ────────────────────────────────────────────────────────

int draw_inverse_block(AppState& state,
                       MathNode* sel,
                       const std::vector<MathlibHit>& hits,
                       int col, int y, int card_w,
                       Vector2 mouse)
{
    const Theme& th = g_theme;
    Color hdr_col = (state.mode == ViewMode::MSC2020) ? th.success : th.warning;
    const char* lbl = (state.mode == ViewMode::MSC2020)
        ? "MODULOS MATHLIB RELACIONADOS (via MSC)"
        : "MODULOS MATHLIB RELACIONADOS (via Estandar)";
    DrawTextF(lbl, col, y, 11, hdr_col);
    y += 18;

    if (hits.empty()) {
        DrawTextF("No hay referencias cruzadas para esta area.",
                 col, y, 11, th_alpha(th.text_dim));
        DrawTextF("Agrega entradas en crossref.json con los codigos de este nodo.",
                 col, y + 16, 10, th_alpha(th.text_dim));
        y += 42;
    } else {
        const int ch = 64;
        int shown = std::min((int)hits.size(), 9);
        for (int i = 0; i < shown; i++) {
            draw_mathlib_hit_card(state, hits[i],
                col + (i % 3) * (card_w + 10),
                y   + (i / 3) * (ch + 8),
                card_w, ch, mouse);
        }
        y += ((shown + 2) / 3) * (ch + 8) + 4;
        if ((int)hits.size() > shown) {
            char more[64];
            snprintf(more, sizeof(more), "... y %d modulos mas",
                     (int)hits.size() - shown);
            DrawTextF(more, col, y, 10, th_alpha(th.text_dim));
            y += 18;
        }
    }

    DrawLine(col, y, col + card_w * 3 + 20, y, th_alpha(th.border));
    y += 14;
    return y;
}

// ── draw_crossrefs_block ──────────────────────────────────────────────────────

int draw_crossrefs_block(AppState& state,
                         const std::vector<std::string>& msc_refs,
                         const std::vector<std::string>& std_refs,
                         int col, int y,
                         int card_w, int card_h, int panel_w,
                         Vector2 mouse)
{
    const Theme& th = g_theme;
    DrawTextF("REFERENCIAS CRUZADAS", col, y, 11, th_alpha(th.text_dim));
    y += 18;
    int ci = 0;

    // Cards MSC
    for (auto& code : msc_refs) {
        int rx = col + (ci % 3) * (card_w + 10);
        int ry = y   + (ci / 3) * (card_h + 10);
        bool hov = (mouse.x > rx && mouse.x < rx + card_w &&
                    mouse.y > ry && mouse.y < ry + card_h);

        Color bg     = hov ? Color{ th.accent.r, th.accent.g, th.accent.b, 60 }
                           : th.bg_card;
        Color border = hov ? th.accent : th_alpha(th.border);

        if (g_skin.card.valid()) {
            g_skin.card.draw((float)rx,(float)ry,(float)card_w,(float)card_h, bg);
            DrawRectangleLinesEx({(float)rx,(float)ry,(float)card_w,(float)card_h},
                                 1.0f, border);
        } else {
            DrawRectangle(rx, ry, card_w, card_h, bg);
            DrawRectangleLinesEx({(float)rx,(float)ry,(float)card_w,(float)card_h},
                                 1, border);
        }

        xref_chip("MSC", rx + card_w - 44, ry + 6,
                   { th.accent.r, th.accent.g, th.accent.b, 40 },
                   th.accent_hover);
        DrawTextF(code.c_str(),     rx + 10, ry + 10, 14, th.accent_hover);
        DrawTextF("Ver en MSC2020", rx + 10, ry + 32, 10, th_alpha(th.text_secondary));
        DrawTextF("[ click ]",      rx + 10, ry + 48,  9, th_alpha(th.text_dim));

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            auto f = find_node_by_code_local(state.msc_root, code);
            if (f)
                state.pending_nav = { true, ViewMode::MSC2020,
                                      f->code, f->label,
                                      state.msc_root, f };
        }
        ci++;
    }

    // Cards STD
    for (auto& code : std_refs) {
        int rx = col + (ci % 3) * (card_w + 10);
        int ry = y   + (ci / 3) * (card_h + 10);
        bool hov = (mouse.x > rx && mouse.x < rx + card_w &&
                    mouse.y > ry && mouse.y < ry + card_h);

        Color bg     = hov ? Color{ th.success.r, th.success.g, th.success.b, 60 }
                           : th.bg_card;
        Color border = hov ? th.success : th_alpha(th.border);

        if (g_skin.card.valid()) {
            g_skin.card.draw((float)rx,(float)ry,(float)card_w,(float)card_h, bg);
            DrawRectangleLinesEx({(float)rx,(float)ry,(float)card_w,(float)card_h},
                                 1.0f, border);
        } else {
            DrawRectangle(rx, ry, card_w, card_h, bg);
            DrawRectangleLinesEx({(float)rx,(float)ry,(float)card_w,(float)card_h},
                                 1, border);
        }

        xref_chip("STD", rx + card_w - 44, ry + 6,
                   { th.success.r, th.success.g, th.success.b, 40 },
                   th.success);

        auto dot = code.rfind('.');
        std::string label = (dot != std::string::npos)
                          ? code.substr(dot + 1) : code;
        DrawTextF(label.c_str(),     rx + 10, ry + 10, 13, th.success);
        DrawTextF(code.c_str(),      rx + 10, ry + 28,  9, th_alpha(th.success_dim));
        DrawTextF("Ver en Estandar", rx + 10, ry + 42, 10, th_alpha(th.text_secondary));
        DrawTextF("[ click ]",       rx + 10, ry + 54,  9, th_alpha(th.text_dim));

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            state.standard_root)
        {
            auto f = find_node_by_code_local(state.standard_root, code);
            if (f)
                state.pending_nav = { true, ViewMode::Standard,
                                      f->code, f->label,
                                      state.standard_root, f };
        }
        ci++;
    }

    y += ((ci + 2) / 3) * (card_h + 10) + 14;
    DrawLine(col, y, col + card_w * 3 + 20, y, th_alpha(th.border));
    y += 14;
    return y;
}
