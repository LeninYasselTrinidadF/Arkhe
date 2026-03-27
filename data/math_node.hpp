#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>

enum class ViewMode  { MSC2020, Mathlib, Standard };
enum class NodeLevel { Root, Macro, Section, Subsection, Topic };

struct Resource {
    std::string kind;     // "book", "link", "explanation", "latex"
    std::string title;
    std::string content;
};

struct MathNode {
    std::string code;
    std::string label;
    std::string note;
    std::string latex_symbol;

    NodeLevel level  = NodeLevel::Root;
    float     radius = 40.0f;
    Color     color  = {200, 200, 220, 255};
    float     x = 0, y = 0;

    std::vector<std::shared_ptr<MathNode>> children;
    std::vector<Resource>                  resources;

    // Referencias cruzadas (inyectadas desde crossref.json)
    std::vector<std::string> msc_refs;       // ej: "34A12"
    std::vector<std::string> standard_refs;  // ej: "ODE.ExistenceUniqueness"

    Texture2D latex_tex = {};
};
