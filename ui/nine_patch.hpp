#pragma once
#include "raylib.h"

// ── NinePatch ─────────────────────────────────────────────────────────────────
// Divide una textura en 9 zonas y la escala sin deformar las esquinas.
//
//  ┌────┬──────────┬────┐
//  │ TL │   TOP    │ TR │   ← esquinas: fijas (margen px)
//  ├────┼──────────┼────┤
//  │ L  │  CENTER  │  R │   ← laterales/centro: escalan
//  ├────┼──────────┼────┤
//  │ BL │  BOTTOM  │ BR │
//  └────┴──────────┴────┘
//
// Uso:
//   NinePatch np{ tex, 12, 12, 12, 12 };
//   np.draw({ x, y, w, h }, WHITE);
// ─────────────────────────────────────────────────────────────────────────────

struct NinePatch {
    Texture2D tex  = {};    // textura fuente (id==0 → no dibuja nada)
    int top    = 8;         // margen superior en píxeles de la textura
    int right  = 8;         // margen derecho
    int bottom = 8;         // margen inferior
    int left   = 8;         // margen izquierdo

    bool valid() const { return tex.id > 0; }

    // Dibuja el nine-patch escalado al rectángulo dest.
    // tint: COLOR multiplicativo (WHITE = sin tinte).
    void draw(Rectangle dest, Color tint = WHITE) const;

    // Variante con posición + tamaño
    void draw(float x, float y, float w, float h, Color tint = WHITE) const {
        draw({ x, y, w, h }, tint);
    }
};

// ── Shader de máscara circular ────────────────────────────────────────────────
// Recorta en círculo la textura que se dibuje dentro de BeginShaderMode/EndShaderMode.
// center: centro del círculo en coordenadas de PANTALLA (no de mundo).
// radius: radio en píxeles de pantalla.

struct CircleMaskShader {
    Shader shader = {};
    int loc_center = -1;
    int loc_radius = -1;
    int loc_tex_size = -1;

    bool valid() const { return shader.id > 0; }

    // Carga el shader desde código fuente embebido (no necesita archivos externos).
    void load();
    void unload();

    // Configura los uniforms antes de BeginShaderMode.
    void set_params(float cx, float cy, float radius) const;
};

// Singleton global (se carga una vez en main después de InitWindow).
extern CircleMaskShader g_circle_mask;

// ── Helpers de dibujo con nine-patch ─────────────────────────────────────────
// Estas funciones usan la skin global (g_skin) internamente.
// Se declaran aquí para que los archivos de UI no necesiten incluir skin.hpp.

void np_draw_panel  (float x, float y, float w, float h, Color tint = WHITE);
void np_draw_button (float x, float y, float w, float h, Color tint = WHITE);
void np_draw_field  (float x, float y, float w, float h, Color tint = WHITE);
void np_draw_card   (float x, float y, float w, float h, Color tint = WHITE);
void np_draw_toolbar(float x, float y, float w, float h, Color tint = WHITE);

// Dibuja una textura recortada en círculo usando g_circle_mask.
// La textura se escala para cubrir el círculo completo (cover).
void draw_circle_texture(Texture2D tex, float cx, float cy, float radius,
                         Color tint = WHITE);

// Versión nine-patch circular: usa el nine-patch de burbuja.
void np_draw_bubble(float cx, float cy, float radius, Color tint = WHITE);
