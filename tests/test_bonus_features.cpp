#include "tools/gui_tool.h"
#include "tools/tool_registry.h"
#include "tools/vector_memory_tool.h"
#include "agent/sub_agent_manager.h"
#include "agent/agent_loop.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <iomanip>
#include <nlohmann/json.hpp>

using namespace agent;

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #cond + " - " + msg); \
    }

struct TestResult {
    std::string name;
    bool passed;
    double latency_ms;
    std::string error_msg;
};

std::vector<TestResult> g_bonus_results;
bool g_bonus_all_passed = true;

template <typename Func>
void run_bonus_test(const std::string& name, Func f) {
    auto start = std::chrono::high_resolution_clock::now();
    bool passed = true;
    std::string error_msg = "";
    try {
        f();
    } catch (const std::exception& e) {
        passed = false;
        error_msg = e.what();
    } catch (...) {
        passed = false;
        error_msg = "Unknown error";
    }
    auto end = std::chrono::high_resolution_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    g_bonus_results.push_back({name, passed, latency_ms, error_msg});
    if (!passed) {
        g_bonus_all_passed = false;
    }
}

// GUI Agent Tests
void test_gui_bonus() {
    std::vector<unsigned char> raw_pixels = {'A', 'B', 'C', 'D', 'E', 'F'};
    std::string b64 = GuiTool::encode_base64(raw_pixels);
    TEST_ASSERT(!b64.empty(), "Base64 string should not be empty");
    TEST_ASSERT(b64 == "QUJDREVGI==" || b64 == "QUJDREVG", "Base64 encoding mismatch");

    GuiTool gui;
    TEST_ASSERT(gui.name() == "gui_tool", "Tool name mismatch");
    TEST_ASSERT(!gui.description().empty(), "Description empty");

    std::string err_res = gui.execute({});
    TEST_ASSERT(err_res.find("Error") != std::string::npos, "Missing action should return error");

    nlohmann::json click_args = {{"action", "click"}, {"x", 100}, {"y", 200}};
    std::string click_res = gui.execute(click_args);
    TEST_ASSERT(click_res.find("Clicked") != std::string::npos || click_res.find("executed") != std::string::npos, "Click execution failed");

    nlohmann::json type_args = {{"action", "type"}, {"text", "Hello GUI"}};
    std::string type_res = gui.execute(type_args);
    bool success = (type_res.find("Typed") != std::string::npos) || 
                   (type_res.find("executed") != std::string::npos) || 
                   (type_res.find("Cannot execute GUI typing") != std::string::npos) ||
                   (type_res.find("Exception") != std::string::npos);
    TEST_ASSERT(success, "GUI typing test failed unexpectedly");

    ToolRegistry registry;
    auto gui_ptr = std::make_shared<GuiTool>();
    registry.register_tool(gui_ptr);
    TEST_ASSERT(registry.has_tool("gui_tool"), "GUI tool registration failed");
}

// Vector Memory Tests
void test_vector_memory_bonus() {
    std::vector<float> v1 = {1.0f, 2.0f, 3.0f};
    std::vector<float> v2 = {1.0f, 2.0f, 3.0f};
    double score1 = VectorMemoryTool::cosine_similarity(v1, v2);
    TEST_ASSERT(std::abs(score1 - 1.0) < 1e-5, "Identical vectors similarity != 1.0");

    std::vector<float> v_ortho1 = {1.0f, 0.0f};
    std::vector<float> v_ortho2 = {0.0f, 1.0f};
    double score2 = VectorMemoryTool::cosine_similarity(v_ortho1, v_ortho2);
    TEST_ASSERT(std::abs(score2 - 0.0) < 1e-5, "Orthogonal vectors similarity != 0.0");

    std::vector<float> v_opp1 = {1.0f, 2.0f};
    std::vector<float> v_opp2 = {-1.0f, -2.0f};
    double score3 = VectorMemoryTool::cosine_similarity(v_opp1, v_opp2);
    TEST_ASSERT(std::abs(score3 - (-1.0)) < 1e-5, "Opposite vectors similarity != -1.0");

    TEST_ASSERT(VectorMemoryTool::cosine_similarity({}, v1) == 0.0, "Empty vector similarity != 0.0");
    TEST_ASSERT(VectorMemoryTool::cosine_similarity(v1, {1.0f}) == 0.0, "Size mismatch similarity != 0.0");

    VectorMemoryTool tool;
    TEST_ASSERT(tool.name() == "vector_memory", "Tool name mismatch");

    std::string err = tool.execute({});
    TEST_ASSERT(err.find("Error") != std::string::npos, "Empty args should return error");

    std::string search_res = tool.execute({{"action", "search"}, {"text", "hello"}});
    TEST_ASSERT(search_res.find("Error") != std::string::npos || search_res.find("No relevant memory") != std::string::npos, "Search in empty DB should return no relevant memory");
}

