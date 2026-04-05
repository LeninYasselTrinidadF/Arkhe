#include "data/editor/editor_io.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <functional>

namespace fs = std::filesystem;
using json   = nlohmann::json;

namespace editor_io {

// ── load_index ────────────────────────────────────────────────────────────────

void load_index(const AppState& state,
                std::unordered_map<std::string, std::string>& index)
{
    index.clear();
    std::ifstream f(index_path(state));
    if (!f.is_open()) return;
    try {
        json j = json::parse(f);
        for (auto& [k, v] : j.items())
            index[k] = v.get<std::string>();
    }
    catch (...) {}
}

// ── save_index ────────────────────────────────────────────────────────────────

void save_index(const AppState& state,
                const std::unordered_map<std::string, std::string>& index)
{
    try { fs::create_directories(state.toolbar.entries_path); } catch (...) {}
    json j = json::object();
    for (auto& [k, v] : index) j[k] = v;
    std::ofstream f(index_path(state));
    if (f.is_open()) f << j.dump(2);
}

// ── list_tex_files ────────────────────────────────────────────────────────────

std::vector<std::string> list_tex_files(const AppState& state)
{
    std::vector<std::string> result;
    try {
        for (auto& e : fs::directory_iterator(state.toolbar.entries_path))
            if (e.is_regular_file() && e.path().extension() == ".tex")
                result.push_back(e.path().filename().string());
    }
    catch (...) {}
    std::sort(result.begin(), result.end());
    return result;
}

// ── read_tex / write_tex ──────────────────────────────────────────────────────

std::string read_tex(const AppState& state, const std::string& filename)
{
    std::ifstream f(full_tex_path(state, filename));
    if (!f.is_open()) return {};
    return { std::istreambuf_iterator<char>(f), {} };
}

bool write_tex(const AppState& state,
               const std::string& filename, const std::string& content)
{
    try { fs::create_directories(state.toolbar.entries_path); } catch (...) { return false; }
    std::ofstream f(full_tex_path(state, filename));
    if (!f.is_open()) return false;
    f << content;
    return true;
}

// ── load_node_json ────────────────────────────────────────────────────────────

void load_node_json(const AppState& state, MathNode* sel, EditState& edit)
{
    if (!sel) return;
    std::ifstream f(node_json_path(state, sel->code));
    if (!f.is_open()) return;   // sin JSON: conservar datos del grafo principal

    try {
        auto j = json::parse(f);

        if (j.contains("basic_info")) {
            auto& bi = j["basic_info"];
            if (bi.contains("note"))        sel->note        = bi["note"].get<std::string>();
            if (bi.contains("texture_key")) sel->texture_key = bi["texture_key"].get<std::string>();
            if (bi.contains("msc_refs"))    sel->msc_refs    = bi["msc_refs"].get<std::vector<std::string>>();
        }

        if (j.contains("cross_refs")) {
            edit.cross_refs.clear();
            for (auto& cr : j["cross_refs"]) {
                EditorCrossRef ref;
                ref.target_code = cr.value("target_code", "");
                ref.relation    = cr.value("relation",    "see_also");
                if (!ref.target_code.empty())
                    edit.cross_refs.push_back(std::move(ref));
            }
        }

        if (j.contains("resources")) {
            sel->resources.clear();
            for (auto& r : j["resources"]) {
                Resource res;
                res.kind    = r.value("kind",    "link");
                res.title   = r.value("title",   "");
                res.content = r.value("content", "");
                if (!res.title.empty())
                    sel->resources.push_back(std::move(res));
            }
        }
    }
    catch (...) {}
}

// ── save_node_json ────────────────────────────────────────────────────────────

void save_node_json(const AppState& state,
                    const MathNode* sel,
                    const EditState& edit)
{
    if (!sel) return;
    try { fs::create_directories(state.toolbar.entries_path); } catch (...) { return; }

    json j;

    // basic_info: datos ya reflejados en sel (live-edit)
    j["basic_info"]["note"]        = sel->note;
    j["basic_info"]["texture_key"] = sel->texture_key;
    j["basic_info"]["msc_refs"]    = sel->msc_refs;

    // cross_refs
    json cr_arr = json::array();
    for (auto& cr : edit.cross_refs) {
        json o;
        o["target_code"] = cr.target_code;
        o["relation"]    = cr.relation;
        cr_arr.push_back(o);
    }
    j["cross_refs"] = cr_arr;

    // resources
    json res_arr = json::array();
    for (auto& r : sel->resources) {
        json o;
        o["kind"]    = r.kind;
        o["title"]   = r.title;
        o["content"] = r.content;
        res_arr.push_back(o);
    }
    j["resources"] = res_arr;

    std::ofstream f(node_json_path(state, sel->code));
    if (f.is_open()) f << j.dump(2);
}

// ── Traverse helper (interno) ─────────────────────────────────────────────────

static void traverse(MathNode* node, const std::function<void(MathNode*)>& fn)
{
    if (!node) return;
    fn(node);
    for (auto& child : node->children)
        traverse(child.get(), fn);
}

// ── acople_basic_info ─────────────────────────────────────────────────────────

void acople_basic_info(AppState& state, MathNode* root)
{
    traverse(root, [&](MathNode* node) {
        std::ifstream f(node_json_path(state, node->code));
        if (!f.is_open()) return;
        try {
            auto j = json::parse(f);
            if (!j.contains("basic_info")) return;
            auto& bi = j["basic_info"];
            if (bi.contains("note"))        node->note        = bi["note"].get<std::string>();
            if (bi.contains("texture_key")) node->texture_key = bi["texture_key"].get<std::string>();
            if (bi.contains("msc_refs"))    node->msc_refs    = bi["msc_refs"].get<std::vector<std::string>>();
        } catch (...) {}
    });
}

// ── acople_cross_refs ─────────────────────────────────────────────────────────

void acople_cross_refs(AppState& state, MathNode* root)
{
    traverse(root, [&](MathNode* node) {
        std::ifstream f(node_json_path(state, node->code));
        if (!f.is_open()) return;
        try {
            auto j = json::parse(f);
            if (!j.contains("cross_refs")) return;
            node->cross_refs.clear();
            for (auto& cr : j["cross_refs"]) {
                EditorCrossRef ref;
                ref.target_code = cr.value("target_code", "");
                ref.relation    = cr.value("relation",    "see_also");
                if (!ref.target_code.empty())
                    node->cross_refs.push_back(std::move(ref));
            }
        } catch (...) {}
    });
}

// ── acople_resources ──────────────────────────────────────────────────────────

void acople_resources(AppState& state, MathNode* root)
{
    traverse(root, [&](MathNode* node) {
        std::ifstream f(node_json_path(state, node->code));
        if (!f.is_open()) return;
        try {
            auto j = json::parse(f);
            if (!j.contains("resources")) return;
            node->resources.clear();
            for (auto& r : j["resources"]) {
                Resource res;
                res.kind    = r.value("kind",    "link");
                res.title   = r.value("title",   "");
                res.content = r.value("content", "");
                if (!res.title.empty())
                    node->resources.push_back(std::move(res));
            }
        } catch (...) {}
    });
}

} // namespace editor_io
