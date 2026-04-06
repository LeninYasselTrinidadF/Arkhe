#include "ui/info/info_text_select.hpp"
#include "ui/aesthetic/theme.hpp"
#include "ui/aesthetic/font_manager.hpp" // DrawTextF, MeasureTextF
#include "ui/constants.hpp"            // g_split_y
#include "ui/key_controls/keyboard_nav.hpp"  // g_kbnav, FocusZone
#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <sstream>

InfoTextSelect g_info_sel;

// ── begin_frame ───────────────────────────────────────────────────────────────

void InfoTextSelect::begin_frame() {
    entries.clear();
    if (copy_flash > 0) copy_flash--;
}

// ── register_entry ────────────────────────────────────────────────────────────

void InfoTextSelect::register_entry(const char* text, int x, int y, int w, int h) {
    if (!text || text[0] == '\0') return;
    entries.push_back({ std::string(text),
                        { (float)x, (float)y, (float)w, (float)h },
                        false });
}

// ── sel_rect ──────────────────────────────────────────────────────────────────

Rectangle InfoTextSelect::sel_rect() const {
    return {
        std::min(drag_start.x, drag_cur.x),
        std::min(drag_start.y, drag_cur.y),
        std::abs(drag_cur.x - drag_start.x),
        std::abs(drag_cur.y - drag_start.y)
    };
}

// ── sel_text / all_text ───────────────────────────────────────────────────────

std::string InfoTextSelect::sel_text() const {
    std::string out;
    for (auto& e : entries) {
        if (e.selected) {
            if (!out.empty()) out += '\n';
            out += e.text;
        }
    }
    return out;
}

std::string InfoTextSelect::all_text() const {
    std::string out;
    for (auto& e : entries) {
        if (!out.empty()) out += '\n';
        out += e.text;
    }
    return out;
}

// ── handle ────────────────────────────────────────────────────────────────────

void InfoTextSelect::handle(Vector2 mouse, int scroll_top, bool blocked) {
    // ── Bloqueo por modal ─────────────────────────────────────────────────────
    if (blocked) {
        drag_started = dragging = false;
        return;
    }

    bool in_info = (mouse.y >= (float)scroll_top);

    // ── Inicio de drag ────────────────────────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && in_info) {
        drag_started = true;
        dragging     = false;
        drag_start   = drag_cur = mouse;
    }

    // ── Movimiento: activar drag tras umbral de 8 px ──────────────────────────
    if (drag_started && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        drag_cur = mouse;
        float d2 = (drag_cur.x - drag_start.x) * (drag_cur.x - drag_start.x)
                 + (drag_cur.y - drag_start.y) * (drag_cur.y - drag_start.y);
        if (!dragging && d2 > 64.f) {  // 8 px
            dragging = true;
            has_sel  = false;
            for (auto& e : entries) e.selected = false;
        }
    }

    // ── Soltar botón ──────────────────────────────────────────────────────────
    if (drag_started && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (dragging) {
            // Finalizar selección: marcar entradas que intersectan el rect
            Rectangle r = sel_rect();
            has_sel = true;
            for (auto& e : entries)
                e.selected = (r.width > 2.f || r.height > 2.f)
                          && CheckCollisionRecs(e.rect, r);

            // Si ningún item quedó seleccionado, limpiar
            bool any = false;
            for (auto& e : entries) if (e.selected) { any = true; break; }
            if (!any) has_sel = false;
        } else {
            // Click simple sin arrastrar → limpiar selección
            has_sel = false;
            for (auto& e : entries) e.selected = false;
        }
        drag_started = dragging = false;
    }

    // ── Ctrl+A: seleccionar todo ──────────────────────────────────────────────
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool in_zone = in_info || g_kbnav.in(FocusZone::Info);

    if (ctrl && IsKeyPressed(KEY_A) && in_zone) {
        has_sel = true;
        for (auto& e : entries) e.selected = true;
    }

    // ── Ctrl+C: copiar al portapapeles ────────────────────────────────────────
    if (ctrl && IsKeyPressed(KEY_C) && in_zone) {
        std::string text = has_sel ? sel_text() : all_text();
        if (!text.empty()) {
            SetClipboardText(text.c_str());
            copy_flash = 90;  // ~1.5 s a 60 fps
        }
    }
}

// ── draw ──────────────────────────────────────────────────────────────────────

void InfoTextSelect::draw() const {
    const Theme& th = g_theme;

    // ── Highlights de items seleccionados ─────────────────────────────────────
    for (auto& e : entries) {
        if (!e.selected) continue;
        Rectangle r = { e.rect.x - 2.f, e.rect.y - 1.f,
                        e.rect.width + 4.f, e.rect.height + 2.f };
        DrawRectangleRec(r, ColorAlpha(th.accent, 0.28f));
    }

    // ── Rectángulo de arrastre activo ─────────────────────────────────────────
    if (dragging) {
        Rectangle r = sel_rect();
        if (r.width > 2.f || r.height > 2.f) {
            DrawRectangleRec(r, ColorAlpha(th.accent, 0.12f));
            DrawRectangleLinesEx(r, 1.0f, ColorAlpha(th.accent, 0.55f));
        }
    }

    // ── Feedback "copiado" ────────────────────────────────────────────────────
    if (copy_flash > 0) {
        float alpha = std::min(1.f, copy_flash / 30.f);
        DrawTextF("✓ Copiado", 18, g_split_y + 4, 10,
                  ColorAlpha(th.success, alpha));
    }
}
