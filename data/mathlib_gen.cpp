#include "mathlib_gen.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
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

// Elimina separadores finales para que fs::path::filename() funcione en Windows.
// "C:\foo\bar\"  → fs::path("C:\foo\bar")  → filename() == "bar"  OK
// "C:\foo\bar"   → sin cambio              → filename() == "bar"  OK
static fs::path normalize_input_path(const std::string& p) {
    std::string s = p;
    while (!s.empty() && (s.back() == '/' || s.back() == '\\'))
        s.pop_back();
    return fs::path(s);
}

// ─────────────────────────────────────────────────────────────────────────────
// GENERADOR DE LAYOUT
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

static json build_folder_node(const fs::path& folder, const fs::path& repo_root,
    int max_decls, int min_decls,
    int& total_folders, int& total_files, int& total_decls)
{
    total_folders++;
    json node;
    node["code"] = lean_module_name(repo_root, folder);
    node["label"] = folder.filename().string();
    node["path"] = fs::relative(folder, repo_root).string();
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
                    max_decls, min_decls,
                    total_folders, total_files, total_decls);
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
                file_node["path"] = fs::relative(entry.path(), repo_root).string();
                file_node["declarations"] = json::array();
                for (auto& d : decls) {
                    json dj;
                    dj["kind"] = d.kind;
                    dj["name"] = d.name;
                    dj["line"] = d.line;
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

void generate_mathlib_layout(const MathlibGenOptions& opts, MathlibGenJob& job) {
    job.running = true;
    job.done = false;
    job.success = false;
    job.status = "Iniciando...";

    // normalize_input_path quita el separador final para que filename() funcione
    // correctamente en Windows con rutas tipo "C:\foo\bar\" devueltas por tinyfd.
    std::error_code ec;
    fs::path input_path = normalize_input_path(opts.mathlib_path);
    fs::path repo_root;
    fs::path mathlib_dir;

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
        job.running = false;
        job.done = true;
        return;
    }

    job.status = "Escaneando archivos .lean...";

    int total_folders = 0, total_files = 0, total_decls = 0;
    json root_node = build_folder_node(mathlib_dir, repo_root,
        opts.max_decls, opts.min_decls,
        total_folders, total_files, total_decls);

    json output = json::object();
    output["root"] = "Mathlib";
    output["folders"] = root_node["children"];
    output["files"] = root_node["files"];

    std::string out_path = ensure_slash(opts.assets_path) + "mathlib_layout.json";
    std::ofstream f(out_path);
    if (!f.is_open()) {
        job.status = "ERROR: No se pudo escribir " + out_path;
        job.running = false;
        job.done = true;
        return;
    }
    f << output.dump(2);
    f.close();

    job.folders = total_folders;
    job.files = total_files;
    job.decls = total_decls;
    job.success = true;
    job.status = "Layout OK: " + std::to_string(total_files) + " archivos, "
        + std::to_string(total_decls) + " declaraciones";
    job.running = false;
    job.done = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// GENERADOR DE DEPENDENCIAS
// ─────────────────────────────────────────────────────────────────────────────

// ── extract_imports ───────────────────────────────────────────────────────────
// Lee solo el preámbulo del archivo .lean y devuelve los módulos importados.
//
// Máquina de estado:
//   · Ignora bloques /- ... -/ (multilinea, incluido /-! y /--)
//   · Ignora comentarios de línea --
//   · Recoge líneas "import Module.Name"
//   · Detiene al encontrar cualquier otra línea no vacía (namespace, open, theorem, …)
//
// En Lean 4, los `import` deben aparecer antes que cualquier otro código.

static std::vector<std::string> extract_imports(const fs::path& path)
{
    std::vector<std::string> imports;
    std::ifstream f(path);
    if (!f.is_open()) return imports;

    std::string line;
    bool in_block = false;
    bool seen_import = false;  // tras el primer import, el primer codigo real termina

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (in_block) {
            if (line.find("-/") != std::string::npos)
                in_block = false;
            continue;
        }

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;

        const char* p = line.c_str() + first;
        size_t      rest = line.size() - first;

        // Bloque /- ... -/ (incluye /-! docstrings de modulo)
        if (rest >= 2 && p[0] == '/' && p[1] == '-') {
            const char* close = strstr(p + 2, "-/");
            if (!close) in_block = true;
            continue;
        }

        // Comentario de linea --
        if (rest >= 2 && p[0] == '-' && p[1] == '-') continue;

        // Directivas import: "import X", "public import X", "private import X"
        // Lean 4 / Mathlib usa "public import" en archivos de barril (.olean)
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

        // Una vez que vimos imports, cualquier linea de codigo real termina el preambulo
        if (seen_import) break;

        // Antes del primer import: ignorar "module", "set_option", "prelude", etc.
        // Solo terminamos si es codigo real inequivoco
        static const char* CODE_KW[] = {
            "namespace ", "open ", "section ", "theorem ", "lemma ",
            "def ", "class ", "structure ", "instance ", "variable ",
            "noncomputable ", "attribute ", "#", nullptr
        };
        for (int i = 0; CODE_KW[i]; i++) {
            size_t klen = strlen(CODE_KW[i]);
            if (rest >= klen && strncmp(p, CODE_KW[i], klen) == 0)
                goto done;
        }
        continue;  // "module", "set_option" u otra directiva pre-import: seguimos
    }
done:
    return imports;
}

