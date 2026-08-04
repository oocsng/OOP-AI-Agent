#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "step.h"

namespace agent {

struct Trajectory {
    std::string task_id;
    std::string model;
    bool success{false};
    std::string timestamp;
    std::size_t total_tokens{0};
    double total_time_ms{0.0};
    double execution_duration_ms{0.0};
    std::vector<Step> steps;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Trajectory, task_id, timestamp, model, success, total_tokens, total_time_ms, execution_duration_ms, steps)
};

} // namespace agent
