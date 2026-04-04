#include "ui/views/bubble/bubble_label.hpp"
#include "ui/aesthetic/font_manager.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

// ── split_camel ───────────────────────────────────────────────────────────────

std::string split_camel(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (int i = 0; i < (int)s.size(); ++i) {
        if (i > 0 && std::isupper((unsigned char)s[i])
            && (std::islower((unsigned char)s[i - 1])
                || std::isdigit((unsigned char)s[i - 1])))
            out += ' ';
        out += s[i];
    }
    return out;
}

// ── word_count ────────────────────────────────────────────────────────────────

int word_count(const std::string& s) {
    int n = 0; bool in_word = false;
    for (unsigned char c : s) {
        if (std::isspace(c)) { in_word = false; }
        else { if (!in_word) ++n; in_word = true; }
    }
    return n;
}

// ── Helpers internos ──────────────────────────────────────────────────────────

static std::vector<std::string> split_words(const std::string& s) {
    std::vector<std::string> words;
    std::istringstream ss(s);
    std::string w;
    while (ss >> w) words.push_back(w);
    return words;
}

static const std::unordered_set<std::string> k_skip_words = {
    "and","of","the","a","an","in","for","by","to","with","from","on","at"
};

static std::string make_abbrev(const std::vector<std::string>& words, int letters = 1) {
    if (letters <= 1) {
        std::string abbr;
        for (auto& w : words) {
            std::string lw = w;
            for (char& c : lw) c = (char)tolower((unsigned char)c);
            if (!k_skip_words.count(lw) && !w.empty())
                abbr += (char)toupper((unsigned char)w[0]);
        }
        return abbr;
    }
    // Primera palabra significativa con `letters` letras + iniciales del resto
    std::string result;
    bool first_done = false;
    for (auto& w : words) {
        std::string lw = w;
        for (char& c : lw) c = (char)tolower((unsigned char)c);
        if (k_skip_words.count(lw)) continue;
        if (!first_done) {
            result += w.substr(0, std::min(letters, (int)w.size()));
            first_done = true;
        } else if (!w.empty()) {
            result += (char)toupper((unsigned char)w[0]);
        }
    }
    return result;
}

static std::unordered_map<std::string, std::string>
resolve_abbrevs(const std::vector<std::string>& labels) {
    std::unordered_map<std::string, std::string> result;
    std::unordered_map<std::string, std::vector<std::string>> by_abbr;

    for (auto& lbl : labels) {
        auto words = split_words(lbl);
        std::string abbr = make_abbrev(words, 1);
        by_abbr[abbr].push_back(lbl);
        result[lbl] = abbr;
    }
    for (auto& [abbr, group] : by_abbr) {
        if (group.size() <= 1) continue;
        for (int extra = 2; extra <= 8; ++extra) {
            std::unordered_map<std::string, std::vector<std::string>> check;
            for (auto& lbl : group) {
                auto words = split_words(lbl);
                check[make_abbrev(words, extra)].push_back(lbl);
            }
            bool all_unique = true;
            for (auto& [a2, g2] : check) {
                if (g2.size() > 1) all_unique = false;
                else               result[g2[0]] = a2;
            }
            if (all_unique) break;
        }
    }
    return result;
}

static inline float usable_width (float r) { return r * 2.0f * 0.72f; }
static inline float usable_height(float r) { return r * 2.0f * 0.60f; }

static std::vector<std::string> word_wrap(const std::vector<std::string>& words,
                                          float max_w, int font_size)
{
    std::vector<std::string> lines;
    std::string cur;
    for (auto& w : words) {
        std::string test = cur.empty() ? w : cur + " " + w;
        if (MeasureTextF(test.c_str(), font_size) <= (int)max_w) {
            cur = test;
        } else {
            if (!cur.empty()) lines.push_back(cur);
            cur = w;
        }
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

static bool lines_fit(const std::vector<std::string>& lines, float r, int font_size) {
    float uw = usable_width(r), uh = usable_height(r);
    int line_h = MeasureTextF("Ag", font_size);
    int total_h = (int)lines.size() * line_h + ((int)lines.size() - 1) * 2;
    if ((float)total_h > uh) return false;
    for (auto& ln : lines)
        if (MeasureTextF(ln.c_str(), font_size) > (int)uw) return false;
    return true;
}

// ── fit_radius_for_label ──────────────────────────────────────────────────────

float fit_radius_for_label(const std::string& label, float base_r,
                           float max_r, int font_size)
{
    auto words = split_words(label);
    if (words.empty()) return base_r;
    for (float r = base_r; r <= max_r + 0.5f; r += 1.0f) {
        auto lines = word_wrap(words, usable_width(r), font_size);
        if (lines_fit(lines, r, font_size)) return r;
    }
    return max_r;
}

// ── make_label_lines ──────────────────────────────────────────────────────────

BubbleLabelLines make_label_lines(const std::string& label, float radius,
                                  int font_size)
{
    BubbleLabelLines out;
    auto words = split_words(label);
    if (words.empty()) return out;

    int   n_words  = (int)words.size();
    float uw       = usable_width(radius);
    float uh       = usable_height(radius);
    int   line_h   = MeasureTextF("Ag", font_size);
    int   max_lines = std::max(1, (int)((uh + 2) / (line_h + 2)));

    auto wrapped = word_wrap(words, uw, font_size);

    if ((int)wrapped.size() <= max_lines && lines_fit(wrapped, radius, font_size)) {
        out.lines = wrapped;
        return out;
    }

    if (n_words >= 4 && n_words <= 6) {
        std::vector<std::string> truncated;
        for (int i = 0; i < (int)wrapped.size() && i < max_lines; i++)
            truncated.push_back(wrapped[i]);

        if (!truncated.empty()) {
            std::string& last = truncated.back();
            if (MeasureTextF((last + "...").c_str(), font_size) <= (int)uw) {
                last += "...";
            } else {
                auto lw = split_words(last);
                while (lw.size() > 1) {
                    lw.pop_back();
                    std::string candidate;
                    for (size_t i = 0; i < lw.size(); i++) {
                        if (i) candidate += " ";
                        candidate += lw[i];
                    }
                    if (MeasureTextF((candidate + "...").c_str(), font_size) <= (int)uw) {
                        last = candidate + "..."; break;
                    }
                }
                if (lw.size() == 1) { out.needs_abbrev = true; return out; }
            }
            out.lines = truncated;
            return out;
        }
    }

    out.needs_abbrev = true;
    return out;
}

// ── build_child_abbrev_map ────────────────────────────────────────────────────

std::unordered_map<std::string, std::string>
build_child_abbrev_map(const std::vector<std::string>& labels) {
    return resolve_abbrevs(labels);
}

// ── short_label (legacy) ──────────────────────────────────────────────────────

std::string short_label(const std::string& label, float radius) {
    auto result = make_label_lines(label, radius, 15);
    if (result.needs_abbrev) {
        return make_abbrev(split_words(label), 1);
    }
    if (result.lines.size() == 1) return result.lines[0];
    std::string out;
    for (size_t i = 0; i < result.lines.size(); ++i) {
        if (i) out += ", ";
        out += result.lines[i];
    }
    return out;
}
