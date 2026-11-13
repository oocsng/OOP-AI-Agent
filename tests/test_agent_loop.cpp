#include "agent/agent_loop.h"
#include "client/mock_llm_client.h"
#include "tools/calculator_tool.h"
#include "tools/custom_tool3.h"
#include "common/cpp_features.h"
#include "test_framework.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>

using namespace agent;

class TestableAgentLoop : public AgentLoop {
public:
    using AgentLoop::AgentLoop;
    using AgentLoop::parse_llm_response;
};

class SequenceMockLLMClient : public LLMClient {
private:
    std::vector<std::string> canned_responses_;
    size_t index_{0};

public:
    explicit SequenceMockLLMClient(std::vector<std::string> responses)
        : canned_responses_(std::move(responses)) {}

    LLMResponse chat(const std::vector<Message>& /*history*/, const LLMConfig& /*config*/) override {
        if (index_ >= canned_responses_.size()) {
            return LLMResponse{.content = "Final Answer: Default Mock Answer", .raw_json = "", .tokens_used = 10};
        }
        std::string text = canned_responses_[index_++];
        return LLMResponse{.content = text, .raw_json = "", .tokens_used = 15};
    }
};

void test_react_parsing() {
    auto registry = std::make_shared<ToolRegistry>();
    TestableAgentLoop loop(nullptr, registry);

    // Test tool call format
    std::string tool_resp = "Thought: I should evaluate math expression.\nAction: calculator\nAction Input: {\"expression\": \"2 + 2\"}";
    ParsedAction action1 = loop.parse_llm_response(tool_resp);
    TEST_ASSERT(!action1.is_final_answer, "Action 1 should not be final answer");
    TEST_ASSERT(action1.thought.find("evaluate math") != std::string::npos, "Thought content mismatch");
    TEST_ASSERT(action1.tool_name == "calculator", "Tool name should be calculator");
    TEST_ASSERT(action1.tool_args["expression"] == "2 + 2", "Tool args mismatch");

    // Test final answer format
    std::string final_resp = "Thought: Got the result.\nFinal Answer: 4";
    ParsedAction action2 = loop.parse_llm_response(final_resp);
    TEST_ASSERT(action2.is_final_answer, "Action 2 should be final answer");
    TEST_ASSERT(action2.final_answer == "4", "Final answer mismatch");
}

void test_agent_loop_execution_and_step_hook() {
    auto registry = std::make_shared<ToolRegistry>();
    registry->register_tool(std::make_shared<CalculatorTool>());
    registry->register_tool(std::make_shared<RegexExtractorTool>());

    std::vector<std::string> mock_turns = {
        "Thought: Need calculation.\nAction: calculator\nAction Input: {\"expression\": \"10 + 20\"}",
        "Thought: Calculated result.\nFinal Answer: 30"
    };

    auto mock_client = std::make_shared<SequenceMockLLMClient>(mock_turns);
    AgentLoop loop(mock_client, registry, nullptr, /*max_steps=*/5);

    std::vector<Step> recorded_steps;
    loop.set_step_hook([&recorded_steps](const Step& step) {
        recorded_steps.push_back(step);
    });

    LLMConfig config;
    Trajectory traj = loop.run("task_test_01", "Calculate 10 + 20", config);

    TEST_ASSERT(traj.success, "Trajectory should be marked as success");
    TEST_ASSERT(traj.task_id == "task_test_01", "Task ID mismatch");
    TEST_ASSERT(recorded_steps.size() == 2, "Should have 2 recorded steps");

    // Step 1 check
    TEST_ASSERT(recorded_steps[0].step_id == 1, "Step 1 ID mismatch");
    TEST_ASSERT(recorded_steps[0].action.dump().find("calculator") != std::string::npos, "Step 1 action mismatch");
    TEST_ASSERT(recorded_steps[0].tool_result.find("30") != std::string::npos, "Step 1 tool result mismatch");

    // Step 2 check
    TEST_ASSERT(recorded_steps[1].step_id == 2, "Step 2 ID mismatch");
    TEST_ASSERT(recorded_steps[1].action.dump().find("\"30\"") != std::string::npos, "Step 2 action mismatch");
    TEST_ASSERT(recorded_steps[1].tool_result == "30", "Step 2 tool result mismatch");
    TEST_ASSERT(traj.total_tokens == 30, "Total tokens mismatch");
}