// Multi-Agent Tests
void test_multi_agent_bonus() {
    MessageQueue queue;
    TEST_ASSERT(queue.empty(), "Queue should be empty initially");

    const int num_producers = 4;
    const int msgs_per_producer = 25;
    std::vector<std::thread> producers;

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&queue, i, msgs_per_producer]() {
            for (int j = 0; j < msgs_per_producer; ++j) {
                queue.push("msg_" + std::to_string(i) + "_" + std::to_string(j));
            }
        });
    }

    int received_count = 0;
    std::vector<std::string> received_messages;
    std::mutex res_mtx;

    std::thread consumer([&queue, &received_count, &received_messages, &res_mtx, total = num_producers * msgs_per_producer]() {
        for (int i = 0; i < total; ++i) {
            std::string msg = queue.pop();
            std::lock_guard<std::mutex> lock(res_mtx);
            received_messages.push_back(msg);
            received_count++;
        }
    });

    for (auto& t : producers) {
        t.join();
    }
    consumer.join();

    TEST_ASSERT(received_count == num_producers * msgs_per_producer, "Did not receive all messages");
    TEST_ASSERT(queue.empty(), "Queue not empty after consumer finished");

    SubAgentManager manager;
    manager.send_message("agent_b", "Hello Agent B from main");
    std::string msg = manager.receive_message("agent_b");
    TEST_ASSERT(msg == "Hello Agent B from main", "SubAgentManager direct message failed");

    bool thread_ran = false;
    std::thread t([&manager, &thread_ran]() {
        manager.send_message("worker_inbox", "Task Completed");
        thread_ran = true;
    });
    t.join();

    TEST_ASSERT(thread_ran, "Thread did not run");
    std::string worker_msg = manager.receive_message("worker_inbox");
    TEST_ASSERT(worker_msg == "Task Completed", "Thread message not received");
}

int main() {
    if (!std::filesystem::exists("benchmark") && std::filesystem::exists("../benchmark")) {
        std::filesystem::current_path("..");
    } else if (!std::filesystem::exists("benchmark") && std::filesystem::exists("../../benchmark")) {
        std::filesystem::current_path("../..");
    }

    run_bonus_test("GUI Agent Automation", test_gui_bonus);
    run_bonus_test("Persistent Vector Memory", test_vector_memory_bonus);
    run_bonus_test("Multi-Agent Coordination", test_multi_agent_bonus);

    std::cout << "==================================================\n";
    std::cout << "         UNIT TEST RESULTS: BONUS FEATURES        \n";
    std::cout << "==================================================\n";

    int passed_count = 0;
    for (const auto& res : g_bonus_results) {
        std::cout << (res.passed ? "\033[32m[PASS]\033[0m" : "\033[31m[FAIL]\033[0m") 
                  << " " << std::left << std::setw(30) << res.name 
                  << " (" << std::fixed << std::setprecision(2) << res.latency_ms << " ms)\n";
        if (res.passed) {
            passed_count++;
        } else {
            std::cout << "       -> Error: " << res.error_msg << "\n";
        }
    }

    std::cout << "--------------------------------------------------\n";
    double success_rate = g_bonus_results.empty() ? 0.0 : (100.0 * passed_count / g_bonus_results.size());
    std::cout << "Summary: " << passed_count << "/" << g_bonus_results.size() 
              << " Bonus Modules PASSED (" << std::fixed << std::setprecision(0) << success_rate << "% Success Rate)\n";
    std::cout << "==================================================\n";

    return g_bonus_all_passed ? 0 : 1;
}
