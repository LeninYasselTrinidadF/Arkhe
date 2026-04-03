/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 1 – search_panel.cpp
   Cambios a aplicar manualmente (los bloques son exactos del archivo original)
   ═══════════════════════════════════════════════════════════════════════════ */

// ── (a) Añadir include al principio ──────────────────────────────────────────
// Junto a los demás includes del archivo:
#include "key_controls/kbnav_search.hpp"

// ── (b) Primeras líneas de draw_search_panel ─────────────────────────────────
// ORIGINAL:
void draw_search_panel(AppState& state, const MathNode* search_root, Vector2 mouse) {
    const Theme& th = g_theme;
    const int SB_W = 7;
    // ...

// NUEVO (añadir justo después de las declaraciones de constantes locales,
//        ANTES del primer dibujo):
    // Procesar teclado zona Search (escribe en buffers, lanza loogle si Enter)
    kbnav_search_handle(state, search_root);

// ── (c) Capturar field_y_local ────────────────────────────────────────────────
// ORIGINAL (sección local fuzzy, donde se dibuja el campo):
    int field_y = y;
    draw_search_field(px, y, pw, field_h, state.search_buf);

// NUEVO:
    int field_y_local = y;   // ← NEW: coordenada Y para indicador KB
    int field_y = y;
    draw_search_field(px, y, pw, field_h, state.search_buf);

// ── (d) Inhibir captura de teclado cuando kbnav controla la zona ─────────────
// ORIGINAL (bloque if(panel_active) de captura de teclado local):
    if (panel_active) {
        static bool local_focus = true;
        Rectangle lf = { (float)px, (float)field_y, (float)pw, (float)field_h };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            local_focus = CheckCollisionPointRec(mouse, lf);
        if (local_focus) {
            // ... lectura de chars ...
        }
    }

// NUEVO: añadir al inicio del if(panel_active):
    if (panel_active) {
        static bool local_focus = true;
        // Cuando kbnav controla la zona, kbnav_search_handle ya escribió
        // en los buffers → cedemos el foco interno para no duplicar entrada
        if (g_kbnav.in(FocusZone::Search)) { local_focus = false; }
        else {
            Rectangle lf = { (float)px, (float)field_y, (float)pw, (float)field_h };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                local_focus = CheckCollisionPointRec(mouse, lf);
        }
        if (local_focus && !g_kbnav.in(FocusZone::Search)) {
            // ... lectura de chars (sin cambios) ...
        }
    }

// ── (e) Capturar field_y_loogle y loogle_btn_x ───────────────────────────────
// ORIGINAL (sección loogle):
    int btn_w = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    int lfld_y = ly;
    draw_search_field(px, ly, pw - btn_w - 4, field_h, state.loogle_buf);
    Rectangle btn = { (float)(px + pw - btn_w), (float)ly, (float)btn_w, (float)field_h };

// NUEVO: añadir captura:
    int btn_w = MeasureTextF("Buscar", 12) + g_fonts.scale(20);
    int lfld_y = ly;
    int field_y_loogle = ly;          // ← NEW
    int loogle_btn_x   = px + pw - btn_w; // ← NEW
    draw_search_field(px, ly, pw - btn_w - 4, field_h, state.loogle_buf);
    Rectangle btn = { (float)loogle_btn_x, (float)ly, (float)btn_w, (float)field_h };

// ── (f) Inhibir captura de teclado loogle cuando kbnav activo ────────────────
// ORIGINAL:
    if (panel_active) {
        static bool loogle_focus = false;
        Rectangle lf = { ... };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            loogle_focus = CheckCollisionPointRec(mouse, lf);
        if (loogle_focus) { ... }
        if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ...) { ... }
    }

// NUEVO: al inicio del bloque añadir:
    if (panel_active) {
        static bool loogle_focus = false;
        if (g_kbnav.in(FocusZone::Search)) { loogle_focus = false; }
        else {
            Rectangle lf = { ... };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                loogle_focus = CheckCollisionPointRec(mouse, lf);
        }
        if (loogle_focus && !g_kbnav.in(FocusZone::Search)) { ... }
        // botón Buscar (sin cambios)
    }

// ── (g) Dibujar indicador al final de draw_search_panel ──────────────────────
// Añadir como ÚLTIMA instrucción antes del cierre de la función:
    kbnav_search_draw(px, pw,
                      field_y_local,
                      field_y_loogle,
                      field_h,
                      loogle_btn_x, btn_w);
}


/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 2 – info_panel.cpp
   ═══════════════════════════════════════════════════════════════════════════ */

