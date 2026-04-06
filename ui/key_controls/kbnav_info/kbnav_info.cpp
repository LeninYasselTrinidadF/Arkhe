#include "ui/key_controls/kbnav_info/kbnav_info.hpp"
#include "ui/key_controls/keyboard_nav.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "app.hpp"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <queue>

// ── Estado global ─────────────────────────────────────────────────────────────

KbInfoState g_kbinfo;

// ── Helpers de navegación a nodo ─────────────────────────────────────────────

static std::shared_ptr<MathNode>
find_node_bfs(const std::shared_ptr<MathNode>& root, const std::string& code) {
    if (!root || code.empty()) return nullptr;
    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (n->code == code) return n;
        for (auto& c : n->children) q.push(c);
    }
    return nullptr;
}

// ── kbnav_info_begin_frame ────────────────────────────────────────────────────

void kbnav_info_begin_frame() {
    g_kbinfo.clear();
}

// ── kbnav_info_register_* ─────────────────────────────────────────────────────

void kbnav_info_register_render(Rectangle r) {
    g_kbinfo.push({ InfoItemKind::RenderButton, r, "" });
}

void kbnav_info_register_resource(Rectangle r, const std::string& url_or_code,
    bool is_web_link) {
    InfoItemKind k = is_web_link ? InfoItemKind::ResourceLink
                                 : InfoItemKind::CrossrefCard;
    g_kbinfo.push({ k, r, url_or_code });
}

void kbnav_info_register_crossref(Rectangle r, const std::string& code,
    InfoItemKind kind) {
    g_kbinfo.push({ kind, r, code });
}

// ── kbnav_info_handle ─────────────────────────────────────────────────────────

bool kbnav_info_handle(AppState& state, int split_y_min, int split_y_max,
    int& g_split_y)
{
    if (!g_kbnav.in(FocusZone::Info)) return false;

    if (state.toolbar.ubicaciones_open || state.toolbar.docs_open
        || state.toolbar.editor_open   || state.toolbar.config_open)
        return false;

    bool consumed = false;
    int  n = g_kbinfo.count();

    // ── Espacio: expandir / contraer info_panel ───────────────────────────────
    if (IsKeyPressed(KEY_SPACE)) {
        g_split_y = g_kbinfo.expanded ? (split_y_min + split_y_max) / 2
                                      : split_y_min;
        g_kbinfo.expanded = !g_kbinfo.expanded;
        consumed = true;
    }

    // ── Up/Down: ciclar ítems ─────────────────────────────────────────────────
    if (n > 0) {
        if (IsKeyPressed(KEY_UP)) {
            g_kbnav.info_idx = (g_kbnav.info_idx - 1 + n) % n;
            consumed = true;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            g_kbnav.info_idx = (g_kbnav.info_idx + 1) % n;
            consumed = true;
        }

        g_kbnav.info_idx = std::clamp(g_kbnav.info_idx, 0, n - 1);

        // ── Enter: activar ítem ───────────────────────────────────────────────
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            const InfoNavItem* it = g_kbinfo.current(g_kbnav.info_idx);
            if (it) {
                switch (it->kind) {

                case InfoItemKind::RenderButton:
                {
                    auto& job = state.latex_render;
                    if (job.state == LatexRenderState::Idle ||
                        job.state == LatexRenderState::Failed)
                        job.kb_trigger = true;
                }
                consumed = true;
                break;

                case InfoItemKind::ResourceLink:
                    if (!it->data.empty())
                        OpenURL(it->data.c_str());
                    consumed = true;
                    break;

                case InfoItemKind::CrossrefCard:
                    // Navegar al nodo MSC
                {
                    auto f = find_node_bfs(state.msc_root, it->data);
                    if (f)
                        state.pending_nav = { true, ViewMode::MSC2020,
                                              f->code, f->label,
                                              state.msc_root, f };
                }
                consumed = true;
                break;

                case InfoItemKind::StdrefCard:
                    // Navegar al nodo Standard
                {
                    if (state.standard_root) {
                        auto f = find_node_bfs(state.standard_root, it->data);
                        if (f)
                            state.pending_nav = { true, ViewMode::Standard,
                                                  f->code, f->label,
                                                  state.standard_root, f };
                    }
                }
                consumed = true;
                break;

                case InfoItemKind::MathlibHitCard:
                    // Navegar al módulo Mathlib
                {
                    auto f = find_node_bfs(state.mathlib_root, it->data);
                    if (f)
                        state.pending_nav = { true, ViewMode::Mathlib,
                                              f->code, f->label,
                                              state.mathlib_root, f };
                }
                consumed = true;
                break;

                default: break;
                }
            }
        }
    }

    return consumed;
}

// ── kbnav_info_draw ───────────────────────────────────────────────────────────

void kbnav_info_draw() {
    if (!g_kbnav.in(FocusZone::Info)) return;
    if (g_kbinfo.count() == 0) return;

    int idx = std::clamp(g_kbnav.info_idx, 0, g_kbinfo.count() - 1);
    const InfoNavItem& it = g_kbinfo.items[idx];

    const Theme& th = g_theme;
    float t = (float)GetTime();
    float alpha = 0.55f + 0.45f * sinf(t * 5.f);

    DrawRectangleLinesEx(it.rect, 2.f, ColorAlpha(th.accent, alpha));
    DrawRectangleLinesEx(
        { it.rect.x - 3.f, it.rect.y - 3.f,
          it.rect.width + 6.f, it.rect.height + 6.f },
        1.f, ColorAlpha(th.accent, alpha * 0.30f));

    const char* hint = nullptr;
    switch (it.kind) {
    case InfoItemKind::RenderButton:   hint = "Enter: render";   break;
    case InfoItemKind::ResourceLink:   hint = "Enter: abrir";    break;
    case InfoItemKind::CrossrefCard:   hint = "Enter: ir a MSC"; break;
    case InfoItemKind::StdrefCard:     hint = "Enter: ir a STD"; break;
    case InfoItemKind::MathlibHitCard: hint = "Enter: navegar";  break;
    default: break;
    }
    if (hint) {
        int hw = MeasureTextF(hint, 9);
        DrawTextF(hint,
            (int)(it.rect.x + it.rect.width  - hw - 4),
            (int)(it.rect.y + it.rect.height - 12),
            9, ColorAlpha(th.accent, 0.75f));
    }
}
