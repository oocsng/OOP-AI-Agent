#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace agent {

enum class LoopStatus {
    Normal,
    Warning,
    CriticalStop
};

struct LoopCheckResult {
    LoopStatus status{LoopStatus::Normal};
    std::string message;
};

struct ToolCallRecord {
    std::string tool_name;
    std::string args_dump;
};

class LoopDetector {
private:
    std::vector<ToolCallRecord> history_;
    std::size_t repeat_warning_threshold_{2};
    std::size_t repeat_stop_threshold_{3};
    std::size_t pingpong_stop_threshold_{2}; // A-B-A-B repeated pairs

public:
    LoopDetector(std::size_t repeat_warning = 2, std::size_t repeat_stop = 3, std::size_t pingpong_stop = 2)
        : repeat_warning_threshold_(repeat_warning),
          repeat_stop_threshold_(repeat_stop),
          pingpong_stop_threshold_(pingpong_stop) {}

    void reset();

    LoopCheckResult record_and_check(const std::string& tool_name, const nlohmann::json& args);

    [[nodiscard]] const std::vector<ToolCallRecord>& get_history() const noexcept { return history_; }
};

} // namespace agent
