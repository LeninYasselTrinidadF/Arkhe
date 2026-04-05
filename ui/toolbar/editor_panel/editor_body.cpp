// ── editor_body.cpp ───────────────────────────────────────────────────────────
// Unity entry point del subsistema de body-editing.
// NO añadir lógica aquí.
//
// Sub-módulos (NO compilar directamente):
//   editor_body_preview.cpp   → draw_body_preview
//   editor_body_keyboard.cpp  → handle_body_keyboard  (static)
//   editor_body_render.cpp    → render_body_edit       (static)
//   editor_body_section.cpp   → draw_body_section
// ─────────────────────────────────────────────────────────────────────────────

#include "editor_sections_internal.hpp"
#include "data/editor/body_parser.hpp"
#include "data/editor/equation_cache.hpp"

// ── Estado persistente del textarea ──────────────────────────────────────────
namespace editor_sections {
    static BodyCursor  g_body;
    static std::string g_tf_clipboard;
}

// ── Sub-módulos ───────────────────────────────────────────────────────────────
#include "editor_body_preview.cpp"
#include "editor_body_keyboard.cpp"
#include "editor_body_render.cpp"
#include "editor_body_section.cpp"
