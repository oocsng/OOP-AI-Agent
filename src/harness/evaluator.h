#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "dto/trajectory.h"

namespace agent::harness {

struct TaskSpec {
    std::string task_id;
    std::string prompt;
    std::vector<std::string> expected_keywords;
    std::string test_command;
    std::string category;
};

struct EvalResult {
    bool passed{false};
    double score{0.0}; // 0.0 to 1.0
    std::string feedback;
};

class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual EvalResult evaluate(const Trajectory& trajectory, const TaskSpec& spec) = 0;
};

} // namespace agent::harness
