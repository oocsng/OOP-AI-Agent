#pragma once

#include "evaluator.h"
#include "client/llm_client.h"
#include <memory>
#include <string>

namespace agent::harness {

// VLMEvaluator: uses a VLM client to verify screenshot/visual task outputs (Strategy Pattern)
class VLMEvaluator : public Evaluator {
public:
    explicit VLMEvaluator(std::shared_ptr<LLMClient> client = nullptr)
        : client_(std::move(client)) {}

    EvalResult evaluate(const Trajectory& trajectory, const TaskSpec& spec) override;

private:
    std::shared_ptr<LLMClient> client_;
};

} // namespace agent::harness
