#include "keyword_evaluator.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace agent::harness {

static std::string normalize(std::string_view str) {
    std::string result;
    result.reserve(str.length());
    for (char c : str) {
        if (c == '-') {
            result.push_back('_');
        } else {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return result;
}

EvalResult KeywordEvaluator::evaluate(const Trajectory& trajectory, const TaskSpec& spec) {
    EvalResult result;
    if (spec.expected_keywords.empty()) {
        result.passed = trajectory.success;
        result.score = trajectory.success ? 1.0 : 0.0;
        result.feedback = "No keywords specified for task.";
        return result;
    }

    // Concatenate all step thoughts and tool results
    std::stringstream full_text_ss;
    for (const auto& step : trajectory.steps) {
        full_text_ss << step.thought << " " << step.action.dump() << " " << step.tool_result << " ";
    }
    std::string full_text = normalize(full_text_ss.str());

    std::size_t matched_count = 0;
    std::vector<std::string> missing_keywords;

    for (const auto& kw : spec.expected_keywords) {
        std::string lower_kw = normalize(kw);
        // C++23 std::string::contains
        if (full_text.contains(lower_kw)) {
            matched_count++;
        } else {
            missing_keywords.push_back(kw);
        }
    }

    result.score = static_cast<double>(matched_count) / static_cast<double>(spec.expected_keywords.size());
    result.passed = (matched_count == spec.expected_keywords.size());

    if (result.passed) {
        result.feedback = "All expected keywords matched successfully (" + std::to_string(matched_count) + "/" +
                          std::to_string(spec.expected_keywords.size()) + ").";
    } else {
        result.feedback = "Missing keywords: ";
        for (std::size_t i = 0; i < missing_keywords.size(); ++i) {
            result.feedback += missing_keywords[i] + (i + 1 < missing_keywords.size() ? ", " : "");
        }
    }

    return result;
}

} // namespace agent::harness
