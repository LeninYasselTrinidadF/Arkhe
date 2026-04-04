#include "ui/toolbar/ubicaciones_panel.hpp"
#include "data/gen/mathlib_gen.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include "ui/constants.hpp"
#include "tinyfiledialogs.h"
#include <cstring>
#include <thread>
#include <string>

static void browse_folder(char* buf, int buf_len, const char* title) {
    const char* result = tinyfd_selectFolderDialog(title, buf);
    if (result) {
        strncpy(buf, result, buf_len - 1);
        buf[buf_len - 1] = '\0';
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] != '/' && buf[len - 1] != '\\')
            if (len < buf_len - 2) { buf[len] = '/'; buf[len + 1] = '\0'; }
    }
}

static void browse_file(char* buf, int buf_len, const char* title,
    int n_filters = 1, const char** filters = nullptr,
    const char* filter_desc = "Archivo")
{
    const char* default_exe[] = { "*.exe" };
    if (!filters) filters = default_exe;
    const char* result = tinyfd_openFileDialog(title, buf, n_filters, filters, filter_desc, 0);
    if (result) {
        strncpy(buf, result, buf_len - 1);
        buf[buf_len - 1] = '\0';
    }
}

static bool action_btn(const char* label, int x, int y, int w, int h,
    bool enabled, Vector2 mouse,
    Color bg_normal, Color bg_hover, Color bg_disabled)
{
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    bool hov = enabled && CheckCollisionPointRec(mouse, r);
    Color bg = !enabled ? bg_disabled : hov ? bg_hover : bg_normal;
    Color tc = !enabled ? Color{ 90, 100, 130, 160 } : WHITE;
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.f, enabled ? Color{ 70, 130, 255, 200 } : Color{ 40, 50, 80, 120 });
    int tw = MeasureTextF(label, 11);
    DrawTextF(label, x + (w - tw) / 2, y + (h - 11) / 2, 11, tc);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void launch_job(MathlibGenJob& job,
    const MathlibGenOptions& opts,
    void (*fn)(const MathlibGenOptions&, MathlibGenJob&))
{
    job.running = true;
    job.done = false;
    job.status = "Iniciando...";
    std::thread([opts, &job, fn]() mutable {
        fn(opts, job);
        }).detach();
}

