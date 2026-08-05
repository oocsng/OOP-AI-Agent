#pragma once

#include "evaluator.h"

namespace agent::harness {

class FunctionalEvaluator : public Evaluator {
public:
    EvalResult evaluate(const Trajectory& trajectory, const TaskSpec& spec) override;
};

} // namespace agent::harness
