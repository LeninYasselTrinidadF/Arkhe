#include "data/editor/body_parser.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// parse_body
// ─────────────────────────────────────────────────────────────────────────────

std::vector<BodySegment> parse_body(const std::string& body)
{
    std::vector<BodySegment> segs;
    std::string cur_text;
    size_t i = 0;
    const size_t n = body.size();

    auto flush_text = [&]() {
        if (!cur_text.empty()) {
            segs.push_back({ BodySegment::Type::Text, std::move(cur_text) });
            cur_text.clear();
        }
    };

    while (i < n) {
        if (body[i] == '$') {
            // ── $$...$$ → DisplayMath ─────────────────────────────────────────
            if (i + 1 < n && body[i + 1] == '$') {
                flush_text();
                size_t end = body.find("$$", i + 2);
                if (end == std::string::npos) {
                    // Sin cerrar: tratar resto como texto
                    cur_text += body.substr(i);
                    break;
                }
                segs.push_back({
                    BodySegment::Type::DisplayMath,
                    body.substr(i, end + 2 - i)   // incluye los $$
                });
                i = end + 2;
            }
            // ── $...$ → InlineMath ────────────────────────────────────────────
            else {
                flush_text();
                size_t end = body.find('$', i + 1);
                if (end == std::string::npos) {
                    cur_text += body.substr(i);
                    break;
                }
                segs.push_back({
                    BodySegment::Type::InlineMath,
                    body.substr(i, end + 1 - i)   // incluye los $
                });
                i = end + 1;
            }
        } else {
            cur_text += body[i++];
        }
    }
    flush_text();
    return segs;
}
