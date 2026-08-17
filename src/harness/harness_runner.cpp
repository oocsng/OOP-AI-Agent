#include "harness_runner.h"
#include "keyword_evaluator.h"
#include "functional_evaluator.h"
#include <fstream>
#include <iostream>
#include <print>
#include <nlohmann/json.hpp>
#include "utils/time_utils.hpp"

namespace agent::harness {

HarnessRunner::HarnessRunner(std::shared_ptr<AgentLoop> agent_loop, std::filesystem::path output_dir)
    : agent_loop_(std::move(agent_loop)), output_dir_(std::move(output_dir)) {
    keyword_evaluator_ = std::make_unique<KeywordEvaluator>();
    functional_evaluator_ = std::make_unique<FunctionalEvaluator>();
}

void HarnessRunner::set_evaluators(std::unique_ptr<Evaluator> keyword_eval,
                                    std::unique_ptr<Evaluator> functional_eval) {
    if (keyword_eval) keyword_evaluator_ = std::move(keyword_eval);
    if (functional_eval) functional_evaluator_ = std::move(functional_eval);
}

void HarnessRunner::set_environment(std::unique_ptr<Environment> env) {
    environment_ = std::move(env);
}

TaskRunSummary HarnessRunner::run_task(const TaskSpec& spec, const LLMConfig& config) {
    TaskRunSummary summary;
    summary.spec = spec;

    if (!agent_loop_) {
        summary.trajectory.task_id = spec.task_id;
        summary.trajectory.success = false;
        return summary;
    }

    // Environment setup (Observer Pattern: harness injects hook)
    if (environment_) environment_->setup();

    // Attach Observer step hook
    agent_loop_->set_step_hook([&spec](const Step& step) {
        // Real-time Observer log output
        std::println("[{}] Step {} - Action: {} | Tokens: {} | Latency: {:.2f}ms", 
                     agent::utils::get_local_timestamp(), step.step_id, step.action.dump(), step.tokens_used, step.latency_ms);
    });

    // Execute AgentLoop
    summary.trajectory = agent_loop_->run(spec.task_id, spec.prompt, config);

    // Run Evaluators (Strategy Pattern)
    if (keyword_evaluator_) {
        summary.keyword_eval = keyword_evaluator_->evaluate(summary.trajectory, spec);
    }
    if (functional_evaluator_) {
        summary.functional_eval = functional_evaluator_->evaluate(summary.trajectory, spec);
    }

    // Save JSON trajectory file
    save_trajectory_json(summary.trajectory, spec, summary.keyword_eval, summary.functional_eval);

    // Environment teardown
    if (environment_) environment_->teardown();

    return summary;
}

void HarnessRunner::save_trajectory_json(const Trajectory& trajectory, const TaskSpec& spec, const EvalResult& keyword_eval, const EvalResult& functional_eval) const {
    if (!std::filesystem::exists(output_dir_)) {
        std::filesystem::create_directories(output_dir_);
    }

    std::filesystem::path file_path = output_dir_ / ("trajectory_" + spec.task_id + ".json");
    std::ofstream file(file_path);
    if (!file.is_open()) return;

    nlohmann::json root;
    root["task_id"] = spec.task_id;
    root["prompt"] = spec.prompt;
    root["model"] = trajectory.model;
    root["success"] = trajectory.success;
    root["timestamp"] = trajectory.timestamp;
    root["total_tokens"] = trajectory.total_tokens;
    root["total_time_ms"] = trajectory.total_time_ms;
    root["execution_duration_ms"] = trajectory.execution_duration_ms;
    root["evaluation"] = {
        {"keyword", {
            {"passed", keyword_eval.passed},
            {"score", keyword_eval.score},
            {"feedback", keyword_eval.feedback}
        }},
        {"functional", {
            {"passed", functional_eval.passed},
            {"score", functional_eval.score},
            {"feedback", functional_eval.feedback}
        }}
    };

    nlohmann::json steps_json = nlohmann::json::array();
    for (const auto& s : trajectory.steps) {
        nlohmann::json step_obj = s;
        steps_json.push_back(step_obj);
    }
    root["steps"] = steps_json;

    file << root.dump(2);
}

} // namespace agent::harness
