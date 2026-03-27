#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>

enum class ViewMode { MSC2020, Mathlib, Standard };
enum class NodeLevel { Root, Macro, Section, Subsection, Topic };

struct Resource {
    std::string kind;    // "book" | "link" | "explanation" | "latex"
    std::string title;
    std::string content; // URL, texto, o formula LaTeX
};

struct MathNode {
    std::string  code;
    std::string  label;
    std::string  note;           // notas MSC ej. "[See also 05Cxx]"
    std::string  latex_symbol;   // formula LaTeX si aplica, ej. "\\int_a^b"
    NodeLevel    level  = NodeLevel::Root;
    float        radius = 38.0f;
    Color        color  = WHITE;
    float        x = 0.0f, y = 0.0f;
    std::vector<std::shared_ptr<MathNode>> children;
    std::vector<Resource>                  resources;
    Texture2D    latex_tex = {};  // PNG pre-renderizado, cargado en runtime
};
