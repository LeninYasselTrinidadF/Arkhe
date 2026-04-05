#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// Resultados de formateo de etiqueta para una burbuja.
struct BubbleLabelLines {
    std::vector<std::string> lines;
    bool needs_abbrev = false;
};

// Utilidades para calcular y acotar etiquetas de burbujas.
std::string split_camel(const std::string& s);
int word_count(const std::string& s);
float fit_radius_for_label(const std::string& label, float base_r, float max_r, int font_size);
BubbleLabelLines make_label_lines(const std::string& label, float radius, int font_size);
std::unordered_map<std::string, std::string> build_child_abbrev_map(const std::vector<std::string>& labels);
std::string short_label(const std::string& label, float radius);
