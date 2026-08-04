#include "loop_detector.h"

namespace agent {

void LoopDetector::reset() {
    history_.clear();
}

LoopCheckResult LoopDetector::record_and_check(const std::string& tool_name, const nlohmann::json& args) {
    std::string args_str = args.dump();
    history_.push_back({tool_name, args_str});

    std::size_t n = history_.size();

    // 1. Generic Repeat Detection (consecutive identical tool calls)
    std::size_t consecutive_count = 0;
    for (std::size_t i = n; i > 0; --i) {
        if (history_[i - 1].tool_name == tool_name && history_[i - 1].args_dump == args_str) {
            consecutive_count++;
        } else {
            break;
        }
    }

    if (consecutive_count >= repeat_stop_threshold_) {
        return {
            LoopStatus::CriticalStop,
            "Critical Stop: Tool '" + tool_name + "' was invoked " + std::to_string(consecutive_count) +
            " consecutive times with identical parameters."
        };
    } else if (consecutive_count >= repeat_warning_threshold_) {
        return {
            LoopStatus::Warning,
            "Warning: Tool '" + tool_name + "' repeated " + std::to_string(consecutive_count) + " times."
        };
    }

    // 2. Ping-Pong Loop Detection (A -> B -> A -> B)
    if (n >= 4) {
        const auto& call1 = history_[n - 4];
        const auto& call2 = history_[n - 3];
        const auto& call3 = history_[n - 2];
        const auto& call4 = history_[n - 1];

        bool is_pingpong = (call1.tool_name == call3.tool_name && call1.args_dump == call3.args_dump) &&
                           (call2.tool_name == call4.tool_name && call2.args_dump == call4.args_dump) &&
                           (call1.tool_name != call2.tool_name || call1.args_dump != call2.args_dump);

        if (is_pingpong) {
            return {
                LoopStatus::CriticalStop,
                "Critical Stop: Ping-Pong loop detected between '" + call1.tool_name + "' and '" + call2.tool_name + "'."
            };
        }
    }

    return {LoopStatus::Normal, ""};
}

} // namespace agent
