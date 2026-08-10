#include "harness/keyword_evaluator.h"
#include "harness/functional_evaluator.h"
#include "test_framework.h"
#include <iostream>

using namespace agent::harness;
using agent::Trajectory;
using agent::Step;

void test_keyword_evaluator() {
    KeywordEvaluator eval;
    TaskSpec spec;
    spec.task_id = "task_kw_01";
    spec.expected_keywords = {"apple", "banana", "cherry"};

    Trajectory traj;
    traj.success = true;
    traj.steps.push_back(Step{.thought = "Found an apple", .action = "none", .tool_result = "Got banana"});

    // Partial match (2 out of 3)
    EvalResult res1 = eval.evaluate(traj, spec);
    TEST_ASSERT(!res1.passed, "Partial match should not pass");
    TEST_ASSERT(res1.score > 0.6 && res1.score < 0.7, "Partial match score out of bounds");
    TEST_ASSERT(res1.feedback.find("Missing keywords: cherry") != std::string::npos, "Missing keyword feedback incorrect");

    // Full match
    traj.steps.push_back(Step{.thought = "Also bought cherry", .action = "none", .tool_result = "Done"});
    EvalResult res2 = eval.evaluate(traj, spec);
    TEST_ASSERT(res2.passed, "Full match should pass");
    TEST_ASSERT(res2.score == 1.0, "Full match score should be 1.0");
    TEST_ASSERT(res2.feedback.find("All expected keywords matched") != std::string::npos, "Full match feedback incorrect");
}

void test_functional_evaluator() {
    FunctionalEvaluator eval;
    TaskSpec spec_pass;
    spec_pass.task_id = "task_fn_01";
#ifdef _WIN32
    spec_pass.test_command = "cmd /c exit 0";
#else
    spec_pass.test_command = "exit 0";
#endif

    Trajectory traj;
    traj.success = true;

    EvalResult res_pass = eval.evaluate(traj, spec_pass);
    TEST_ASSERT(res_pass.passed, "Functional eval pass should succeed");
    TEST_ASSERT(res_pass.score == 1.0, "Functional eval pass score should be 1.0");
    TEST_ASSERT(res_pass.feedback.find("passed cleanly") != std::string::npos, "Functional eval pass feedback incorrect");

    TaskSpec spec_fail;
    spec_fail.task_id = "task_fn_02";
#ifdef _WIN32
    spec_fail.test_command = "cmd /c exit 1";
#else
    spec_fail.test_command = "exit 1";
#endif

    EvalResult res_fail = eval.evaluate(traj, spec_fail);
    TEST_ASSERT(!res_fail.passed, "Functional eval fail should fail");
    TEST_ASSERT(res_fail.score == 0.0, "Functional eval fail score should be 0.0");
    TEST_ASSERT(res_fail.feedback.find("failed with exit code") != std::string::npos, "Functional eval fail feedback incorrect");
}

int main() {
    TestSuite suite("EVALUATORS MODULE");
    suite.run("Keyword Evaluator", test_keyword_evaluator);
    suite.run("Functional Evaluator", test_functional_evaluator);
    return suite.print_summary();
}
