#pragma once
// ── edit_state.hpp ────────────────────────────────────────────────────────────
// Estado mutable del EntryEditor para el nodo seleccionado actualmente.
// ─────────────────────────────────────────────────────────────────────────────

#include "data/math_node.hpp"
#include "data/editor/section_data.hpp"
#include <string>
#include <vector>
#include <cstring>

struct EditState {

    // ── Buffers editables ─────────────────────────────────────────────────────
    char note_buf   [512]  = {};
    char tex_buf    [128]  = {};
    char msc_buf    [512]  = {};
    char body_buf   [8192] = {};

    char new_kind    [32]  = "link";
    char new_title  [128]  = {};
    char new_content[256]  = {};

    // ── Snapshots (para detección de cambios sin guardar) ─────────────────────
    char note_snap[512] = {};
    char tex_snap [128] = {};
    char msc_snap [512] = {};

    // ── Cross-refs ────────────────────────────────────────────────────────────
    std::vector<EditorCrossRef> cross_refs;
    std::size_t           xrefs_snap_size = 0;   // tamaño al último guardado
    char new_xref_code[64]  = {};
    char new_xref_rel [32]  = "see_also";

    // ── Metadatos del nodo ────────────────────────────────────────────────────
    std::string last_code;
    std::string tex_file;

    // ── Dirty flags ───────────────────────────────────────────────────────────
    bool body_dirty        = false;
    bool basic_info_dirty  = false;   // recomputado cada frame a partir de snapshot
    bool xrefs_dirty       = false;   // recomputado cada frame
    bool resources_dirty   = false;   // recomputado cada frame

    // Tamaño de resources al último guardado (para detectar cambios).
    std::size_t resources_snap_size = 0;

    // ── Vista previa ──────────────────────────────────────────────────────────
    bool preview_mode = false;

    // ── Collapse state de secciones ───────────────────────────────────────────
    // Stored here (not in EntryEditor) so it persists across calls within a frame.
    bool sec_basic_open  = true;
    bool sec_body_open   = true;
    bool sec_xrefs_open  = true;
    bool sec_res_open    = true;

    // ── sync ──────────────────────────────────────────────────────────────────
    /// Carga los campos desde `sel`, `body`, `fname` y `xrefs`.
    /// No-op si el nodo no cambió (mismo code).
    void sync(MathNode* sel,
              const std::string& body,
              const std::string& fname,
              const std::vector<EditorCrossRef>& loaded_xrefs,
              std::size_t loaded_res_size)
    {
        if (!sel || sel->code == last_code) return;

        last_code             = sel->code;
        tex_file              = fname;
        body_dirty            = false;
        basic_info_dirty      = false;
        xrefs_dirty           = false;
        resources_dirty       = false;
        preview_mode          = false;

        strncpy(note_buf,  sel->note.c_str(),        511);  note_buf[511]  = '\0';
        strncpy(tex_buf,   sel->texture_key.c_str(), 127);  tex_buf[127]   = '\0';
        strncpy(body_buf,  body.c_str(),             8191); body_buf[8191] = '\0';

        // MSC refs → cadena separada por comas
        std::string joined;
        for (int i = 0; i < (int)sel->msc_refs.size(); i++) {
            if (i > 0) joined += ',';
            joined += sel->msc_refs[i];
        }
        strncpy(msc_buf, joined.c_str(), 511); msc_buf[511] = '\0';

        // Snapshots para detección de cambios
        strncpy(note_snap, note_buf, 511); note_snap[511] = '\0';
        strncpy(tex_snap,  tex_buf,  127); tex_snap[127]  = '\0';
        strncpy(msc_snap,  msc_buf,  511); msc_snap[511]  = '\0';

        cross_refs          = loaded_xrefs;
        xrefs_snap_size     = cross_refs.size();
        resources_snap_size = loaded_res_size;

        new_title[0]     = '\0';
        new_content[0]   = '\0';
        new_xref_code[0] = '\0';
        strncpy(new_xref_rel, "see_also", 31);
    }

    // ── update_dirty_flags ────────────────────────────────────────────────────
    /// Recomputa los flags dirty. Llamar una vez por frame antes de dibujar secciones.
    void update_dirty_flags(const MathNode* sel) {
        basic_info_dirty = (strcmp(note_buf, note_snap) != 0 ||
                            strcmp(tex_buf,  tex_snap)  != 0 ||
                            strcmp(msc_buf,  msc_snap)  != 0);
        xrefs_dirty       = (cross_refs.size() != xrefs_snap_size);
        resources_dirty   = sel && (sel->resources.size() != resources_snap_size);
    }

    // ── commit_snapshots ──────────────────────────────────────────────────────
    /// Llama después de un guardado JSON exitoso para resetear dirty flags.
    void commit_snapshots(const MathNode* sel) {
        strncpy(note_snap, note_buf, 511);
        strncpy(tex_snap,  tex_buf,  127);
        strncpy(msc_snap,  msc_buf,  511);
        xrefs_snap_size     = cross_refs.size();
        resources_snap_size = sel ? sel->resources.size() : 0;
        basic_info_dirty    = false;
        xrefs_dirty         = false;
        resources_dirty     = false;
    }

    // ── invalidate ────────────────────────────────────────────────────────────
    void invalidate() { last_code.clear(); }
};
