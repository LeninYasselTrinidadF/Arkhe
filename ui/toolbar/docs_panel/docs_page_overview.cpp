// ── docs_page_overview.cpp ───────────────────────────────────────────────────
// Página 1 — Visión general de Arkhe
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_overview_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Introducción ──────────────────────────────────────────────────────────
    docs_draw_section_title("¿Qué es Arkhe?", lx, y);
    docs_draw_paragraph(
        "Arkhe es un explorador visual del espacio matemático. Muestra la clasificación "
        "MSC2020, la librería formal Mathlib (Lean 4) y un temario académico personalizado "
        "como grafos de burbujas navegables. Permite editar entradas con cuerpo LaTeX, "
        "referencias cruzadas y recursos adjuntos.",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;

    // ── Tres modos de vista ───────────────────────────────────────────────────
    docs_draw_section_title("Modos de vista", lx, y);

    docs_draw_card("MSC2020",
        "Classification Mathematics Subject (AMS). Jerarquía de áreas de investigación "
        "matemática. Dos niveles: sección (00–97) → subsección (00Axx) → tema (00A05).",
        lx, lw, y, docs_col::accent);

    docs_draw_card("Mathlib",
        "Librería formal Lean 4. Árbol de módulos, archivos y declaraciones. "
        "Los nodos hoja son teoremas, definiciones y lemas individuales.",
        lx, lw, y, docs_col::accent2);

    docs_draw_card("Estándar",
        "Temario académico personalizado. Conecta conceptos de MSC2020 con módulos "
        "de Mathlib para mostrar la formalización de un currículo de matemáticas.",
        lx, lw, y, docs_col::accent3);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Estructura de archivos de assets ─────────────────────────────────────
    docs_draw_section_title("Assets esperados", lx, y);

    const char* asset_rows[][2] = {
        { "assets/msc2020_tree.json",   "Árbol MSC2020 completo" },
        { "assets/mathlib_layout.json", "Árbol Mathlib generado" },
        { "assets/crossref.json",       "Módulo Mathlib → MSC + Estándar" },
        { "assets/deps.json",           "Grafo de dependencias MSC" },
        { "assets/deps_mathlib.json",   "Grafo de dependencias Mathlib" },
        { "assets/entries/<code>.tex",  "Cuerpo LaTeX de cada nodo" },
        { "assets/entries/<code>.json", "Metadatos JSON de cada nodo" },
        { "assets/graphics/",           "Texturas, skin nine-patch, icono" },
    };
    for (auto& r : asset_rows)
        docs_draw_kv_row(r[0], r[1], lx, lw, y, docs_col::text_code);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Flujo típico de uso ───────────────────────────────────────────────────
    docs_draw_section_title("Flujo típico", lx, y);

    const char* steps[] = {
        "1. Configura las rutas en Ubicaciones (icono carpeta en toolbar).",
        "2. Pulsa 'Aplicar' para cargar los assets.",
        "3. Navega por el grafo haciendo click en las burbujas.",
        "4. Selecciona una burbuja y abre el Editor de Entradas (icono lápiz).",
        "5. Escribe el cuerpo LaTeX, añade referencias y recursos.",
        "6. Guarda con el botón 'Guardar' de cada sección.",
        "7. Usa Buscar (panel derecho) para localizar nodos por nombre o fórmula.",
    };
    for (auto* s : steps)
        docs_draw_paragraph(s, lx + 8, lw - 8, y, docs_col::text_body, 10);

    if (y >= y_bottom) return;  // guard
}
