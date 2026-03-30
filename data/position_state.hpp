#pragma once
#include "../app.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

static constexpr const char* POSITION_STATE_FILE = "state_positions.json";

inline void position_state_save(const AppState& state) {
    nlohmann::json j;
    for (auto& [k,v] : state.temp_positions) {
        j[k] = { v.x, v.y };
    }
    try {
        std::ofstream f(POSITION_STATE_FILE);
        if (f.is_open()) f << j.dump(2);
    } catch(...) {}
}

inline void position_state_load(AppState& state) {
    nlohmann::json j;
    try {
        std::ifstream f(POSITION_STATE_FILE);
        if (!f.is_open()) return;
        j = nlohmann::json::parse(f);
    } catch(...) { return; }
    state.temp_positions.clear();
    if (!j.is_object()) return;
    for (auto& it : j.items()) {
        auto arr = it.value();
        if (!arr.is_array() || arr.size() < 2) continue;
        state.temp_positions[it.key()] = { (float)arr[0].get<double>(), (float)arr[1].get<double>() };
    }
}
