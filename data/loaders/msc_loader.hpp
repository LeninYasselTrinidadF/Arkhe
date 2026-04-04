#pragma once
#include "data/math_node.hpp"
#include <string>
#include <memory>

std::shared_ptr<MathNode> load_msc2020(const std::string& path);
