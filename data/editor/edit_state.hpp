#pragma once
// ── edit_state.hpp ────────────────────────────────────────────────────────────
// Estado mutable que el EntryEditor mantiene sobre el nodo actualmente
// seleccionado. Se extrae aquí para que sea fácil leerlo, ampliarlo o
// testearlo de forma independiente.
// ─────────────────────────────────────────────────────────────────────────────

#include "data/math_node.hpp"
#include <string>
#include <cstring>

struct EditState {
    // ── Buffers editables ─────────────────────────────────────────────────────
    char note_buf  [512]   = {};
    char tex_buf   [128]   = {};
    char msc_buf   [512]   = {};
    char body_buf  [8192]  = {};   ///< cuerpo LaTeX del .tex vinculado
    char new_kind  [32]    = "link";
    char new_title [128]   = {};
    char new_content[256]  = {};

    // ── Metadatos del nodo en edición ─────────────────────────────────────────
    std::string last_code;          ///< code del nodo que se sincronizó por última vez
    std::string tex_file;           ///< filename (sin ruta) del .tex vinculado
    bool        body_dirty    = false; ///< hay cambios sin guardar en body_buf

    // ── Vista previa ──────────────────────────────────────────────────────────
    /// Cuando true, draw_body_section muestra el preview renderizado en lugar
    /// de la textarea de edición.
    bool        preview_mode  = false;

    // ── sync ──────────────────────────────────────────────────────────────────
    /// Carga los campos desde `sel` y `body`. Es no-op si el nodo no cambió.
    void sync(MathNode* sel,
              const std::string& body,
              const std::string& fname)
    {
        if (!sel || sel->code == last_code) return;

        last_code     = sel->code;
        tex_file      = fname;
        body_dirty    = false;
        preview_mode  = false;          // al cambiar de nodo, volver a edición

        strncpy(note_buf, sel->note.c_str(), 511);          note_buf[511]  = '\0';
        strncpy(tex_buf,  sel->texture_key.c_str(), 127);   tex_buf[127]   = '\0';
        strncpy(body_buf, body.c_str(), 8191);               body_buf[8191] = '\0';

        // MSC refs → string separado por comas
        std::string joined;
        for (int i = 0; i < (int)sel->msc_refs.size(); i++) {
            if (i > 0) joined += ',';
            joined += sel->msc_refs[i];
        }
        strncpy(msc_buf, joined.c_str(), 511); msc_buf[511] = '\0';

        new_title[0]   = '\0';
        new_content[0] = '\0';
    }

    // ── reset ─────────────────────────────────────────────────────────────────
    /// Fuerza una resincronización en el próximo frame (p.ej. al recargar assets).
    void invalidate() { last_code.clear(); }
};
