#include "ui/aesthetic/nine_patch.hpp"
#include "ui/aesthetic/skin.hpp"
#include "raylib.h"
#include "raymath.h"

// ── NinePatch::draw ───────────────────────────────────────────────────────────

void NinePatch::draw(Rectangle dest, Color tint) const {
    if (!valid()) return;

    const float tw = (float)tex.width;
    const float th = (float)tex.height;

    // Ancho/alto de la zona central de la textura
    float cx_w = tw - left - right;
    float cx_h = th - top  - bottom;

    // Ancho/alto de la zona central del destino
    float dc_w = dest.width  - left - right;
    float dc_h = dest.height - top  - bottom;

    // Clamp: si el destino es más pequeño que los márgenes, escalar todo
    if (dc_w < 0) dc_w = 0;
    if (dc_h < 0) dc_h = 0;

    // Coordenadas en destino
    float dx = dest.x, dy = dest.y;
    float dw = dest.width, dh = dest.height;

    // ── 9 rectángulos fuente → destino ────────────────────────────────────────
    // Columnas: left | center | right
    // Filas:    top  | center | bottom

    struct Patch { Rectangle src; Rectangle dst; };
    Patch patches[9] = {
        // top-left
        { {0,       0,      (float)left,  (float)top},
          {dx,      dy,     (float)left,  (float)top} },
        // top-center
        { {(float)left, 0,  cx_w, (float)top},
          {dx+(float)left, dy, dc_w, (float)top} },
        // top-right
        { {tw-(float)right, 0, (float)right, (float)top},
          {dx+dw-(float)right, dy, (float)right, (float)top} },

        // mid-left
        { {0, (float)top, (float)left, cx_h},
          {dx, dy+(float)top, (float)left, dc_h} },
        // mid-center
        { {(float)left, (float)top, cx_w, cx_h},
          {dx+(float)left, dy+(float)top, dc_w, dc_h} },
        // mid-right
        { {tw-(float)right, (float)top, (float)right, cx_h},
          {dx+dw-(float)right, dy+(float)top, (float)right, dc_h} },

        // bot-left
        { {0, th-(float)bottom, (float)left, (float)bottom},
          {dx, dy+dh-(float)bottom, (float)left, (float)bottom} },
        // bot-center
        { {(float)left, th-(float)bottom, cx_w, (float)bottom},
          {dx+(float)left, dy+dh-(float)bottom, dc_w, (float)bottom} },
        // bot-right
        { {tw-(float)right, th-(float)bottom, (float)right, (float)bottom},
          {dx+dw-(float)right, dy+dh-(float)bottom, (float)right, (float)bottom} },
    };

    for (auto& p : patches) {
        if (p.dst.width <= 0 || p.dst.height <= 0) continue;
        DrawTexturePro(tex, p.src, p.dst, {0,0}, 0.0f, tint);
    }
}

// ── Shader de máscara circular ────────────────────────────────────────────────
// GLSL 330 (OpenGL 3.3, compatible con raylib en desktop Windows)

static const char* CIRCLE_MASK_FRAG = R"(
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;

// Centro del círculo en coordenadas de pantalla (pixels)
uniform vec2  u_center;
// Radio en pixels
uniform float u_radius;
// Tamaño de la textura en pixels (para convertir fragTexCoord → pixel)
uniform vec2  u_tex_size;

out vec4 finalColor;

void main() {
    vec2 pixel = fragTexCoord * u_tex_size;
    float dist = length(pixel - u_center);
    // Suavizado de 1.5px en el borde para antialiasing
    float alpha = 1.0 - smoothstep(u_radius - 1.5, u_radius + 0.5, dist);
    vec4 col = texture(texture0, fragTexCoord) * fragColor;
    finalColor = vec4(col.rgb, col.a * alpha);
}
)";

// Raylib usa un vertex shader por defecto; lo replicamos mínimamente
static const char* CIRCLE_MASK_VERT = R"(
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
)";

CircleMaskShader g_circle_mask;

void CircleMaskShader::load() {
    shader    = LoadShaderFromMemory(CIRCLE_MASK_VERT, CIRCLE_MASK_FRAG);
    loc_center   = GetShaderLocation(shader, "u_center");
    loc_radius   = GetShaderLocation(shader, "u_radius");
    loc_tex_size = GetShaderLocation(shader, "u_tex_size");
}

void CircleMaskShader::unload() {
    if (valid()) UnloadShader(shader);
    shader = {};
}

void CircleMaskShader::set_params(float cx, float cy, float radius) const {
    if (!valid()) return;
    float center[2] = { cx, cy };
    SetShaderValue(shader, loc_center,   center, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, loc_radius,   &radius, SHADER_UNIFORM_FLOAT);
}

// ── draw_circle_texture ───────────────────────────────────────────────────────
// Dibuja la textura centrada en (cx,cy) escalada para cubrir el radio,
// recortada en círculo mediante el shader.

void draw_circle_texture(Texture2D tex, float cx, float cy, float radius,
                         Color tint)
{
    if (tex.id == 0) return;

    // Escalar para "cover": la imagen cubre todo el círculo
    float scale = (radius * 2.0f) / (float)std::min(tex.width, tex.height);
    float dw = tex.width  * scale;
    float dh = tex.height * scale;

    Rectangle src  = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dest = { cx - dw * 0.5f, cy - dh * 0.5f, dw, dh };

    if (g_circle_mask.valid()) {
        // El shader necesita coordenadas de pantalla relativas a la textura
        // que se está dibujando (fragTexCoord * u_tex_size = pixel en dest)
        float tex_size[2] = { dw, dh };
        // Centro en coordenadas locales de la textura dibujada
        float local_cx = cx - dest.x;
        float local_cy = cy - dest.y;
        float center[2] = { local_cx, local_cy };

        SetShaderValue(g_circle_mask.shader, g_circle_mask.loc_center,
                       center, SHADER_UNIFORM_VEC2);
        SetShaderValue(g_circle_mask.shader, g_circle_mask.loc_radius,
                       &radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_circle_mask.shader, g_circle_mask.loc_tex_size,
                       tex_size, SHADER_UNIFORM_VEC2);

        BeginShaderMode(g_circle_mask.shader);
        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, tint);
        EndShaderMode();
    } else {
        // Fallback sin shader: dibuja sin recorte circular
        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, tint);
    }
}

// ── np_draw_* helpers ─────────────────────────────────────────────────────────

void np_draw_panel(float x, float y, float w, float h, Color tint) {
    g_skin.panel.draw(x, y, w, h, tint);
}
void np_draw_button(float x, float y, float w, float h, Color tint) {
    g_skin.button.draw(x, y, w, h, tint);
}
void np_draw_field(float x, float y, float w, float h, Color tint) {
    g_skin.field.draw(x, y, w, h, tint);
}
void np_draw_card(float x, float y, float w, float h, Color tint) {
    g_skin.card.draw(x, y, w, h, tint);
}
void np_draw_toolbar(float x, float y, float w, float h, Color tint) {
    g_skin.toolbar_bg.draw(x, y, w, h, tint);
}

void np_draw_bubble(float cx, float cy, float radius, Color tint) {
    if (!g_skin.bubble.valid()) return;
    draw_circle_texture(g_skin.bubble.tex, cx, cy, radius, tint);
}
