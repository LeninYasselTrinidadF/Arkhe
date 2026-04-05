// ── docs_page_data_formats.cpp ───────────────────────────────────────────────
// Página 6 — Formatos de datos: JSON de nodo, crossref, deps
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_data_formats_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── JSON de nodo ──────────────────────────────────────────────────────────
    docs_draw_section_title("Formato: <code>.json", lx, y);
    docs_draw_paragraph(
        "Cada nodo tiene su propio archivo JSON en la carpeta de entradas. "
        "El nombre es el code del nodo con caracteres especiales reemplazados por '_'.",
        lx, lw, y);

    docs_draw_code_block(
        "{\n"
        "  \"basic_info\": {\n"
        "    \"note\":        \"Descripcion del nodo\",\n"
        "    \"texture_key\": \"icono_nodo.png\",\n"
        "    \"msc_refs\":    [\"26A15\", \"54E35\"]\n"
        "  },\n"
        "  \"cross_refs\": [\n"
        "    { \"target_code\": \"26A03\", \"relation\": \"requires\" },\n"
        "    { \"target_code\": \"54A05\", \"relation\": \"see_also\" }\n"
        "  ],\n"
        "  \"resources\": [\n"
        "    { \"kind\": \"link\", \"title\": \"nLab\",\n"
        "      \"content\": \"https://ncatlab.org/...\" }\n"
        "  ]\n"
        "}",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── entries_index.json ────────────────────────────────────────────────────
    docs_draw_section_title("Formato: entries_index.json", lx, y);
    docs_draw_paragraph(
        "Índice que mapea cada code de nodo al nombre de su archivo .tex. "
        "Gestionado automáticamente por el Editor de Entradas.",
        lx, lw, y);

    docs_draw_code_block(
        "{\n"
        "  \"26A15\": \"26A15.tex\",\n"
        "  \"54E35\": \"54E35.tex\",\n"
        "  \"Mathlib.Data.Real.Basic\": \"Mathlib_Data_Real_Basic.tex\"\n"
        "}",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── crossref.json ─────────────────────────────────────────────────────────
    docs_draw_section_title("Formato: crossref.json", lx, y);
    docs_draw_paragraph(
        "Mapa de módulos Mathlib a códigos MSC y temas del temario Estándar. "
        "Cargado al inicio y usado para inyectar msc_refs y standard_refs "
        "en los nodos Mathlib del árbol.",
        lx, lw, y);

    docs_draw_code_block(
        "{\n"
        "  \"Mathlib.Data.Real.Basic\": {\n"
        "    \"msc\":      [\"26A03\", \"54E35\"],\n"
        "    \"standard\": [\"Analysis.Real.Construction\"]\n"
        "  },\n"
        "  \"Mathlib.Topology.Basic\": {\n"
        "    \"msc\":      [\"54A05\", \"54A10\"],\n"
        "    \"standard\": [\"Topology.TopologicalSpaces\"]\n"
        "  }\n"
        "}",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── deps.json ─────────────────────────────────────────────────────────────
    docs_draw_section_title("Formato: deps.json", lx, y);
    docs_draw_paragraph(
        "Grafo de dependencias entre nodos. Cada clave es un code; "
        "depends_on lista los codes de los que depende.",
        lx, lw, y);

    docs_draw_code_block(
        "{\n"
        "  \"26A03\": {\n"
        "    \"label\":      \"Foundations; limits\",\n"
        "    \"depends_on\": [\"00A05\"]\n"
        "  },\n"
        "  \"26A15\": {\n"
        "    \"label\":      \"Continuity\",\n"
        "    \"depends_on\": [\"26A03\"]\n"
        "  }\n"
        "}",
        lx, lw, y);

    docs_draw_paragraph(
        "Hay tres grafos de dependencias independientes: "
        "deps.json (MSC), deps_mathlib.json (Mathlib) y deps_standard.json (Estándar). "
        "El activo en cada momento es el del modo de vista seleccionado.",
        lx, lw, y, docs_col::text_dim, 10);

    if (y >= y_bottom) return;
}
