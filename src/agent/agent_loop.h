#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

#include "dto/trajectory.h"
#include "client/llm_client.h"
#include "tools/tool_registry.h"
#include "skill_loader.h"
#include "loop_detector.h"

namespace agent {

struct ParsedAction {
    std::string thought;
    bool is_final_answer{false};
    std::string final_answer;
    std::string tool_name;
    nlohmann::json tool_args;
};

using StepHook = std::function<void(const Step&)>;

class AgentLoop {
protected:
    std::shared_ptr<LLMClient> client_;
    std::shared_ptr<ToolRegistry> tool_registry_;
    std::shared_ptr<SkillLoader> skill_loader_;
    LoopDetector loop_detector_;
    StepHook step_hook_;
    std::size_t max_steps_{10};
    std::vector<Message> history_;
    bool is_interactive_cli_{false};

public:
    AgentLoop(std::shared_ptr<LLMClient> client,
              std::shared_ptr<ToolRegistry> tool_registry,
              std::shared_ptr<SkillLoader> skill_loader = nullptr,
              std::size_t max_steps = 10)
        : client_(std::move(client)),
          tool_registry_(std::move(tool_registry)),
          skill_loader_(std::move(skill_loader)),
          max_steps_(max_steps) {}

    virtual ~AgentLoop() = default;

    void set_step_hook(StepHook hook) { step_hook_ = std::move(hook); }
    void set_max_steps(std::size_t max_steps) { max_steps_ = max_steps; }
    void set_interactive_cli(bool v) { is_interactive_cli_ = v; }
    void clear_history();

    // === TEMPLATE METHOD ===
    // Fixed skeleton of the ReAct algorithm loop
    Trajectory run(const std::string& task_id, const std::string& user_prompt, const LLMConfig& config);

protected:
    // Primitive virtual methods customizable by subclasses (Template Method Hooks)
    virtual std::string build_system_prompt(const std::string& user_prompt);
    virtual ParsedAction parse_llm_response(const std::string& response_text);
    virtual bool is_goal_achieved(const ParsedAction& action);

    // Helpers
    virtual LLMResponse think(const std::vector<Message>& history, const LLMConfig& config);
    virtual std::string act(const std::string& tool_name, const nlohmann::json& tool_args);
};

} // namespace agent
