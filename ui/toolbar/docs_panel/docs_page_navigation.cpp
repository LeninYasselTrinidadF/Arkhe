// ── docs_page_navigation.cpp ─────────────────────────────────────────────────
// Página 2 — Navegación por el canvas y atajos de teclado
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_navigation_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Canvas: ratón ─────────────────────────────────────────────────────────
    docs_draw_section_title("Navegación con ratón", lx, y);

    struct MouseRow { const char* action; const char* desc; };
    MouseRow mouse_rows[] = {
        { "Click burbuja",           "Navegar hacia adentro (si tiene hijos)" },
        { "Doble click burbuja",     "Seleccionar el nodo sin entrar" },
        { "Drag fondo",              "Mover la cámara (pan)" },
        { "Rueda ratón",             "Zoom in / out sobre el cursor" },
        { "Rueda cerca del divisor", "Mover el divisor horizontal" },
        { "Drag divisor",            "Redimensionar Info ↕ Canvas" },
    };
    for (auto& r : mouse_rows)
        docs_draw_kv_row(r.action, r.desc, lx, lw, y, docs_col::accent);

    y += 6;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Canvas: teclado ───────────────────────────────────────────────────────
    docs_draw_section_title("Atajos — Canvas", lx, y);

    docs_draw_shortcut_row({ "ESC" },
        "Subir un nivel (pop del nav stack)", lx, lw, y);
    docs_draw_shortcut_row({ "Tab" },
        "Activar navegación por teclado / ciclar zona de foco", lx, lw, y);
    docs_draw_shortcut_row({ "←", "→" },
        "Ciclar modo de vista (MSC2020 / Mathlib / Estándar)", lx, lw, y);
    docs_draw_shortcut_row({ "↑", "↓" },
        "Mover selección entre burbujas del anillo", lx, lw, y);
    docs_draw_shortcut_row({ "Enter" },
        "Entrar en la burbuja seleccionada por teclado", lx, lw, y);
    docs_draw_shortcut_row({ "D" },
        "Activar / desactivar vista de grafo de dependencias", lx, lw, y);

    y += 6;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Zonas de foco ─────────────────────────────────────────────────────────
    docs_draw_section_title("Zonas de foco (Tab)", lx, y);
    docs_draw_paragraph(
        "La navegación por teclado organiza la pantalla en cuatro zonas. "
        "Pulsa Tab repetidamente para ciclar entre ellas. El indicador '⌨ Zona' "
        "aparece en el toolbar cuando el modo teclado está activo.",
        lx, lw, y);

    const char* zones[][2] = {
        { "Canvas",   "Vista de burbujas / dependencias" },
        { "Toolbar",  "Tabs y botones del toolbar superior" },
        { "Búsqueda", "Panel lateral derecho (local + Loogle)" },
        { "Info",     "Panel de información inferior" },
    };
    for (auto& z : zones)
        docs_draw_kv_row(z[0], z[1], lx, lw, y, docs_col::accent2);

    y += 6;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Vista de dependencias ─────────────────────────────────────────────────
    docs_draw_section_title("Vista de dependencias", lx, y);
    docs_draw_paragraph(
        "Muestra un grafo dirigido de dependencias entre nodos. "
        "Los arcos indican 'requiere'. Los nodos se colorean según profundidad. "
        "Fuentes: deps.json (MSC), deps_mathlib.json (Mathlib), deps_standard.json.",
        lx, lw, y);

    docs_draw_shortcut_row({ "D" },         "Activar/desactivar", lx, lw, y);
    docs_draw_shortcut_row({ "ESC" },       "Salir de la vista de deps", lx, lw, y);
    docs_draw_shortcut_row({ "Drag fondo"}, "Pan en el grafo de deps", lx, lw, y);

    if (y >= y_bottom) return;
}
