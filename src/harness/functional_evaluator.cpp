#include "functional_evaluator.h"
#include <cstdio>
#include <array>
#include <memory>

namespace agent::harness {

EvalResult FunctionalEvaluator::evaluate(const Trajectory& trajectory, const TaskSpec& spec) {
    EvalResult result;
    if (spec.test_command.empty()) {
        result.passed = trajectory.success;
        result.score = trajectory.success ? 1.0 : 0.0;
        result.feedback = "No functional test command specified.";
        return result;
    }

    std::string command = spec.test_command;
    std::string output;
    std::array<char, 256> buffer;

#if defined(_WIN32)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        result.passed = false;
        result.score = 0.0;
        result.feedback = "Failed to execute functional test command: " + command;
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        output += buffer.data();
    }

#if defined(_WIN32)
    int exit_code = _pclose(pipe.release());
#else
    int exit_code = pclose(pipe.release());
#endif

    if (exit_code == 0) {
        result.passed = true;
        result.score = 1.0;
        result.feedback = "Functional test passed cleanly. Output: " + output;
    } else {
        result.passed = false;
        result.score = 0.0;
        result.feedback = "Functional test failed with exit code " + std::to_string(exit_code) + ". Output: " + output;
    }

    return result;
}

} // namespace agent::harness
