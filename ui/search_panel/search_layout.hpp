#pragma once

// ── SearchLayout ──────────────────────────────────────────────────────────────
// Métricas de layout del panel de búsqueda, calculadas una vez por frame en
// SearchPanel::draw() y pasadas a los sub-componentes.
// Todos los valores ya tienen g_fonts.scale() aplicado.
// ─────────────────────────────────────────────────────────────────────────────

struct SearchLayout {
    int px;       // x del contenido (CANVAS_W + 10)
    int pw;       // ancho del contenido (PANEL_W - SB_W - 16)
    int SB_X;     // x de la scrollbar (CANVAS_W + PANEL_W - SB_W - 3)
    int field_h;  // altura del campo de texto
    int lbl_gap;  // espacio bajo etiquetas
    int item_gap; // espacio entre tarjetas
    int pager_h;  // altura de la fila de paginación
    int LBL_SZ;   // tamaño de fuente etiquetas
    int ITEM_SZ;  // tamaño de fuente ítems de lista
};
