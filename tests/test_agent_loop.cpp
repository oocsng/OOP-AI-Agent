#include "agent/agent_loop.h"
#include "tools/calculator_tool.h"
#include "tools/custom_tool3.h"
#include "test_framework.h"
#include <iostream>

using namespace agent;

class TestableAgentLoop : public AgentLoop {
public:
    using AgentLoop::AgentLoop;
    using AgentLoop::parse_llm_response;
};

class MockLLMClient : public LLMClient {
private:
    std::vector<std::string> canned_responses_;
    size_t index_{0};

public:
    explicit MockLLMClient(std::vector<std::string> responses)
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

    auto mock_client = std::make_shared<MockLLMClient>(mock_turns);
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
}

int main() {
    TestSuite suite("AGENT LOOP MODULE");
    suite.run("ReAct Parsing", test_react_parsing);
    suite.run("Agent Loop Execution", test_agent_loop_execution_and_step_hook);
    return suite.print_summary();
}