void generate_mathlib_deps(const MathlibGenOptions& opts, MathlibGenJob& job) {
    job.running = true;
    job.done = false;
    job.success = false;
    job.status = "Iniciando...";

    // normalize_input_path quita el separador final para que filename() funcione
    // correctamente en Windows con rutas tipo "C:\foo\bar\" devueltas por tinyfd.
    std::error_code ec;
    fs::path input_path = normalize_input_path(opts.mathlib_path);
    fs::path repo_root;
    fs::path mathlib_dir;

    if (input_path.filename() == "Mathlib" && fs::is_directory(input_path, ec) && !ec) {
        // El usuario apuntó directamente a Mathlib/
        mathlib_dir = input_path;
        repo_root = input_path.parent_path();
    }
    else {
        // El usuario apuntó a la raíz del repo; buscamos Mathlib/ dentro
        repo_root = input_path;
        mathlib_dir = repo_root / "Mathlib";
    }

    ec.clear();
    if (!fs::exists(mathlib_dir, ec) || ec) {
        job.status = "ERROR: No se encontro Mathlib/ en " + opts.mathlib_path;
        job.running = false;
        job.done = true;
        return;
    }

    job.status = "Extrayendo imports...";

    // ── FIX 2: inicializar output como objeto JSON explícito ─────────────────
    // Sin esto, si el loop no escribe nada, output.dump() produce "null".
    json output = json::object();
    int total_files = 0, total_edges = 0;

    // ── FIX 3: verificar que el iterador se construyó correctamente ───────────
    ec.clear();
    fs::recursive_directory_iterator it(
        mathlib_dir,
        fs::directory_options::skip_permission_denied,
        ec
    );

    if (ec) {
        job.status = "ERROR: No se pudo leer " + mathlib_dir.string() + " : " + ec.message();
        job.running = false;
        job.done = true;
        return;
    }

    fs::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lean") continue;
        if (entry.path().filename() == "lakefile.lean") continue;

        std::string module = lean_module_name(repo_root, entry.path());
        if (module.empty()) continue;

        // Solo procesar módulos que pertenezcan a Mathlib.*
        // (lean_module_name devuelve "Mathlib.X.Y" para archivos dentro de Mathlib/)
        if (module.rfind("Mathlib.", 0) != 0) continue;

        auto imports = extract_imports(entry.path());

        // Filtrar solo imports de Mathlib.*
        std::vector<std::string> mathlib_imports;
        for (auto& imp : imports)
            if (imp.rfind("Mathlib.", 0) == 0)
                mathlib_imports.push_back(imp);

        // Descartar solo archivos sin ningun import en absoluto (no aportan info al grafo)
        if (imports.empty() && mathlib_imports.empty()) continue;

        // Label: último componente del módulo
        std::string label = module;
        size_t dot = label.rfind('.');
        if (dot != std::string::npos) label = label.substr(dot + 1);

        json node = json::object();
        node["label"] = label;
        node["depends_on"] = mathlib_imports;   // array vacío si no hay deps Mathlib

        output[module] = std::move(node);
        total_files++;
        total_edges += (int)mathlib_imports.size();

        if (total_files % 200 == 0)
            job.status = "Procesando... " + std::to_string(total_files) + " modulos";
    }

    // ── FIX 5: reportar si no se encontró ningún archivo ─────────────────────
    if (total_files == 0) {
        job.status = "ERROR: No se encontraron archivos .lean en " + mathlib_dir.string()
            + " (verifica que la ruta apunta a la raiz del repo mathlib4)";
        job.running = false;
        job.done = true;
        return;
    }

    std::string out_path = ensure_slash(opts.assets_path) + "deps_mathlib.json";
    std::ofstream f(out_path);
    if (!f.is_open()) {
        job.status = "ERROR: No se pudo escribir " + out_path;
        job.running = false;
        job.done = true;
        return;
    }
    f << output.dump(2);
    f.close();

    job.files = total_files;
    job.dep_edges = total_edges;
    job.success = true;
    job.status = "Deps OK: " + std::to_string(total_files) + " modulos, "
        + std::to_string(total_edges) + " aristas";
    job.running = false;
    job.done = true;
}