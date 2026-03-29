#pragma once
#include "../app.hpp"
#include "raylib.h"
#include "../data/math_node.hpp"
#include <string>

// Dibuja el bloque de descripción (texto plano o fuente .tex + widget LaTeX).
// Devuelve la coordenada Y donde terminó el bloque.
//
// sel          : nodo seleccionado (puede ser nullptr)
// tex_target   : código que se usa como nombre del .tex cargado
// cached_raw   : contenido crudo del .tex (vacío si no hay)
// cached_display : versión "limpia" del .tex para mostrar como texto
// mouse        : posición del mouse (para el botón Render LaTeX)
// x, y         : posición inicial de dibujo
// w            : ancho disponible
// scroll_h     : alto del área scrollable (para calcular escala de la imagen)
int draw_description_block(AppState& state,
                           MathNode* sel,
                           const std::string& tex_target,
                           const std::string& cached_raw,
                           const std::string& cached_display,
                           Vector2 mouse,
                           int x, int y, int w, int scroll_h);

// Lanza la compilación asíncrona de LaTeX para `raw_tex` (sin bloquear).
void launch_latex_render(AppState& state,
                         const std::string& code,
                         const std::string& raw_tex);

// Debe llamarse cada frame para detectar cuando el hilo terminó y
// cargar la textura resultante.
void poll_latex_render(AppState& state);

// Carga el archivo <entries_path>/<code>.tex y devuelve su contenido.
// Devuelve "" si no existe.
std::string load_entry_tex(const std::string& entries_path,
                           const std::string& code);

// Convierte LaTeX crudo a texto legible (sin comandos, sin $).
std::string tex_to_display(const std::string& tex);
