// ── docs_page_entry_editor.cpp ───────────────────────────────────────────────
// Página 3 — Editor de Entradas (EntryEditor)
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_entry_editor_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Introducción ──────────────────────────────────────────────────────────
    docs_draw_section_title("Editor de Entradas", lx, y);
    docs_draw_paragraph(
        "Panel flotante para documentar cualquier nodo del grafo. "
        "Se abre seleccionando una burbuja y pulsando el icono lápiz en el toolbar. "
        "Contiene cuatro secciones colapsables, cada una con su propio botón 'Guardar'.",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;

    // ── Persistencia ──────────────────────────────────────────────────────────
    docs_draw_section_title("Persistencia de datos", lx, y);
    docs_draw_paragraph(
        "Cada nodo se guarda en dos archivos dentro de entries_path:",
        lx, lw, y);

    docs_draw_kv_row("<code>.tex",  "Cuerpo LaTeX del nodo",                  lx, lw, y, docs_col::text_code);
    docs_draw_kv_row("<code>.json", "Basic info + cross-refs + recursos",      lx, lw, y, docs_col::text_code);
    docs_draw_kv_row("entries_index.json", "Mapa code → nombre de archivo",   lx, lw, y, docs_col::text_code);

    docs_draw_paragraph(
        "El nombre de archivo se genera de forma segura: los caracteres '/', '\\', "
        "':', ' ' y '.' del code se reemplazan por '_'. "
        "Ejemplo: 'Mathlib.Data.Real.Basic' → 'Mathlib_Data_Real_Basic.json'.",
        lx, lw, y, docs_col::text_dim, 10);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Sección BASIC INFO ────────────────────────────────────────────────────
    docs_draw_section_title("Sección: BASIC INFO", lx, y);

    docs_draw_kv_row("Nota / descripción",
        "Texto libre de anotación sobre el nodo.",
        lx, lw, y);
    docs_draw_kv_row("Clave de imagen (texture_key)",
        "Nombre del archivo en graphics/ que se muestra en la burbuja.",
        lx, lw, y);
    docs_draw_kv_row("MSC refs",
        "Lista de códigos MSC2020 separados por comas. "
        "Ej: '26A15, 54E35'.",
        lx, lw, y);

    docs_draw_paragraph(
        "Los cambios se escriben en vivo sobre el nodo en memoria. "
        "El botón 'Guardar' serializa todo al <code>.json del nodo. "
        "Un asterisco (*) en la barra de sección indica cambios sin guardar.",
        lx, lw, y, docs_col::text_dim, 10);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    // ── Sección CUERPO LATEX ──────────────────────────────────────────────────
    docs_draw_section_title("Sección: CUERPO LATEX", lx, y);
    docs_draw_paragraph(
        "Textarea multilínea para escribir contenido LaTeX del nodo. "
        "Soporta edición con cursor, selección, Ctrl+C/V/A y scroll.",
        lx, lw, y);

    // Subsección: botones del cuerpo
    docs_draw_kv_row("Preview / Editar",
        "Alterna entre el textarea de edición y la vista previa renderizada.",
        lx, lw, y, docs_col::accent2);
    docs_draw_kv_row("Importar",
        "Abre el File Manager para asociar un .tex existente al nodo.",
        lx, lw, y, docs_col::accent2);
    docs_draw_kv_row("Guardar",
        "Escribe el buffer al .tex en disco y actualiza el entries_index.",
        lx, lw, y, docs_col::accent2);

    y += 4;
    docs_draw_paragraph("Atajos del textarea:", lx, lw, y, docs_col::text_head, 10);
    docs_draw_shortcut_row({ "Tab" },           "Salir del textarea al siguiente campo", lx, lw, y);
    docs_draw_shortcut_row({ "Ctrl", "A" },     "Seleccionar todo el texto", lx, lw, y);
    docs_draw_shortcut_row({ "Ctrl", "C" },     "Copiar selección al clipboard", lx, lw, y);
    docs_draw_shortcut_row({ "Ctrl", "V" },     "Pegar desde clipboard", lx, lw, y);
    docs_draw_shortcut_row({ "Ctrl", "Z" },     "Deshacer (historial de edición)", lx, lw, y);
    docs_draw_shortcut_row({ "↑", "↓" },        "Mover cursor línea arriba/abajo", lx, lw, y);
    docs_draw_shortcut_row({ "Rueda ratón" },   "Scroll en el textarea", lx, lw, y);

    y += 4;
    docs_draw_paragraph(
        "El File Manager muestra los .tex disponibles en entries_path. "
        "Al seleccionar uno, su contenido se carga en el buffer y se asocia "
        "al nodo en el entries_index.",
        lx, lw, y, docs_col::text_dim, 10);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    if (y >= y_bottom) return;

    // ── Sección REFERENCIAS CRUZADAS ──────────────────────────────────────────
    docs_draw_section_title("Sección: REFERENCIAS CRUZADAS", lx, y);
    docs_draw_paragraph(
        "Gestiona enlaces tipados entre el nodo actual y otros nodos del grafo. "
        "Cada referencia tiene un código de destino y un tipo de relación.",
        lx, lw, y);

    const char* rel_types[][2] = {
        { "Ver también",  "Referencia general (see_also) — por defecto" },
        { "Requiere",     "El nodo actual depende del destino (requires)" },
        { "Extiende",     "El nodo amplía o generaliza el destino (extends)" },
        { "Ej. de",       "El nodo es un ejemplo del destino (example_of)" },
        { "Generaliza",   "El nodo es más general que el destino (generalizes)" },
        { "Equivale a",   "Equivalencia matemática o formal (equivalent_to)" },
    };
    for (auto& r : rel_types)
        docs_draw_kv_row(r[0], r[1], lx, lw, y, docs_col::accent3);

    docs_draw_paragraph(
        "El campo de código destino tiene autocompletado: muestra hasta 5 sugerencias "
        "de códigos de nodos hermanos (hijos del nodo actual) mientras se escribe.",
        lx, lw, y, docs_col::text_dim, 10);
    docs_draw_paragraph(
        "Pulsa 'Agregar' para añadir la referencia a la lista. "
        "Usa el botón 'x' de cada fila para eliminar una referencia existente. "
        "'Guardar' serializa la lista completa al <code>.json.",
        lx, lw, y, docs_col::text_dim, 10);

    y += 4;
    docs_draw_sep(lx, lw, y); y += 8;

    if (y >= y_bottom) return;

    // ── Sección RECURSOS ──────────────────────────────────────────────────────
    docs_draw_section_title("Sección: RECURSOS", lx, y);
    docs_draw_paragraph(
        "Lista de recursos adjuntos al nodo (enlaces, libros, papers, videos, etc.).",
        lx, lw, y);

    docs_draw_kv_row("kind",    "Tipo de recurso: 'link', 'book', 'paper', 'video', etc.", lx, lw, y);
    docs_draw_kv_row("title",   "Título visible del recurso (campo obligatorio).",          lx, lw, y);
    docs_draw_kv_row("content", "URL, ISBN, DOI o cualquier referencia al recurso.",        lx, lw, y);

    docs_draw_paragraph(
        "Los recursos se almacenan en el array 'resources' del <code>.json. "
        "El botón 'x' de cada fila los elimina en memoria; 'Guardar' persiste el cambio.",
        lx, lw, y, docs_col::text_dim, 10);

    if (y >= y_bottom) return;
}
