// ── docs_page_architecture.cpp ───────────────────────────────────────────────
// Página 7 — Arquitectura de código
// Incluido por docs_panel.cpp (no compilar por separado).
// ─────────────────────────────────────────────────────────────────────────────

static void docs_page_architecture_draw(
    int lx, int lw, int& y, int y_bottom,
    Vector2 /*mouse*/, float& /*scroll*/, float /*dt*/)
{
    // ── Módulos principales ───────────────────────────────────────────────────
    docs_draw_section_title("Módulos del proyecto", lx, y);
    docs_draw_paragraph(
        "El código está organizado en capas. La capa de datos no incluye "
        "headers de Raylib; la capa UI sí. Los paneles son PanelWidgets.",
        lx, lw, y);

    docs_draw_card("data/",
        "MathNode, EditState, SearchHit, CrossRef, Resource. "
        "Loaders (msc, mathlib, crossref, deps). "
        "Generadores (mathlib_gen). Búsqueda (loogle, search_utils).",
        lx, lw, y, docs_col::accent);

    docs_draw_card("ui/widgets/",
        "PanelWidget — base de todos los paneles flotantes. "
        "Gestión de drag, resize, campos de texto con cursor/selección.",
        lx, lw, y, docs_col::accent2);

    docs_draw_card("ui/toolbar/",
        "Toolbar, EntryEditor, UbicacionesPanel, DocsPanel, ConfigPanel. "
        "Cada panel hereda de PanelWidget.",
        lx, lw, y, docs_col::accent3);

    docs_draw_card("ui/toolbar/editor_panel/",
        "Secciones del EntryEditor: basic_info, body (unity build), "
        "cross_refs, resources. Patrón: una función draw_*_section por archivo.",
        lx, lw, y, docs_col::accent);

    docs_draw_card("ui/search_panel/",
        "SearchPanel (orquestador), LocalSearchList, LoogleList. "
        "search_widgets.cpp contiene los primitivos visuales sin estado.",
        lx, lw, y, docs_col::accent2);

    docs_draw_card("ui/key_controls/",
        "keyboard_nav (zonas globales), kbnav_toolbar (Tab en paneles modales), "
        "kbnav_search (Tab en búsqueda), kbnav_views (hotkeys del canvas).",
        lx, lw, y, docs_col::accent3);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── Patrón Unity Build ────────────────────────────────────────────────────
    docs_draw_section_title("Patrón Unity Build (editor_body.cpp)", lx, y);
    docs_draw_paragraph(
        "editor_body.cpp es el único archivo que se compila. "
        "Incluye los sub-módulos con #include, compartiendo estado estático "
        "del namespace editor_sections sin exponerlo al resto del proyecto.",
        lx, lw, y);

    docs_draw_code_block(
        "// editor_body.cpp — punto de entrada\n"
        "namespace editor_sections {\n"
        "    static BodyCursor  g_body;\n"
        "    static std::string g_tf_clipboard;\n"
        "}\n"
        "#include \"editor_body_preview.cpp\"\n"
        "#include \"editor_body_keyboard.cpp\"\n"
        "#include \"editor_body_render.cpp\"\n"
        "#include \"editor_body_section.cpp\"",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── Patrón SaveFn callback ────────────────────────────────────────────────
    docs_draw_section_title("Patrón SaveFn", lx, y);
    docs_draw_paragraph(
        "Las funciones draw_*_section reciben un callback SaveFn que se llama "
        "cuando el usuario pulsa 'Guardar'. El callback serializa el estado "
        "al JSON del nodo usando editor_io::save_node_json.",
        lx, lw, y);

    docs_draw_code_block(
        "using SaveFn = void(*)(MathNode*, EditState&, AppState&);\n"
        "\n"
        "// En EntryEditor::draw():\n"
        "editor_sections::SaveFn json_save =\n"
        "    [](MathNode* s, EditState&, AppState&) {\n"
        "        if (s_self) s_self->save_json(s);\n"
        "    };",
        lx, lw, y);

    docs_draw_sep(lx, lw, y); y += 8;
    if (y >= y_bottom) return;

    // ── kbnav: patrón de doble buffer ────────────────────────────────────────
    docs_draw_section_title("kbnav: patrón de registro + doble buffer", lx, y);
    docs_draw_paragraph(
        "Los sistemas kbnav_toolbar y kbnav_search siguen el mismo patrón: "
        "los widgets se registran durante el draw() del mismo frame, "
        "y la activación (Enter) se detecta en el frame SIGUIENTE "
        "mediante un doble buffer (s_activated / s_prev_activated).",
        lx, lw, y);

    docs_draw_code_block(
        "// Orden en el draw del panel:\n"
        "kbnav_toolbar_begin_frame();   // 1. limpiar registro\n"
        "int idx = kbnav_toolbar_register(kind, rect, label);\n"
        "if (kbnav_toolbar_is_focused(idx))   { /* highlight */ }\n"
        "if (kbnav_toolbar_is_activated(idx)) { /* accion     */ }\n"
        "// ... dibujar todos los widgets ...\n"
        "kbnav_toolbar_handle(state);   // 4. procesar Tab/Enter\n"
        "kbnav_toolbar_draw();          // 5. borde de foco",
        lx, lw, y);

    if (y >= y_bottom) return;
}
