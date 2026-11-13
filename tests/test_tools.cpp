#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include "tools/memory_tool.h"
#include "tools/web_tool.h"
#include "tools/custom_tool1.h"
#include "tools/custom_tool2.h"
#include "tools/custom_tool3.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
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

std::vector<TestResult> g_results;
bool g_all_passed = true;

template <typename Func>
void run_test(const std::string& name, Func f) {
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
    
    g_results.push_back({name, passed, latency_ms, error_msg});
    if (!passed) {
        g_all_passed = false;
    }
}

void test_tool_registry_policy() {
    ToolRegistry registry;
    auto calc = std::make_shared<CalculatorTool>();
    auto writer = std::make_shared<WriteFileTool>();
    auto reader = std::make_shared<ReadFileTool>();

    registry.register_tool(calc);
    registry.register_tool(writer);
    registry.register_tool(reader);

    TEST_ASSERT(registry.has_tool("calculator"), "Should have calculator");
    TEST_ASSERT(registry.has_tool("write_file"), "Should have write_file");
    TEST_ASSERT(registry.has_tool("read_file"), "Should have read_file");
    TEST_ASSERT(!registry.has_tool("nonexistent_tool"), "Should not have nonexistent_tool");

    registry.set_allow_list({"calculator", "read_file"});
    TEST_ASSERT(registry.is_tool_allowed("calculator"), "calculator should be allowed");
    TEST_ASSERT(registry.is_tool_allowed("read_file"), "read_file should be allowed");
    TEST_ASSERT(!registry.is_tool_allowed("write_file"), "write_file should not be allowed");

    registry.set_deny_list({"read_file"});
    TEST_ASSERT(registry.is_tool_allowed("calculator"), "calculator should be allowed");
    TEST_ASSERT(!registry.is_tool_allowed("read_file"), "read_file should be denied");
    TEST_ASSERT(!registry.is_tool_allowed("write_file"), "write_file should be denied");

    registry.clear_policies();
    TEST_ASSERT(registry.is_tool_allowed("calculator"), "calculator should be allowed after clear");
    TEST_ASSERT(registry.is_tool_allowed("write_file"), "write_file should be allowed after clear");
    TEST_ASSERT(registry.is_tool_allowed("read_file"), "read_file should be allowed after clear");

    std::string schema = registry.get_tools_schema_json();
    TEST_ASSERT(schema.find("calculator") != std::string::npos, "Schema should contain calculator");
    TEST_ASSERT(schema.find("write_file") != std::string::npos, "Schema should contain write_file");
}

void test_calculator_tool() {
    CalculatorTool calc;
    std::string calc_res = calc.execute({{"expression", "(3 + 4) * 2^3"}});
    TEST_ASSERT(calc_res.find("56") != std::string::npos, "Calculation result should be 56");

    std::string tower_res = calc.execute({{"expression", "2^3^2"}});
    TEST_ASSERT(tower_res == "512", "2^3^2 should be right-associative (2^(3^2) = 512)");

    std::string high_bit_res = calc.execute({{"expression", "\xC2\xA0 2 + 2"}});
    TEST_ASSERT(high_bit_res.find("Error evaluating expression") != std::string::npos,
                "High-bit non-ASCII byte should return clean error without signed-char UB");
}

void test_file_tool() {
    WriteFileTool write_tool;
    ReadFileTool read_tool;
    std::string test_file = "test_core_tool_io.tmp";
    std::string write_res = write_tool.execute({{"path", test_file}, {"content", "Hello Core Tools"}});
    TEST_ASSERT(write_res.find("successfully") != std::string::npos || write_res.find("Wrote") != std::string::npos || write_res.find("written") != std::string::npos || write_res.find("Error") == std::string::npos, "Write should succeed");
    
    std::string read_res = read_tool.execute({{"path", test_file}});
    TEST_ASSERT(read_res.find("Hello Core Tools") != std::string::npos, "Read should return written content");
    std::filesystem::remove(test_file);
}

void test_exec_tool() {
    ExecTool exec_tool;
#ifdef _WIN32
    std::string exec_res = exec_tool.execute({{"command", "cmd /c echo HelloExec"}});
#else
    std::string exec_res = exec_tool.execute({{"command", "echo HelloExec"}});
#endif
    TEST_ASSERT(exec_res.find("HelloExec") != std::string::npos, "Exec should output HelloExec");
}

void test_memory_tool() {
    std::string db_file = "test_memory_core.db";
    if (std::filesystem::exists(db_file)) {
        std::filesystem::remove(db_file);
    }
    MemorySaveTool mem_save(db_file);
    MemorySearchTool mem_search(db_file);

    std::string save_res = mem_save.execute({{"key", "project_name"}, {"content", "OOP AI Agent 2026"}});
    TEST_ASSERT(save_res.find("Saved") != std::string::npos || save_res.find("Successfully") != std::string::npos || save_res.find("successfully") != std::string::npos, "Memory save should succeed");

    std::string search_res = mem_search.execute({{"query", "project_name"}});
    TEST_ASSERT(search_res.find("OOP AI Agent 2026") != std::string::npos, "Memory search should find content");

    if (std::filesystem::exists(db_file)) {
        std::filesystem::remove(db_file);
    }
}

void test_web_search_tool() {
    WebSearchTool web_tool;
    nlohmann::json schema = web_tool.parameters_schema();
    TEST_ASSERT(schema.contains("properties"), "Web search schema should contain properties");
    TEST_ASSERT(web_tool.name() == "web_search", "Web search tool name should be web_search");
}

void test_custom_tools() {
    DateTimeTool time_tool;
    std::string time_res = time_tool.execute({});
    TEST_ASSERT(!time_res.empty(), "DateTime should not be empty");
    TEST_ASSERT(time_res.find("Current System Time") != std::string::npos || time_res.find("20") != std::string::npos, "DateTime output format check");

    SystemInfoTool sys_tool;
    std::string sys_res = sys_tool.execute({});
    TEST_ASSERT(!sys_res.empty(), "SystemInfo should not be empty");
    TEST_ASSERT(sys_res.find("os") != std::string::npos || sys_res.find("Windows") != std::string::npos || sys_res.find("Linux") != std::string::npos, "SystemInfo should contain OS info");

    RegexExtractorTool regex_tool;
    std::string regex_res = regex_tool.execute({{"text", "Order #12345 confirmed for item #9999."}, {"pattern", "#[0-9]+"}});
    TEST_ASSERT(regex_res.find("12345") != std::string::npos, "Regex should extract 12345");
    TEST_ASSERT(regex_res.find("9999") != std::string::npos, "Regex should extract 9999");
}

int main() {
    run_test("ToolRegistry Policy Check", test_tool_registry_policy);
    run_test("CalculatorTool", test_calculator_tool);
    run_test("FileTool (Read/Write)", test_file_tool);
    run_test("ExecTool", test_exec_tool);
    run_test("WebSearchTool", test_web_search_tool);
    run_test("MemoryTool (SQLite)", test_memory_tool);
    run_test("CustomTools", test_custom_tools);

    std::cout << "==================================================\n";
    std::cout << "            UNIT TEST RESULTS: TOOLS              \n";
    std::cout << "==================================================\n";

    int passed_count = 0;
    for (const auto& res : g_results) {
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
    double success_rate = g_results.empty() ? 0.0 : (100.0 * passed_count / g_results.size());
    std::cout << "Summary: " << passed_count << "/" << g_results.size() 
              << " Tools PASSED (" << std::fixed << std::setprecision(0) << success_rate << "% Success Rate)\n";
    std::cout << "==================================================\n";

    return g_all_passed ? 0 : 1;
}
