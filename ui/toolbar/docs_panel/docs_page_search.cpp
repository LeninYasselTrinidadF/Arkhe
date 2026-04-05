// ── docs_page_search.cpp ─────────────────────────────────────────────────────
// Página 4 — Panel de búsqueda (Local + Loogle)
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_search_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Introducción ──────────────────────────────────────────────────────────
    docs_draw_section_title("Panel de Búsqueda", lx, y);
    docs_draw_paragraph(
        "Panel lateral fijo en el lado derecho de la ventana. "
        "Dividido en dos mitades: búsqueda local difusa (mitad superior) "
        "y búsqueda Loogle de Mathlib (mitad inferior).",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;

    // ── Búsqueda local ────────────────────────────────────────────────────────
    docs_draw_section_title("Búsqueda Local (fuzzy)", lx, y);
    docs_draw_paragraph(
        "Busca en el árbol de nodos del modo de vista activo (MSC2020, Mathlib o Estándar). "
        "Usa un algoritmo difuso que acepta coincidencias parciales y tolerancia a errores tipográficos.",
        lx, lw, y);

    docs_draw_kv_row("Campo de texto",
        "Escribe el término a buscar. La búsqueda se lanza al pulsar 'Buscar' o Enter.",
        lx, lw, y);
    docs_draw_kv_row("Botón Buscar",
        "Carga todos los resultados (hasta 2000). Necesario para paginar.",
        lx, lw, y);
    docs_draw_kv_row("Resultados",
        "Tarjetas con código y etiqueta del nodo. Click para seleccionar y navegar.",
        lx, lw, y);
    docs_draw_kv_row("Paginador",
        "< N/M > — navega entre páginas de 25 resultados.",
        lx, lw, y);
    docs_draw_kv_row("Contador",
        "'25+ resultados' indica que hay más; pulsa Buscar para cargarlos todos.",
        lx, lw, y);

    y += 4;
    docs_draw_paragraph("Atajos en la zona Búsqueda:", lx, lw, y, docs_col::text_head, 10);
    docs_draw_shortcut_row({ "Tab" },
        "Ciclar entre campo, botón y tarjetas de resultados", lx, lw, y);
    docs_draw_shortcut_row({ "Enter" },
        "Activar el botón enfocado o navegar al resultado seleccionado", lx, lw, y);
    docs_draw_shortcut_row({ "Rueda ratón" },
        "Scroll por la lista de resultados", lx, lw, y);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    if (y >= y_bottom) return;

    // ── Loogle ────────────────────────────────────────────────────────────────
    docs_draw_section_title("Loogle (Mathlib online)", lx, y);
    docs_draw_paragraph(
        "Interfaz a Loogle, el motor de búsqueda de declaraciones Mathlib. "
        "Permite buscar teoremas y definiciones por nombre, tipo o firma. "
        "Requiere conexión a internet.",
        lx, lw, y);

    docs_draw_kv_row("Campo de texto",
        "Introduce un nombre, tipo o patrón de búsqueda de Lean 4.",
        lx, lw, y);
    docs_draw_kv_row("Botón Buscar / Enter",
        "Lanza la búsqueda asíncrona contra loogle.lean-lang.org.",
        lx, lw, y);
    docs_draw_kv_row("Buscando...",
        "Indicador de carga mientras la petición HTTP está en curso.",
        lx, lw, y);
    docs_draw_kv_row("Tarjeta de resultado",
        "Muestra nombre, módulo y firma de tipo de cada declaración.",
        lx, lw, y);

    y += 4;
    docs_draw_paragraph("Comportamiento al hacer click en un resultado Loogle:", lx, lw, y,
        docs_col::text_head, 10);
    docs_draw_paragraph(
        "· Si el módulo del resultado existe en el árbol Mathlib cargado, "
        "navega automáticamente hasta el nodo correspondiente (o hasta la declaración hija).",
        lx, lw, y, docs_col::text_body, 10);
    docs_draw_paragraph(
        "· Si el módulo no se encuentra localmente, abre la URL de Loogle "
        "en el navegador del sistema.",
        lx, lw, y, docs_col::text_body, 10);

    y += 4;
    docs_draw_code_block(
        "Ejemplo de búsqueda Loogle:\n"
        "  continuous_mul          → busca por nombre\n"
        "  Real → Real             → busca por tipo\n"
        "  ∀ x : ℝ, 0 ≤ x ^ 2    → busca por patrón",
        lx, lw, y);

    if (y >= y_bottom) return;
}
