#include "mathlib_loader.hpp"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

using json = nlohmann::json;

static void layout_ring(MathNode& parent, float ring_radius) {
    int total = (int)parent.children.size();
    if (total == 0) return;
    for (int i = 0; i < total; i++) {
        float angle = (i / (float)total) * 2.0f * PI - PI / 2.0f;
        parent.children[i]->x = cosf(angle) * ring_radius;
        parent.children[i]->y = sinf(angle) * ring_radius;
    }
}

// Colores por carpeta raiz (igual que el script Python original)
static Color color_for_group(const std::string& label) {
    // Mapeo de los grupos principales del script Python
    struct { const char* name; unsigned char r, g, b; } groups[] = {
        {"Algebra",         255, 255,   0},
        {"Analysis",          0, 255, 255},
        {"CategoryTheory",  128, 160, 255},
        {"Topology",        255,   0, 255},
        {"NumberTheory",    128,   0,   0},
        {"LinearAlgebra",     0, 255,   0},
        {"MeasureTheory",   128,   0, 255},
        {"RingTheory",      255, 128,   0},
        {"GroupTheory",     255,  32,  64},
        {"Order",           128,  64,   0},
        {"Data",             64,  64,  64},
        {"Tactic",           64,  64, 128},
        {"Geometry",        255, 128, 255},
        {"Probability",       0,   0, 255},
        {"SetTheory",       255, 128, 128},
    };
    for (auto& g : groups)
        if (label == g.name)
            return {g.r, g.g, g.b, 255};
    return ColorFromHSV((float)(label[0] * 37 % 360), 0.6f, 0.9f);
}

// Nivel 3: declaraciones como nodos hoja
static std::shared_ptr<MathNode> make_decl(const json& decl, Color base_color) {
    auto n = std::make_shared<MathNode>();
    n->code   = decl.value("kind", "def") + " " + decl.value("name", "?");
    n->label  = decl.value("name", "?");
    n->color  = Fade(base_color, 0.85f);
    n->radius = 16.0f;
    n->level  = NodeLevel::Topic;
    return n;
}

// Nivel 2: archivo .lean
static std::shared_ptr<MathNode> make_file(const json& file, Color base_color, float hue) {
    auto n = std::make_shared<MathNode>();
    n->code   = file.value("code",  "");
    n->label  = file.value("label", "");
    n->color  = ColorFromHSV(hue, 0.45f, 1.0f);
    n->radius = 24.0f;
    n->level  = NodeLevel::Subsection;

    if (file.contains("declarations")) {
        for (auto& decl : file["declarations"])
            n->children.push_back(make_decl(decl, n->color));
        layout_ring(*n, 220.0f);
    }
    return n;
}

// Recursivo: carpeta -> subcarpetas + archivos
static std::shared_ptr<MathNode> make_folder(const json& folder, Color base_color,
                                              float hue, NodeLevel level) {
    auto n = std::make_shared<MathNode>();
    n->code   = folder.value("code",  "");
    n->label  = folder.value("label", "");
    n->color  = ColorFromHSV(hue, 0.60f, 0.92f);
    n->level  = level;

    switch (level) {
        case NodeLevel::Macro:      n->radius = 52.0f; break;
        case NodeLevel::Section:    n->radius = 38.0f; break;
        case NodeLevel::Subsection: n->radius = 28.0f; break;
        default:                    n->radius = 22.0f; break;
    }

    NodeLevel child_level = (level == NodeLevel::Macro)    ? NodeLevel::Section    :
                            (level == NodeLevel::Section)   ? NodeLevel::Subsection :
                                                              NodeLevel::Topic;

    // Subcarpetas
    if (folder.contains("children")) {
        int total = (int)folder["children"].size();
        int i = 0;
        for (auto& child : folder["children"]) {
            float child_hue = hue + i * (30.0f / (total + 1));
            n->children.push_back(make_folder(child, base_color, child_hue, child_level));
            i++;
        }
    }

    // Archivos .lean
    if (folder.contains("files")) {
        int total = (int)folder["files"].size();
        int i = 0;
        for (auto& file : folder["files"]) {
            float file_hue = hue + i * (20.0f / (total + 1));
            n->children.push_back(make_file(file, base_color, file_hue));
            i++;
        }
    }

    float ring_r = (level == NodeLevel::Macro) ? 340.0f :
                   (level == NodeLevel::Section) ? 300.0f : 260.0f;
    layout_ring(*n, ring_r);
    return n;
}

std::shared_ptr<MathNode> load_mathlib(const std::string& path) {
    std::ifstream f(path);
    TraceLog(LOG_INFO, "Mathlib: abriendo %s", path.c_str());
    if (!f.is_open()) {
        TraceLog(LOG_ERROR, "No se pudo abrir: %s", path.c_str());
        return nullptr;
    }

    json data = json::parse(f);

    auto root = std::make_shared<MathNode>();
    root->code  = "ROOT";
    root->label = "Mathlib";
    root->level = NodeLevel::Root;

    if (!data.contains("folders")) {
        TraceLog(LOG_ERROR, "Mathlib: JSON sin 'folders'");
        return nullptr;
    }

    auto& folders = data["folders"];
    int total = (int)folders.size();
    TraceLog(LOG_INFO, "Mathlib: %d carpetas raiz", total);

    int i = 0;
    for (auto& folder : folders) {
        std::string label = folder.value("label", "");
        Color base_color  = color_for_group(label);
        float hue         = i * (360.0f / total);
        root->children.push_back(
            make_folder(folder, base_color, hue, NodeLevel::Macro));
        i++;
    }

    layout_ring(*root, 340.0f);
    TraceLog(LOG_INFO, "Mathlib cargado: %d nodos raiz", (int)root->children.size());
    return root;
}
