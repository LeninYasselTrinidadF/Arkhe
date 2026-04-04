#include "data/search/loogle.hpp"
#include "raylib.h"
#include <algorithm>
#include <queue>
#include <thread>

// ── Fuzzy local ───────────────────────────────────────────────────────────────

int fuzzy_score(const std::string& query, const std::string& target) {
    if (query.empty()) return 0;
    std::string q = query, t = target;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);

    if (t.find(q) != std::string::npos) return 100 + (int)(50 - t.size());

    int qi = 0, score = 0;
    for (char c : t) {
        if (qi < (int)q.size() && c == q[qi]) { qi++; score += 10; }
    }
    return (qi == (int)q.size()) ? score : 0;
}

std::vector<SearchHit> fuzzy_search(
    const MathNode* root,
    const std::string& query,
    int max_results)
{
    std::vector<SearchHit> hits;
    if (!root || query.empty()) return hits;

    std::queue<std::shared_ptr<MathNode>> sq;
    for (auto& child : root->children) sq.push(child);

    while (!sq.empty() && (int)hits.size() < max_results * 3) {
        auto node = sq.front(); sq.pop();
        int s = std::max(
            fuzzy_score(query, node->code),
            fuzzy_score(query, node->label)
        );
        if (s > 0) hits.push_back({ node, s });
        for (auto& child : node->children) sq.push(child);
    }

    std::sort(hits.begin(), hits.end(),
        [](const SearchHit& a, const SearchHit& b) { return a.score > b.score; });
    if ((int)hits.size() > max_results) hits.resize(max_results);
    return hits;
}

// ── Loogle HTTP ───────────────────────────────────────────────────────────────

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static std::string loogle_get(const std::string& query) {
    // Codificación URL
    std::string encoded;
    for (unsigned char c : query) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            encoded += buf;
        }
    }

    std::wstring host = L"loogle.lean-lang.org";
    std::wstring path = L"/json?q=" + std::wstring(encoded.begin(), encoded.end());

    HINTERNET session = WinHttpOpen(
        L"MathExplorer/1.0",   // ← aquí
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return "";

    HINTERNET connect = WinHttpConnect(session, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) { WinHttpCloseHandle(session); return ""; }

    HINTERNET request = WinHttpOpenRequest(
        connect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    WinHttpAddRequestHeaders(request,
        L"Accept: application/json",
        (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    // Timeout de 8 segundos para no bloquear indefinidamente
    DWORD timeout = 8000;
    WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(request, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(request, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    std::string response;
    bool ok = WinHttpSendRequest(request,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (ok && WinHttpReceiveResponse(request, NULL)) {
        DWORD status = 0, status_size = sizeof(status);
        WinHttpQueryHeaders(request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
            WINHTTP_NO_HEADER_INDEX);

        if (status == 200) {
            DWORD size = 0;
            do {
                WinHttpQueryDataAvailable(request, &size);
                if (size == 0) break;
                std::string chunk(size, '\0');
                DWORD read = 0;
                WinHttpReadData(request, &chunk[0], size, &read);
                chunk.resize(read);
                response += chunk;
            } while (size > 0);
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return response;
}

static std::vector<LoogleResult> parse_loogle(const std::string& body) {
    std::vector<LoogleResult> results;
    if (body.empty()) return results;
    try {
        auto j = json::parse(body);
        if (!j.contains("hits")) return results;
        for (auto& hit : j["hits"]) {
            LoogleResult r;
            r.name = hit.value("name", "");
            r.module = hit.value("module", "");
            r.type_sig = hit.value("type", "");
            if (!r.name.empty()) results.push_back(r);
            if ((int)results.size() >= 20) break;
        }
    }
    catch (...) {}
    return results;
}

#else
static std::string loogle_get(const std::string&) { return ""; }
static std::vector<LoogleResult> parse_loogle(const std::string&) { return {}; }
#endif

// ── Lanzador async ────────────────────────────────────────────────────────────

void loogle_search_async(AppState& state, const std::string& query) {
    if (state.loogle_loading.load()) return;
    state.loogle_loading = true;
    state.loogle_error.clear();
    state.loogle_results.clear();

    std::thread([&state, query]() {
        TraceLog(LOG_INFO, "Loogle: buscando '%s'", query.c_str());
        std::string body = loogle_get(query);

        if (body.empty()) {
            // Sin conexión o timeout — fallo silencioso, no es crítico
            state.loogle_error = "Loogle no disponible (sin conexion)";
            TraceLog(LOG_WARNING, "Loogle: sin respuesta");
        }
        else {
            state.loogle_results = parse_loogle(body);
            if (state.loogle_results.empty()) {
                state.loogle_error = "Sin resultados en Loogle";
            }
            else {
                state.loogle_error.clear();
                TraceLog(LOG_INFO, "Loogle: %d resultados", (int)state.loogle_results.size());
            }
        }
        state.loogle_loading = false;
        }).detach();
}