// ── (a) Include ───────────────────────────────────────────────────────────────
#include "key_controls/kbnav_info.hpp"

// ── (b) Inicio de draw_info_panel ─────────────────────────────────────────────
// Añadir como PRIMERA línea del cuerpo de la función:
void draw_info_panel(AppState& state, Vector2 mouse) {
    kbnav_info_begin_frame();   // ← NEW: resetea lista de ítems del frame anterior
    poll_latex_render(state);
    // ... resto sin cambios ...

// ── (c) Después de EndScissorMode(), antes del scrollbar ──────────────────────
// ORIGINAL:
    EndScissorMode();

    // 10) Scrollbar
    int full_h = ...

// NUEVO:
    EndScissorMode();

    // ── Teclado zona Info ─────────────────────────────────────────────────────
    {
        const int SPLIT_MIN = TOOLBAR_H + 160;
        const int SPLIT_MAX = SH() - 120;
        kbnav_info_handle(state, SPLIT_MIN, SPLIT_MAX, g_split_y);
        kbnav_info_draw();
    }

    // 10) Scrollbar (sin cambios)
    int full_h = ...


/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 3 – info_description.cpp  (consumir kb_trigger)
   ═══════════════════════════════════════════════════════════════════════════ */

// ── (a) Include ───────────────────────────────────────────────────────────────
#include "key_controls/kbnav_info.hpp"

// ── (b) Tras dibujar btn_r, registrarlo y consumir kb_trigger ────────────────
// ORIGINAL:
            if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                launch_latex_render(state, tex_target, cached_raw);

// NUEVO:
            kbnav_info_register_render(btn_r);   // ← registrar para teclado

            bool kb_fire = state.latex_render.kb_trigger;
            state.latex_render.kb_trigger = false;
            if ((btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || kb_fire)
                launch_latex_render(state, tex_target, cached_raw);


/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 4 – info_resources.cpp  (registrar tarjetas)
   ═══════════════════════════════════════════════════════════════════════════ */

// ── (a) Include ───────────────────────────────────────────────────────────────
#include "key_controls/kbnav_info.hpp"

// ── (b) Bucle de recursos reales – al final de cada iteración ─────────────────
// ORIGINAL (dentro del bloque else, bucle for sobre *res):
            if (r.kind == "link" && hov &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                OpenURL(r.content.c_str());
        }

// NUEVO: añadir ANTES del cierre de llaves del bucle:
            if (r.kind == "link" && hov &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                OpenURL(r.content.c_str());

            // Registrar para navegación por teclado
            {
                Rectangle cr = { (float)rx, (float)ry, (float)card_w, (float)card_h };
                bool is_web = (r.kind == "link");
                kbnav_info_register_resource(cr, r.content, is_web);
            }
        }


/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 5 – info_crossrefs.cpp  (registrar tarjetas crossref y mathlib)
   ═══════════════════════════════════════════════════════════════════════════ */

// ── (a) Include ───────────────────────────────────────────────────────────────
#include "key_controls/kbnav_info.hpp"

// ── (b) draw_mathlib_hit_card – al final, justo antes del if(hov&&click) ──────
// ORIGINAL:
    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ... }
}

// NUEVO (añadir antes del if):
    // Registrar para teclado
    {
        Rectangle cr = { (float)rx, (float)ry, (float)cw, (float)ch };
        kbnav_info_register_crossref(cr, hit.module, InfoItemKind::MathlibHitCard);
    }

    if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ... }
}

// ── (c) draw_crossrefs_block – bucle MSC: añadir registro ────────────────────
// ORIGINAL (dentro del bucle for msc_refs):
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ... }
        ci++;

// NUEVO:
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ... }

        // Registrar para teclado
        {
            Rectangle cr = { (float)rx, (float)ry, (float)card_w, (float)card_h };
            kbnav_info_register_crossref(cr, code, InfoItemKind::CrossrefCard);
        }
        ci++;

// ── (d) draw_crossrefs_block – bucle STD: ídem ───────────────────────────────
// Mismo patrón, al final del bucle std_refs:
        {
            Rectangle cr = { (float)rx, (float)ry, (float)card_w, (float)card_h };
            kbnav_info_register_crossref(cr, code, InfoItemKind::CrossrefCard);
        }
        ci++;


/* ═══════════════════════════════════════════════════════════════════════════
   DIFF 6 – app.hpp  (añadir kb_trigger a LatexRenderJob)
   ═══════════════════════════════════════════════════════════════════════════ */

// Busca el struct/miembro que contiene state, tex_code, etc., y añade:
    bool kb_trigger = false;   // ← teclado solicita launch_latex_render