void test_cpp26_saturation_arithmetic() {
    int max_int = std::numeric_limits<int>::max();
    int min_int = std::numeric_limits<int>::min();

    // Saturated addition on signed int
    int sat_add = agent::cpp26::safe_add(max_int - 10, 20);
    TEST_ASSERT(sat_add == max_int, "safe_add should saturate at int max");

    int normal_add = agent::cpp26::safe_add(15, 25);
    TEST_ASSERT(normal_add == 40, "safe_add normal addition mismatch");

    // Saturated subtraction on signed int
    int sat_sub = agent::cpp26::safe_sub(min_int + 10, 20);
    TEST_ASSERT(sat_sub == min_int, "safe_sub should saturate at int min");

    int normal_sub = agent::cpp26::safe_sub(50, 20);
    TEST_ASSERT(normal_sub == 30, "safe_sub normal subtraction mismatch");

    // Mixed type addition (size_t + int) matching agent_loop usage
    std::size_t base_tokens = 100;
    int added_tokens = 50;
    std::size_t mixed_add = agent::cpp26::safe_add(base_tokens, added_tokens);
    TEST_ASSERT(mixed_add == 150, "safe_add mixed-type addition mismatch");

    // Saturated addition on unsigned size_t
    std::size_t max_size = std::numeric_limits<std::size_t>::max();
    std::size_t sat_size_add = agent::cpp26::safe_add(max_size - 5, std::size_t{10});
    TEST_ASSERT(sat_size_add == max_size, "safe_add should saturate at size_t max");

    // Narrow unsigned target with wide signed operand
    std::uint8_t narrow_u_sub_neg = agent::cpp26::safe_sub(std::uint8_t{10}, -300);
    TEST_ASSERT(narrow_u_sub_neg == 255, "uint8_t(10) - (-300) should saturate at 255");

    std::uint8_t narrow_u_sub_pos = agent::cpp26::safe_sub(std::uint8_t{200}, 300);
    TEST_ASSERT(narrow_u_sub_pos == 0, "uint8_t(200) - 300 should saturate at 0");

    // Narrow signed target with wide unsigned operand where result fits in signed range
    std::int8_t narrow_s_sub_u = agent::cpp26::safe_sub(std::int8_t{100}, std::uint32_t{150});
    TEST_ASSERT(narrow_s_sub_u == -50, "int8_t(100) - uint32_t(150) should equal -50");

    std::int8_t narrow_s_sub_u_sat = agent::cpp26::safe_sub(std::int8_t{100}, std::uint32_t{300});
    TEST_ASSERT(narrow_s_sub_u_sat == -128, "int8_t(100) - uint32_t(300) should saturate at -128");

    std::int8_t narrow_s_add_u = agent::cpp26::safe_add(std::int8_t{-100}, std::uint32_t{150});
    TEST_ASSERT(narrow_s_add_u == 50, "int8_t(-100) + uint32_t(150) should equal 50");

    std::int8_t narrow_s_add_u_sat = agent::cpp26::safe_add(std::int8_t{-100}, std::uint32_t{300});
    TEST_ASSERT(narrow_s_add_u_sat == 127, "int8_t(-100) + uint32_t(300) should saturate at 127");
}

class HistoryRecordingClient : public LLMClient {
public:
    std::vector<std::vector<Message>> recorded_histories;
    std::vector<std::string> canned_responses;
    size_t index{0};

    explicit HistoryRecordingClient(std::vector<std::string> responses = {})
        : canned_responses(std::move(responses)) {}

    LLMResponse chat(const std::vector<Message>& history, const LLMConfig& /*config*/) override {
        recorded_histories.push_back(history);
        if (index < canned_responses.size()) {
            return LLMResponse{.content = canned_responses[index++], .raw_json = "", .tokens_used = 10};
        }
        return LLMResponse{.content = "Thought: Done.\nFinal Answer: ok", .raw_json = "", .tokens_used = 10};
    }
};