void UbicacionesPanel::draw(Vector2 mouse) {
    if (!state.toolbar.ubicaciones_open) return;

    if (draw_window_frame(PW, PH,
        "UBICACIONES", { 120, 160, 255, 255 },
        { 50, 80, 160, 220 }, mouse))
    {
        state.toolbar.ubicaciones_open = false;
        return;
    }

    auto& tb = state.toolbar;
    const int lx = pos_x + 16;
    const int field_w = PW - 200;
    const int label_w = 130;
    const int btn_w = 36;
    const int fh = 22;
    const int spacing = 46;

    struct Row {
        const char* label;
        const char* title_dialog;
        char* buf;
        int         buf_len;
        int         field_id;
        bool        is_file;
    };

    Row rows[] = {
        { "Raiz de Assets",  "Seleccionar carpeta de assets",   tb.assets_path,      512, 0, false },
        { "Entradas (.tex)", "Seleccionar carpeta de entradas", tb.entries_path,     512, 1, false },
        { "Graficos",        "Seleccionar carpeta de graficos", tb.graphics_path,    512, 2, false },
        { "pdflatex.exe",    "Seleccionar pdflatex",           tb.latex_path,       512, 3, true  },
        { "pdftoppm.exe",    "Seleccionar pdftoppm",           tb.pdftoppm_path,    512, 4, true  },
        { "Mathlib src",     "Seleccionar raiz de mathlib4",   tb.mathlib_src_path, 512, 5, false },
    };

    int  y = pos_y + 40;
    bool any_confirmed = false;

    for (auto& row : rows) {
        DrawTextF(row.label, lx, y + (fh - 11) / 2, 11, { 140, 150, 185, 230 });
        int fx = lx + label_w;
        any_confirmed |= draw_text_field(row.buf, row.buf_len,
            fx, y, field_w, fh, 11,
            tb.active_field, row.field_id, mouse);

        Rectangle br = { (float)(fx + field_w + 6), (float)y, (float)btn_w, (float)fh };
        bool bhov = CheckCollisionPointRec(mouse, br);
        DrawRectangleRec(br, bhov ? Color{ 60, 80, 150, 255 } : Color{ 30, 35, 60, 255 });
        DrawRectangleLinesEx(br, 1.f, bhov ? Color{ 100, 140, 255, 220 } : Color{ 50, 60, 110, 200 });
        DrawTextF("...", (int)br.x + 8, (int)br.y + (fh - 10) / 2, 10, { 160, 170, 210, 220 });
        if (bhov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (row.is_file) browse_file(row.buf, row.buf_len, row.title_dialog);
            else             browse_folder(row.buf, row.buf_len, row.title_dialog);
        }
        y += spacing;
    }

    draw_h_line(lx, y + 4, pos_x + PW - lx);
    y += 12;
    Rectangle apply_r = { (float)(pos_x + PW - 110), (float)y, 94.f, 26.f };
    bool ahov = CheckCollisionPointRec(mouse, apply_r);
    DrawRectangleRec(apply_r, ahov ? Color{ 40, 80, 180, 255 } : Color{ 28, 50, 120, 255 });
    DrawRectangleLinesEx(apply_r, 1.f, { 70, 130, 255, 220 });
    DrawTextF("Aplicar", (int)apply_r.x + 14, (int)apply_r.y + 7, 12, WHITE);
    DrawTextF("Tip: Enter en cualquier campo o click en Aplicar.",
        lx, (int)apply_r.y + 8, 9, { 70, 80, 130, 180 });
    if ((ahov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || any_confirmed) {
        tb.assets_changed = true; tb.ubicaciones_open = false; tb.active_field = -1;
    }
    y += 36;

    // ── Sección 2: generadores Mathlib ────────────────────────────────────────
    // "Actualizar" → incremental (force_full=false). Si el JSON no existe, hace full.
    // "Full"       → force_full=true, regenera desde cero siempre.
    draw_h_line(lx, y, pos_x + PW - lx);
    y += 8;
    DrawTextF("GENERADORES MATHLIB", lx, y, 11, { 100, 180, 255, 220 });
    y += 16;
    DrawTextF(
        "Actualizar: parchea el JSON existente (solo .lean modificados).   "
        "Full: regenera desde cero.",
        lx, y, 9, { 80, 100, 140, 180 });
    y += 14;

    bool src_ok = tb.mathlib_src_path[0] != '\0';
    bool layout_busy = state.mathlib_layout_job.running.load();
    bool deps_busy = state.mathlib_deps_job.running.load();

    const int gbw = 108, gbh = 24, gap = 6;
    int bx = lx;

    // Layout: Actualizar
    if (action_btn(layout_busy ? "..." : "Actlz. Layout",
        bx, y, gbw, gbh, src_ok && !layout_busy, mouse,
        { 28, 60, 130, 255 }, { 40, 90, 180, 255 }, { 20, 25, 45, 160 }))
    {
        MathlibGenOptions opts;
        opts.mathlib_path = tb.mathlib_src_path;
        opts.assets_path = tb.assets_path;
        opts.force_full = false;
        launch_job(state.mathlib_layout_job, opts, generate_mathlib_layout);
    }
    bx += gbw + gap;

    // Layout: Full
    if (action_btn(layout_busy ? "..." : "Full Layout",
        bx, y, gbw - 12, gbh, src_ok && !layout_busy, mouse,
        { 55, 28, 110, 255 }, { 80, 40, 160, 255 }, { 20, 15, 40, 160 }))
    {
        MathlibGenOptions opts;
        opts.mathlib_path = tb.mathlib_src_path;
        opts.assets_path = tb.assets_path;
        opts.force_full = true;
        launch_job(state.mathlib_layout_job, opts, generate_mathlib_layout);
    }
    bx += (gbw - 12) + gap * 3;

    // Deps: Actualizar
    if (action_btn(deps_busy ? "..." : "Actlz. Deps",
        bx, y, gbw, gbh, src_ok && !deps_busy, mouse,
        { 28, 80, 60, 255 }, { 40, 120, 80, 255 }, { 15, 28, 20, 160 }))
    {
        MathlibGenOptions opts;
        opts.mathlib_path = tb.mathlib_src_path;
        opts.assets_path = tb.assets_path;
        opts.force_full = false;
        launch_job(state.mathlib_deps_job, opts, generate_mathlib_deps);
    }
    bx += gbw + gap;

    // Deps: Full
    if (action_btn(deps_busy ? "..." : "Full Deps",
        bx, y, gbw - 12, gbh, src_ok && !deps_busy, mouse,
        { 28, 60, 40, 255 }, { 40, 90, 55, 255 }, { 12, 22, 15, 160 }))
    {
        MathlibGenOptions opts;
        opts.mathlib_path = tb.mathlib_src_path;
        opts.assets_path = tb.assets_path;
        opts.force_full = true;
        launch_job(state.mathlib_deps_job, opts, generate_mathlib_deps);
    }

    y += gbh + 6;

    // Estado de los jobs
    auto draw_job_status = [&](MathlibGenJob& job, int row_y, Color ok, Color err) {
        if (job.status.empty()) return;
        Color c = job.running.load() ? Color{ 200, 190, 60, 220 }
        : job.success ? ok : err;
        DrawTextF(job.status.c_str(), lx, row_y, 10, c);
        };

    draw_job_status(state.mathlib_layout_job, y,
        { 80, 220, 100, 220 }, { 255, 80, 80, 220 });
    y += 14;
    draw_job_status(state.mathlib_deps_job, y,
        { 80, 220, 160, 220 }, { 255, 80, 80, 220 });
    y += 14;

    bool any_done_ok = (state.mathlib_layout_job.done.load() && state.mathlib_layout_job.success)
        || (state.mathlib_deps_job.done.load() && state.mathlib_deps_job.success);
    if (any_done_ok) {
        if (action_btn("Recargar Assets", lx, y, 130, 22, true, mouse,
            { 50, 100, 50, 255 }, { 70, 150, 70, 255 }, {}))
        {
            tb.assets_changed = true;
            state.mathlib_layout_job.done = false;
            state.mathlib_layout_job.status.clear();
            state.mathlib_deps_job.done = false;
            state.mathlib_deps_job.status.clear();
        }
    }
}

void draw_ubicaciones_panel(AppState& state, Vector2 mouse) {
    UbicacionesPanel panel(state);
    panel.draw(mouse);
}