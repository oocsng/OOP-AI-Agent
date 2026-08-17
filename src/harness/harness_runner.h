#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include "evaluator.h"
#include "environment.h"
#include "agent/agent_loop.h"

namespace agent::harness {

struct TaskRunSummary {
    TaskSpec spec;
    Trajectory trajectory;
    EvalResult keyword_eval;
    EvalResult functional_eval;
};

class HarnessRunner {
private:
    std::shared_ptr<AgentLoop> agent_loop_;
    std::unique_ptr<Evaluator> keyword_evaluator_;
    std::unique_ptr<Evaluator> functional_evaluator_;
    std::unique_ptr<Environment> environment_;
    std::filesystem::path output_dir_{"trajectories"};

public:
    explicit HarnessRunner(std::shared_ptr<AgentLoop> agent_loop,
                           std::filesystem::path output_dir = "trajectories");

    void set_evaluators(std::unique_ptr<Evaluator> keyword_eval,
                        std::unique_ptr<Evaluator> functional_eval);

    void set_environment(std::unique_ptr<Environment> env);

    TaskRunSummary run_task(const TaskSpec& spec, const LLMConfig& config);

    void save_trajectory_json(const Trajectory& trajectory, const TaskSpec& spec, const EvalResult& keyword_eval, const EvalResult& functional_eval) const;
};

} // namespace agent::harness
