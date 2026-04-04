#include "msc_loader.hpp"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

using json = nlohmann::json;

// ── Layout en anillo ──────────────────────────────────────────────────────────
static void layout_ring(MathNode& parent, float ring_radius) {
    int total = (int)parent.children.size();
    if (total == 0) return;
    for (int i = 0; i < total; i++) {
        float angle = (i / (float)total) * 2.0f * PI - PI / 2.0f;
        parent.children[i]->x = cosf(angle) * ring_radius;
        parent.children[i]->y = sinf(angle) * ring_radius;
    }
}

// ── Helpers de construcción ───────────────────────────────────────────────────
static std::shared_ptr<MathNode> make_topic(const json& top, float hue) {
    auto n = std::make_shared<MathNode>();
    n->code   = top["code"].get<std::string>();
    n->label  = top["label"].get<std::string>();
    n->note   = top.value("note", "");
    n->color  = ColorFromHSV(hue, 0.40f, 1.0f);
    n->radius = 22.0f;
    n->level  = NodeLevel::Topic;
    return n;
}

static std::shared_ptr<MathNode> make_subsection(const json& sub, float hue) {
    auto n = std::make_shared<MathNode>();
    n->code   = sub["full_code"].get<std::string>();
    n->label  = sub["label"].get<std::string>();
    n->note   = sub.value("note", "");
    n->color  = ColorFromHSV(hue, 0.50f, 0.98f);
    n->radius = 30.0f;
    n->level  = NodeLevel::Subsection;

    int total = (int)sub["topics"].size();
    int i = 0;
    for (auto& top : sub["topics"]) {
        float th = hue + i * (15.0f / (total + 1));
        n->children.push_back(make_topic(top, th));
        i++;
    }
    layout_ring(*n, 260.0f);
    return n;
}

static std::shared_ptr<MathNode> make_section(const json& sec, float hue) {
    auto n = std::make_shared<MathNode>();
    n->code   = sec["full_code"].get<std::string>();
    n->label  = sec["label"].get<std::string>();
    n->note   = sec.value("note", "");
    n->color  = ColorFromHSV(hue, 0.60f, 0.95f);
    n->radius = 38.0f;
    n->level  = NodeLevel::Section;

    int total = (int)sec["subsections"].size();
    int i = 0;
    for (auto& sub : sec["subsections"]) {
        float sh = hue + i * (20.0f / (total + 1));
        n->children.push_back(make_subsection(sub, sh));
        i++;
    }
    layout_ring(*n, 280.0f);
    return n;
}

// ── Carga principal ───────────────────────────────────────────────────────────
std::shared_ptr<MathNode> load_msc2020(const std::string& path) {
    std::ifstream f(path);
    TraceLog(LOG_INFO, "MSC2020: abriendo %s", path.c_str());
    if (!f.is_open()) {
        TraceLog(LOG_ERROR, "No se pudo abrir: %s", path.c_str());
        return nullptr;
    }

    json data = json::parse(f);

    auto root = std::make_shared<MathNode>();
    root->code  = "ROOT";
    root->label = "MSC2020";
    root->level = NodeLevel::Root;

    bool has_macros = data.contains("macro_areas") && !data["macro_areas"].empty();

    if (has_macros) {
        auto& macros   = data["macro_areas"];
        int   total_mac = (int)macros.size();
        TraceLog(LOG_INFO, "MSC2020: %d macro-areas", total_mac);

        int mi = 0;
        for (auto& mac : macros) {
            auto mac_node   = std::make_shared<MathNode>();
            mac_node->label  = mac["label"].get<std::string>();
            mac_node->code   = mac_node->label.substr(0, 8);
            mac_node->color  = ColorFromHSV(mi * (360.0f / total_mac), 0.70f, 0.92f);
            mac_node->radius = 52.0f;
            mac_node->level  = NodeLevel::Macro;

            float hue_base = mi * (360.0f / total_mac);
            int   total_sec = (int)mac["sections"].size();
            int   si = 0;
            for (auto& sec : mac["sections"]) {
                float hue = hue_base + si * (25.0f / (total_sec + 1));
                mac_node->children.push_back(make_section(sec, hue));
                si++;
            }
            layout_ring(*mac_node, 340.0f);
            root->children.push_back(mac_node);
            mi++;
        }
    } else {
        // Fallback sin macro_areas
        auto& sections  = data["sections"];
        int   total_sec = (int)sections.size();
        TraceLog(LOG_INFO, "MSC2020 (sin macros): %d secciones", total_sec);
        int si = 0;
        for (auto& sec : sections) {
            float hue = si * (360.0f / total_sec);
            root->children.push_back(make_section(sec, hue));
            si++;
        }
    }

    layout_ring(*root, 340.0f);
    TraceLog(LOG_INFO, "MSC2020 cargado: %d nodos raiz", (int)root->children.size());
    return root;
}
