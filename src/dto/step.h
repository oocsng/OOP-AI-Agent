#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace agent {

struct Step {
    std::size_t step_id{0};
    std::string thought;
    std::string action;
    std::string tool_result;
    std::size_t tokens_used{0};
    double latency_ms{0.0};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Step, step_id, thought, action, tool_result, tokens_used, latency_ms)
};

} // namespace agent
