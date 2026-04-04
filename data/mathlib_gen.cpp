#include "mathlib_gen.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers compartidos
// ─────────────────────────────────────────────────────────────────────────────

static std::string lean_module_name(const fs::path& repo_root, const fs::path& file_path) {
    std::error_code ec;
    auto rel = fs::relative(file_path, repo_root, ec);
    if (ec) return file_path.stem().string();
    std::string result;
    for (auto it = rel.begin(); it != rel.end(); ++it) {
        if (!result.empty()) result += '.';
        std::string part = it->string();
        if (part.size() > 5 && part.substr(part.size() - 5) == ".lean")
            part = part.substr(0, part.size() - 5);
        result += part;
    }
    return result;
}

static std::string ensure_slash(const std::string& p) {
    if (p.empty()) return p;
    return (p.back() != '/' && p.back() != '\\') ? p + '/' : p;
}

static fs::path normalize_input_path(const std::string& p) {
    std::string s = p;
    while (!s.empty() && (s.back() == '/' || s.back() == '\\'))
        s.pop_back();
    return fs::path(s);
}

// Normaliza separadores a '/' para usar como clave en los índices.
static std::string norm_sep(std::string s) {
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

// Convierte file_time_type a unix timestamp (compatible con C++17).
static int64_t file_time_to_unix(fs::file_time_type ft) {
    using namespace std::chrono;
    auto now_sys = system_clock::now();
    auto now_ft = fs::file_time_type::clock::now();
    auto delta = duration_cast<system_clock::duration>(ft - now_ft);
    return duration_cast<seconds>((now_sys + delta).time_since_epoch()).count();
}

static int64_t current_unix_time() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Extracción de declaraciones (sin cambios respecto a la versión anterior)
// ─────────────────────────────────────────────────────────────────────────────

struct DeclInfo {
    std::string kind;
    std::string name;
    int         line = 0;
};

static const struct { const char* prefix; const char* kind; } DECL_PREFIXES[] = {
    { "theorem ",           "theorem"   },
    { "lemma ",             "lemma"     },
    { "noncomputable def ", "def"       },
    { "def ",               "def"       },
    { "abbrev ",            "abbrev"    },
    { "class ",             "class"     },
    { "structure ",         "structure" },
    { "instance ",          "instance"  },
    { "instance:",          "instance"  },
    { "inductive ",         "inductive" },
    { "axiom ",             "axiom"     },
};

static bool starts_with_ws(const std::string& line, const char* prefix) {
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
    return line.compare(i, strlen(prefix), prefix) == 0;
}

static std::string extract_name_after(const std::string& line, const char* prefix) {
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
    i += strlen(prefix);
    size_t start = i;
    while (i < line.size() && line[i] != ' ' && line[i] != ':' &&
        line[i] != '(' && line[i] != '{' && line[i] != '\n') i++;
    return line.substr(start, i - start);
}

static std::vector<DeclInfo> extract_declarations(const fs::path& path, int max_decls) {
    std::vector<DeclInfo> result;
    std::ifstream f(path);
    if (!f.is_open()) return result;
    std::string line;
    int lineno = 0;
    while (std::getline(f, line) && (int)result.size() < max_decls) {
        lineno++;
        size_t non_ws = line.find_first_not_of(" \t");
        if (non_ws != std::string::npos && line[non_ws] == '-' &&
            non_ws + 1 < line.size() && line[non_ws + 1] == '-')
            continue;
        for (auto& [prefix, kind] : DECL_PREFIXES) {
            if (starts_with_ws(line, prefix)) {
                std::string name = extract_name_after(line, prefix);
                if (name.empty()) name = "(anon)";
                result.push_back({ kind, name, lineno });
                break;
            }
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Generación completa del árbol de layout (igual que antes)
// ─────────────────────────────────────────────────────────────────────────────

static json build_folder_node(const fs::path& folder, const fs::path& repo_root,
    int max_decls, int min_decls,
    int& total_folders, int& total_files, int& total_decls)
{
    total_folders++;
    json node;
    node["code"] = lean_module_name(repo_root, folder);
    node["label"] = folder.filename().string();
    node["path"] = norm_sep(fs::relative(folder, repo_root).string());
    node["children"] = json::array();
    node["files"] = json::array();

    std::vector<fs::directory_entry> entries;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(folder, ec)) {
        if (ec) { ec.clear(); continue; }
        entries.push_back(e);
    }
    std::sort(entries.begin(), entries.end(),
        [](auto& a, auto& b) { return a.path() < b.path(); });

    for (auto& entry : entries) {
        if (entry.is_directory()) {
            const auto& name = entry.path().filename().string();
            if (!name.empty() && name[0] != '.') {
                json child = build_folder_node(entry.path(), repo_root,
                    max_decls, min_decls, total_folders, total_files, total_decls);
                if (!child["children"].empty() || !child["files"].empty())
                    node["children"].push_back(std::move(child));
            }
        }
        else if (entry.path().extension() == ".lean" &&
            entry.path().filename() != "lakefile.lean") {
            auto decls = extract_declarations(entry.path(), max_decls);
            if ((int)decls.size() >= min_decls) {
                json file_node;
                file_node["code"] = lean_module_name(repo_root, entry.path());
                file_node["label"] = entry.path().stem().string();
                file_node["path"] = norm_sep(fs::relative(entry.path(), repo_root).string());
                file_node["declarations"] = json::array();
                for (auto& d : decls) {
                    json dj;
                    dj["kind"] = d.kind; dj["name"] = d.name; dj["line"] = d.line;
                    file_node["declarations"].push_back(std::move(dj));
                }
                total_files++;
                total_decls += (int)decls.size();
                node["files"].push_back(std::move(file_node));
            }
        }
    }
    return node;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers para modo incremental del layout
// ─────────────────────────────────────────────────────────────────────────────

// Recorre recursivamente el árbol JSON y llena los índices.
// folder_index: path_normalizado -> puntero al nodo folder
// file_index:   path_normalizado -> puntero al nodo de archivo
static void index_folder_recursive(json& node,
    std::unordered_map<std::string, json*>& folder_index,
    std::unordered_map<std::string, json*>& file_index)
{
    if (node.contains("path"))
        folder_index[node["path"].get<std::string>()] = &node;

    if (node.contains("files"))
        for (auto& f : node["files"])
            if (f.contains("path"))
                file_index[f["path"].get<std::string>()] = &f;

    if (node.contains("children"))
        for (auto& child : node["children"])
            index_folder_recursive(child, folder_index, file_index);
}

static void build_layout_index(json& root,
    std::unordered_map<std::string, json*>& folder_index,
    std::unordered_map<std::string, json*>& file_index)
{
    // Archivos directamente en root (raros, pero posibles)
    if (root.contains("files"))
        for (auto& f : root["files"])
            if (f.contains("path"))
                file_index[f["path"].get<std::string>()] = &f;

    if (root.contains("folders"))
        for (auto& folder : root["folders"])
            index_folder_recursive(folder, folder_index, file_index);
}

// Elimina del árbol los nodos de archivo cuyo path no está en fs_paths.
static void prune_node(json& node,
    const std::unordered_set<std::string>& fs_paths,
    int& removed)
{
    if (node.contains("files")) {
        json kept = json::array();
        for (auto& f : node["files"]) {
            std::string p = f.contains("path")
                ? norm_sep(f["path"].get<std::string>()) : std::string{};
            if (p.empty() || fs_paths.count(p))
                kept.push_back(std::move(f));
            else
                removed++;
        }
        node["files"] = std::move(kept);
    }
    if (node.contains("children"))
        for (auto& child : node["children"])
            prune_node(child, fs_paths, removed);
}

static void prune_missing_files(json& root,
    const std::unordered_set<std::string>& fs_paths,
    int& removed)
{
    if (root.contains("files")) {
        json kept = json::array();
        for (auto& f : root["files"]) {
            std::string p = f.contains("path")
                ? norm_sep(f["path"].get<std::string>()) : std::string{};
            if (p.empty() || fs_paths.count(p))
                kept.push_back(std::move(f));
            else
                removed++;
        }
        root["files"] = std::move(kept);
    }
    if (root.contains("folders"))
        for (auto& folder : root["folders"])
            prune_node(folder, fs_paths, removed);
}

// ─────────────────────────────────────────────────────────────────────────────
// GENERADOR DE LAYOUT
// ─────────────────────────────────────────────────────────────────────────────

void generate_mathlib_layout(const MathlibGenOptions& opts, MathlibGenJob& job) {
    job.running = true;
    job.done = false;
    job.success = false;
    job.status = "Iniciando...";
    job.updated = job.added = job.removed = job.skipped = 0;

    std::error_code ec;
    fs::path input_path = normalize_input_path(opts.mathlib_path);
    fs::path repo_root, mathlib_dir;

    if (input_path.filename() == "Mathlib" && fs::is_directory(input_path, ec) && !ec) {
        mathlib_dir = input_path;
        repo_root = input_path.parent_path();
    }
    else {
        repo_root = input_path;
        mathlib_dir = repo_root / "Mathlib";
    }

    ec.clear();
    if (!fs::exists(mathlib_dir, ec) || ec) {
        job.status = "ERROR: No se encontro Mathlib/ en " + opts.mathlib_path;
        job.running = false; job.done = true;
        return;
    }

    std::string out_path = ensure_slash(opts.assets_path) + "mathlib_layout.json";

    // ── Intentar cargar el JSON existente para modo incremental ───────────────
    json existing;
    bool has_existing = false;
    int64_t prev_ts = 0;

    if (!opts.force_full) {
        std::ifstream ef(out_path);
        if (ef.is_open()) {
            try {
                existing = json::parse(ef);
                has_existing = existing.contains("folders");
                if (has_existing && existing.contains("generated_at_unix"))
                    prev_ts = existing["generated_at_unix"].get<int64_t>();
            }
            catch (...) {
                has_existing = false;
            }
        }
    }

    // ── MODO FULL ─────────────────────────────────────────────────────────────
    if (!has_existing) {
        job.status = "Generacion completa: escaneando .lean...";

        int total_folders = 0, total_files = 0, total_decls = 0;
        json root_node = build_folder_node(mathlib_dir, repo_root,
            opts.max_decls, opts.min_decls,
            total_folders, total_files, total_decls);

        json output;
        output["root"] = "Mathlib";
        output["generated_at_unix"] = current_unix_time();
        output["folders"] = root_node["children"];
        output["files"] = root_node["files"];

        std::ofstream f(out_path);
        if (!f.is_open()) {
            job.status = "ERROR: No se pudo escribir " + out_path;
            job.running = false; job.done = true;
            return;
        }
        f << output.dump(2);

        job.folders = total_folders;
        job.files = total_files;
        job.decls = total_decls;
        job.success = true;
        job.status = "Full OK: " + std::to_string(total_files) + " archivos, "
            + std::to_string(total_decls) + " declaraciones";
        job.running = false; job.done = true;
        return;
    }

    // ── MODO INCREMENTAL ──────────────────────────────────────────────────────
    job.status = "Modo incremental: construyendo indice...";

    // Índices de lo que ya hay en el JSON
    std::unordered_map<std::string, json*> folder_index;   // path -> nodo folder
    std::unordered_map<std::string, json*> file_index;     // path -> nodo archivo
    build_layout_index(existing, folder_index, file_index);

    // Para los archivos nuevos: acumular (folder_path -> vector<json>) para hacer
    // push_back al FINAL, después de usar los punteros de file_index.
    std::unordered_map<std::string, std::vector<json>> pending_adds; // folder_path -> nuevos archivos
    std::unordered_set<std::string> fs_file_paths; // todos los .lean encontrados en el fs

    int skipped_new_folder = 0; // archivos nuevos cuya carpeta no existe en el JSON

    job.status = "Modo incremental: escaneando cambios...";

    ec.clear();
    fs::recursive_directory_iterator it(mathlib_dir,
        fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        job.status = "ERROR: No se pudo iterar " + mathlib_dir.string();
        job.running = false; job.done = true;
        return;
    }

    fs::recursive_directory_iterator end_it;
    int checked = 0;

    for (; it != end_it; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lean") continue;
        if (entry.path().filename() == "lakefile.lean") continue;

        std::string rel_path = norm_sep(
            fs::relative(entry.path(), repo_root, ec).string());
        if (ec) { ec.clear(); continue; }

        fs_file_paths.insert(rel_path);

        // mtime del archivo
        auto ft = fs::last_write_time(entry.path(), ec);
        if (ec) { ec.clear(); ft = fs::file_time_type{}; }
        int64_t mt = (ec) ? 0 : file_time_to_unix(ft);

        if (++checked % 500 == 0)
            job.status = "Incremental: " + std::to_string(checked) + " revisados...";

        auto it_file = file_index.find(rel_path);

        if (it_file != file_index.end()) {
            // Archivo ya existente en el JSON
            if (mt > prev_ts) {
                // Modificado: re-extraer declaraciones
                auto decls = extract_declarations(entry.path(), opts.max_decls);
                json& fnode = *it_file->second;
                fnode["declarations"] = json::array();
                for (auto& d : decls) {
                    json dj;
                    dj["kind"] = d.kind; dj["name"] = d.name; dj["line"] = d.line;
                    fnode["declarations"].push_back(std::move(dj));
                }
                job.updated++;
            }
            else {
                job.skipped++;
            }
        }
        else {
            // Archivo nuevo: buscar su carpeta en el índice
            std::string folder_rel = norm_sep(
                fs::relative(entry.path().parent_path(), repo_root, ec).string());
            if (ec) { ec.clear(); skipped_new_folder++; continue; }

            auto it_folder = folder_index.find(folder_rel);
            if (it_folder == folder_index.end()) {
                // Carpeta nueva: no la creamos en modo incremental
                skipped_new_folder++;
                continue;
            }

            // Carpeta existe: construir nodo de archivo y acumular
            auto decls = extract_declarations(entry.path(), opts.max_decls);
            if ((int)decls.size() < opts.min_decls) continue;

            json file_node;
            file_node["code"] = lean_module_name(repo_root, entry.path());
            file_node["label"] = entry.path().stem().string();
            file_node["path"] = rel_path;
            file_node["declarations"] = json::array();
            for (auto& d : decls) {
                json dj;
                dj["kind"] = d.kind; dj["name"] = d.name; dj["line"] = d.line;
                file_node["declarations"].push_back(std::move(dj));
            }
            pending_adds[folder_rel].push_back(std::move(file_node));
            job.added++;
        }
    }

    // Ahora sí: push_back de archivos nuevos (punteros de file_index ya no se usan)
    for (auto& [fpath, new_files] : pending_adds) {
        auto it_folder = folder_index.find(fpath);
        if (it_folder == folder_index.end()) continue;
        for (auto& nf : new_files)
            it_folder->second->at("files").push_back(nf);
    }

    // Eliminar archivos que ya no existen en el fs
    prune_missing_files(existing, fs_file_paths, job.removed);

    // Actualizar timestamp
    existing["generated_at_unix"] = current_unix_time();

    // Guardar
    std::ofstream f(out_path);
    if (!f.is_open()) {
        job.status = "ERROR: No se pudo escribir " + out_path;
        job.running = false; job.done = true;
        return;
    }
    f << existing.dump(2);
    f.close();

    job.success = true;
    job.status = "Incremental OK — upd:" + std::to_string(job.updated)
        + " add:" + std::to_string(job.added)
        + " del:" + std::to_string(job.removed)
        + " skip:" + std::to_string(job.skipped);
    if (skipped_new_folder > 0)
        job.status += " (+" + std::to_string(skipped_new_folder)
        + " en carpetas nuevas, usa Full para incluirlos)";

    job.running = false; job.done = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Extracción de imports (sin cambios)
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<std::string> extract_imports(const fs::path& path)
{
    std::vector<std::string> imports;
    std::ifstream f(path);
    if (!f.is_open()) return imports;

    std::string line;
    bool in_block = false;
    bool seen_import = false;

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (in_block) {
            if (line.find("-/") != std::string::npos) in_block = false;
            continue;
        }

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;

        const char* p = line.c_str() + first;
        size_t      rest = line.size() - first;

        if (rest >= 2 && p[0] == '/' && p[1] == '-') {
            if (!strstr(p + 2, "-/")) in_block = true;
            continue;
        }
        if (rest >= 2 && p[0] == '-' && p[1] == '-') continue;

        {
            const char* imp_start = nullptr;
            if (rest >= 7 && strncmp(p, "import ", 7) == 0) imp_start = p + 7;
            else if (rest >= 14 && strncmp(p, "public import ", 14) == 0) imp_start = p + 14;
            else if (rest >= 15 && strncmp(p, "private import ", 15) == 0) imp_start = p + 15;

            if (imp_start) {
                seen_import = true;
                std::string mod(imp_start);
                size_t comm = mod.find("--");
                if (comm != std::string::npos) mod = mod.substr(0, comm);
                while (!mod.empty() && (mod.back() == ' ' || mod.back() == '\r'))
                    mod.pop_back();
                size_t ms = mod.find_first_not_of(" \t");
                if (ms != std::string::npos) mod = mod.substr(ms);
                if (!mod.empty()) imports.push_back(std::move(mod));
                continue;
            }
        }

        if (seen_import) break;

        static const char* CODE_KW[] = {
            "namespace ", "open ", "section ", "theorem ", "lemma ",
            "def ", "class ", "structure ", "instance ", "variable ",
            "noncomputable ", "attribute ", "#", nullptr
        };
        for (int i = 0; CODE_KW[i]; i++) {
            size_t klen = strlen(CODE_KW[i]);
            if (rest >= klen && strncmp(p, CODE_KW[i], klen) == 0)
                goto done_imports;
        }
    }
done_imports:
    return imports;
}

// ─────────────────────────────────────────────────────────────────────────────
// GENERADOR DE DEPENDENCIAS
// ─────────────────────────────────────────────────────────────────────────────

void generate_mathlib_deps(const MathlibGenOptions& opts, MathlibGenJob& job) {
    job.running = true;
    job.done = false;
    job.success = false;
    job.status = "Iniciando...";
    job.updated = job.added = job.removed = job.skipped = 0;

    std::error_code ec;
    fs::path input_path = normalize_input_path(opts.mathlib_path);
    fs::path repo_root, mathlib_dir;

    if (input_path.filename() == "Mathlib" && fs::is_directory(input_path, ec) && !ec) {
        mathlib_dir = input_path;
        repo_root = input_path.parent_path();
    }
    else {
        repo_root = input_path;
        mathlib_dir = repo_root / "Mathlib";
    }

    ec.clear();
    if (!fs::exists(mathlib_dir, ec) || ec) {
        job.status = "ERROR: No se encontro Mathlib/ en " + opts.mathlib_path;
        job.running = false; job.done = true;
        return;
    }

    std::string out_path = ensure_slash(opts.assets_path) + "deps_mathlib.json";

    // ── Intentar cargar el JSON existente ─────────────────────────────────────
    json output = json::object();
    bool has_existing = false;
    int64_t prev_ts = 0;

    if (!opts.force_full) {
        std::ifstream ef(out_path);
        if (ef.is_open()) {
            try {
                json loaded = json::parse(ef);
                // El JSON de deps es un objeto plano {module: {label, depends_on}}
                // más un campo especial "_generated_at_unix"
                if (!loaded.is_null()) {
                    output = std::move(loaded);
                    has_existing = true;
                    if (output.contains("_generated_at_unix"))
                        prev_ts = output["_generated_at_unix"].get<int64_t>();
                }
            }
            catch (...) {
                output = json::object();
                has_existing = false;
            }
        }
    }

    job.status = has_existing
        ? "Modo incremental: escaneando imports..."
        : "Generacion completa: escaneando imports...";

    // Conjunto de módulos encontrados en el fs (para detectar eliminados)
    std::unordered_set<std::string> fs_modules;

    ec.clear();
    fs::recursive_directory_iterator it(mathlib_dir,
        fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        job.status = "ERROR: No se pudo iterar " + mathlib_dir.string();
        job.running = false; job.done = true;
        return;
    }

    int total_files = 0, total_edges = 0, checked = 0;
    fs::recursive_directory_iterator end_it;

    for (; it != end_it; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lean") continue;
        if (entry.path().filename() == "lakefile.lean") continue;

        std::string module = lean_module_name(repo_root, entry.path());
        if (module.empty() || module.rfind("Mathlib.", 0) != 0) continue;

        fs_modules.insert(module);

        if (has_existing) {
            // Modo incremental: procesar solo si cambió
            auto ft = fs::last_write_time(entry.path(), ec);
            if (ec) { ec.clear(); ft = fs::file_time_type{}; }
            int64_t mt = file_time_to_unix(ft);

            if (output.contains(module) && mt <= prev_ts) {
                // Sin cambios
                job.skipped++;
                auto& node = output[module];
                total_edges += (int)node.value("depends_on", json::array()).size();
                total_files++;
                continue;
            }
        }

        auto imports = extract_imports(entry.path());
        std::vector<std::string> mathlib_imports;
        for (auto& imp : imports)
            if (imp.rfind("Mathlib.", 0) == 0)
                mathlib_imports.push_back(imp);

        if (imports.empty() && mathlib_imports.empty() && !has_existing) continue;

        std::string label = module;
        size_t dot = label.rfind('.');
        if (dot != std::string::npos) label = label.substr(dot + 1);

        bool is_new = !output.contains(module);

        json node = json::object();
        node["label"] = label;
        node["depends_on"] = mathlib_imports;
        output[module] = std::move(node);

        if (is_new) job.added++;
        else        job.updated++;

        total_files++;
        total_edges += (int)mathlib_imports.size();

        if (++checked % 200 == 0)
            job.status = (has_existing ? "Incremental: " : "Procesando: ")
            + std::to_string(checked) + " modificados...";
    }

    if (total_files == 0 && !has_existing) {
        job.status = "ERROR: No se encontraron archivos .lean en " + mathlib_dir.string();
        job.running = false; job.done = true;
        return;
    }

    // Eliminar módulos que ya no existen en el fs
    if (has_existing) {
        std::vector<std::string> to_remove;
        for (auto& [key, _] : output.items()) {
            if (key == "_generated_at_unix") continue;
            if (!fs_modules.count(key))
                to_remove.push_back(key);
        }
        for (auto& k : to_remove) { output.erase(k); job.removed++; }
    }

    // Guardar timestamp en el propio JSON (campo especial prefijado con _)
    output["_generated_at_unix"] = current_unix_time();

    std::ofstream f(out_path);
    if (!f.is_open()) {
        job.status = "ERROR: No se pudo escribir " + out_path;
        job.running = false; job.done = true;
        return;
    }
    f << output.dump(2);
    f.close();

    job.files = total_files;
    job.dep_edges = total_edges;
    job.success = true;

    if (has_existing)
        job.status = "Incremental OK — upd:" + std::to_string(job.updated)
        + " add:" + std::to_string(job.added)
        + " del:" + std::to_string(job.removed)
        + " skip:" + std::to_string(job.skipped);
    else
        job.status = "Full OK: " + std::to_string(total_files) + " modulos, "
        + std::to_string(total_edges) + " aristas";

    job.running = false; job.done = true;
}