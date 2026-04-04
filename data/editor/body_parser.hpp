#pragma once
// ── body_parser.hpp ───────────────────────────────────────────────────────────
// Divide un cuerpo LaTeX en segmentos Text / InlineMath / DisplayMath.
//
// Reglas:
//   $$...$$  → DisplayMath   (bloque centrado, renderer lo escala a ancho)
//   $...$    → InlineMath    (dentro de una línea de texto)
//   resto    → Text          (incluye saltos de línea normales)
//
// Limitaciones deliberadas de V1:
//   · No soporta \[ \] ni entornos equation/align (extensible después).
//   · Los $ no escapados se tratan como delimitadores.
//   · La cadena 'content' de un segmento math incluye los delimitadores
//     ($ o $$), lo que permite pasarla directamente como src al compilador.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>

struct BodySegment {
    enum class Type { Text, InlineMath, DisplayMath };
    Type        type;
    std::string content;   // texto puro o expresión LaTeX con delimitadores
};

/// Divide `body` en segmentos ordenados.
/// Nunca lanza excepción. Si hay delimitadores sin cerrar, el resto se trata como Text.
std::vector<BodySegment> parse_body(const std::string& body);