class LongOutputTool : public Tool {
public:
    std::string name() const override { return "long_tool"; }
    std::string description() const override { return "Returns >3000 chars for truncation testing."; }
    nlohmann::json parameters_schema() const override { return nlohmann::json::object(); }
    std::string execute(const nlohmann::json& /*args*/) override {
        return std::string(3500, 'A');
    }
};

void test_history_isolation_and_interactive_mode() {
    auto spy = std::make_shared<HistoryRecordingClient>();
    auto registry = std::make_shared<ToolRegistry>();
    auto skill_loader = std::make_shared<SkillLoader>();
    if (std::filesystem::exists("skills")) {
        skill_loader->load_skills("skills");
    } else if (std::filesystem::exists("../skills")) {
        skill_loader->load_skills("../skills");
    }

    AgentLoop loop(spy, registry, skill_loader, /*max_steps=*/5);
    LLMConfig config;

    loop.run("task_1", "Fix this cpp code bug", config);
    loop.run("task_2", "Create a step plan for subtask", config);

    TEST_ASSERT(spy->recorded_histories.size() == 2, "Should record 2 runs");
    TEST_ASSERT(spy->recorded_histories[1].size() == 2, "Non-interactive run should reset history to system + user");
    TEST_ASSERT(spy->recorded_histories[1][0].content.find("Task Planner") != std::string::npos,
                "Task 2 system prompt should inject Task Planner skill");
    TEST_ASSERT(spy->recorded_histories[1][0].content.find("Code Fixer") == std::string::npos,
                "Task 2 system prompt should not retain Task 1 Code Fixer skill");

    loop.set_interactive_cli(true);
    loop.run("task_3", "Follow up question", config);
    TEST_ASSERT(spy->recorded_histories[2].size() > 2, "Interactive CLI mode should retain previous history");

    loop.clear_history();
    loop.run("task_4", "Fresh interactive turn", config);
    TEST_ASSERT(spy->recorded_histories[3].size() == 2, "clear_history() should reset interactive history");
}

void test_loop_warning_and_critical_stop_with_long_output() {
    auto registry = std::make_shared<ToolRegistry>();
    registry->register_tool(std::make_shared<LongOutputTool>());

    std::string repeat_call = "Thought: call long_tool.\nAction: long_tool\nAction Input: {\"k\": 1}";
    auto spy = std::make_shared<HistoryRecordingClient>(
        std::vector<std::string>{repeat_call, repeat_call, repeat_call});

    AgentLoop loop(spy, registry, nullptr, /*max_steps=*/5);
    LLMConfig config;
    Trajectory traj = loop.run("task_warn", "Run long tool", config);

    TEST_ASSERT(traj.steps.size() == 3, "Should execute 2 steps and stop on step 3");
    TEST_ASSERT(traj.steps[1].tool_result.size() == 3500, "Step 2 tool_result should preserve raw tool output");
    TEST_ASSERT(spy->recorded_histories.size() == 3, "Should invoke chat 3 times");
    TEST_ASSERT(spy->recorded_histories[2].back().content.find("[Output truncated to 3000 characters]") != std::string::npos,
                "Long output should still be truncated");
    TEST_ASSERT(spy->recorded_histories[2].back().content.find("[LOOP DETECTOR WARNING]") != std::string::npos,
                "Loop warning must be appended after truncation");
    TEST_ASSERT(traj.steps[2].tool_result.find("[LOOP DETECTOR STOP]") != std::string::npos,
                "Step 3 should stop with LOOP DETECTOR STOP");
}

int main() {
    TestSuite suite("AGENT LOOP MODULE");
    suite.run("ReAct Parsing", test_react_parsing);
    suite.run("Agent Loop Execution", test_agent_loop_execution_and_step_hook);
    suite.run("C++26 Saturation Arithmetic", test_cpp26_saturation_arithmetic);
    suite.run("History Isolation & Interactive Mode", test_history_isolation_and_interactive_mode);
    suite.run("Loop Warning & Critical Stop With Long Output", test_loop_warning_and_critical_stop_with_long_output);
    return suite.print_summary();
}
