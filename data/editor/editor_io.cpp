#include "data/editor/editor_io.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

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
    try { fs::create_directories(state.toolbar.entries_path); }
    catch (...) {}

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

// ── read_tex ──────────────────────────────────────────────────────────────────

std::string read_tex(const AppState& state, const std::string& filename)
{
    std::ifstream f(full_tex_path(state, filename));
    if (!f.is_open()) return {};
    return { std::istreambuf_iterator<char>(f), {} };
}

// ── write_tex ─────────────────────────────────────────────────────────────────

bool write_tex(const AppState& state,
               const std::string& filename, const std::string& content)
{
    try { fs::create_directories(state.toolbar.entries_path); }
    catch (...) { return false; }

    std::ofstream f(full_tex_path(state, filename));
    if (!f.is_open()) return false;
    f << content;
    return true;
}

} // namespace editor_io
