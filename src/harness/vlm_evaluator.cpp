#include "vlm_evaluator.h"
#include "client/ollama_client.h"
#include "dto/message.h"
#include <filesystem>

namespace agent::harness {

EvalResult VLMEvaluator::evaluate(const Trajectory& trajectory, const TaskSpec& spec) {
    (void)spec; // spec unused in this evaluator; keyword/functional checks use it
    EvalResult result;

    // Find latest screenshot path in trajectory tool results
    std::string screenshot_path;
    for (const auto& step : trajectory.steps) {
        size_t p = step.tool_result.find("saved to '");
        if (p != std::string::npos) {
            size_t end = step.tool_result.find("'", p + 10);
            if (end != std::string::npos) {
                screenshot_path = step.tool_result.substr(p + 10, end - (p + 10));
            }
        }
    }

    // If no VLM client or no screenshot, fall back to trajectory success flag
    if (!client_) {
        bool has_shot = !screenshot_path.empty() && std::filesystem::exists(screenshot_path);
        result.passed = has_shot || trajectory.success;
        result.score = result.passed ? 1.0 : 0.0;
        result.feedback = result.passed
            ? "VLM check passed (no client; screenshot artifact detected or task succeeded)."
            : "VLM check failed: no screenshot and no VLM client configured.";
        return result;
    }

    if (screenshot_path.empty() || !std::filesystem::exists(screenshot_path)) {
        result.passed = false;
        result.score = 0.0;
        result.feedback = "VLM evaluation: screenshot file not found for visual verification.";
        return result;
    }

    try {
        std::string b64 = agent::OllamaClient::encode_file_base64(screenshot_path);
        agent::Message vision_msg;
        vision_msg.role = "user";
        vision_msg.content = "Does this screenshot show a completed browser search result page? Answer YES or NO.";
        vision_msg.images = {b64};

        agent::LLMConfig cfg;
        agent::LLMResponse resp = client_->chat({vision_msg}, cfg);
        bool vlm_yes = resp.content.find("YES") != std::string::npos ||
                       resp.content.find("yes") != std::string::npos;
        result.passed = vlm_yes;
        result.score = vlm_yes ? 1.0 : 0.0;
        result.feedback = "VLM response: " + resp.content;
    } catch (const std::exception& e) {
        result.passed = false;
        result.score = 0.0;
        result.feedback = std::string("VLM evaluation error: ") + e.what();
    }

    return result;
}

} // namespace agent::harness
