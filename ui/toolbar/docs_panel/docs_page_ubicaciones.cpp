// ── docs_page_ubicaciones.cpp ────────────────────────────────────────────────
// Página 5 — Panel de Ubicaciones y generadores Mathlib
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_ubicaciones_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Introducción ──────────────────────────────────────────────────────────
    docs_draw_section_title("Panel de Ubicaciones", lx, y);
    docs_draw_paragraph(
        "Configura las rutas del sistema. Se abre desde el icono de carpeta en el toolbar. "
        "Los cambios se aplican al pulsar 'Aplicar' o al confirmar con Enter en cualquier campo. "
        "Al aplicar, todos los assets se recargan.",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;

    // ── Rutas configurables ───────────────────────────────────────────────────
    docs_draw_section_title("Rutas configurables", lx, y);

    const char* paths[][2] = {
        { "Raíz de Assets",
          "Carpeta base. Contiene msc2020_tree.json, crossref.json, deps.json, etc." },
        { "Entradas (.tex)",
          "Carpeta con los .tex y .json de cada nodo. Suele ser assets/entries/." },
        { "Gráficos",
          "Carpeta con texturas, nine-patches del skin y el icono. Suele ser assets/graphics/." },
        { "pdflatex.exe",
          "Ruta al binario pdflatex de TeX Live (para compilar la vista previa LaTeX)." },
        { "pdftoppm.exe",
          "Ruta al binario pdftoppm de Poppler (para convertir PDF → PNG en la preview)." },
        { "Mathlib src",
          "Raíz del repositorio clonado de Mathlib4. Usada por los generadores." },
    };
    for (auto& r : paths)
        docs_draw_kv_row(r[0], r[1], lx, lw, y, docs_col::accent);

    y += 4;
    docs_draw_paragraph(
        "El botón '...' al final de cada campo abre el diálogo del sistema para "
        "seleccionar carpeta o archivo.",
        lx, lw, y, docs_col::text_dim, 10);

    docs_draw_sep(lx, lw, y); y += 8;

    if (y >= y_bottom) return;

    // ── Generadores Mathlib ───────────────────────────────────────────────────
    docs_draw_section_title("Generadores Mathlib", lx, y);
    docs_draw_paragraph(
        "Generan los JSON necesarios a partir del código fuente de Mathlib4. "
        "Requieren que 'Mathlib src' esté configurado. Se ejecutan en un hilo secundario.",
        lx, lw, y);

    docs_draw_card("Actualizar Layout",
        "Parchea el mathlib_layout.json existente con los módulos nuevos o modificados. "
        "Más rápido que un Full; conserva las anotaciones existentes.",
        lx, lw, y, docs_col::accent);

    docs_draw_card("Full Layout",
        "Regenera mathlib_layout.json desde cero leyendo toda la estructura de carpetas "
        "de Mathlib. Tarda más pero garantiza consistencia total.",
        lx, lw, y, docs_col::accent3);

    docs_draw_card("Actualizar Deps",
        "Parchea deps_mathlib.json con las dependencias nuevas.",
        lx, lw, y, docs_col::accent2);

    docs_draw_card("Full Deps",
        "Regenera deps_mathlib.json desde cero analizando los imports de cada módulo.",
        lx, lw, y, docs_col::accent3);

    docs_draw_paragraph(
        "Tras finalizar la generación aparece el botón 'Recargar Assets'. "
        "Pulsarlo aplica los nuevos JSONs al grafo en memoria.",
        lx, lw, y, docs_col::text_dim, 10);

    docs_draw_sep(lx, lw, y); y += 8;

    if (y >= y_bottom) return;

    // ── Acople de secciones JSON ───────────────────────────────────────────────
    docs_draw_section_title("Acople de secciones JSON", lx, y);
    docs_draw_paragraph(
        "Los tres botones de acople leen los <code>.json de la carpeta de entradas "
        "y propagan su contenido a los nodos del grafo en memoria. "
        "Útil después de editar los JSON manualmente o de recargar assets.",
        lx, lw, y);

    docs_draw_kv_row("Acoplar Basic Info",
        "Aplica note, texture_key y msc_refs de cada JSON a su nodo.",
        lx, lw, y, docs_col::accent2);
    docs_draw_kv_row("Acoplar Referencias",
        "Aplica el array cross_refs de cada JSON a su nodo.",
        lx, lw, y, docs_col::accent2);
    docs_draw_kv_row("Acoplar Recursos",
        "Aplica el array resources de cada JSON a su nodo.",
        lx, lw, y, docs_col::accent2);

    docs_draw_paragraph(
        "Nota: el acople también activa assets_changed = true, "
        "lo que provoca una recarga completa del grafo en el siguiente frame.",
        lx, lw, y, docs_col::text_dim, 10);

    if (y >= y_bottom) return;
}
