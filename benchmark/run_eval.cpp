#include <iostream>
#include <fstream>
#include <print>
#include <vector>
#include <memory>
#include <filesystem>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <sqlite3.h>
#include "utils/time_utils.hpp"

#include "dto/trajectory.h"
#include "common/cpp_features.h"
#include "client/ollama_client.h"
#include "client/mock_llm_client.h"
#include "tools/tool_registry.h"
#include "tools/exec_tool.h"
#include "tools/file_tool.h"
#include "tools/web_tool.h"
#include "tools/memory_tool.h"
#include "tools/calculator_tool.h"
#include "tools/custom_tool1.h"
#include "tools/custom_tool2.h"
#include "tools/custom_tool3.h"
#include "tools/vector_memory_tool.h"
#include "tools/gui_tool.h"
#include "tools/sub_agent_tool.h"
#include "agent/skill_loader.h"
#include "agent/agent_loop.h"
#include "agent/sub_agent_manager.h"
#include "harness/harness_runner.h"
#include "harness/keyword_evaluator.h"
#include "harness/functional_evaluator.h"

namespace {

void clean_benchmark_artifacts(bool clean_vector) {
    std::error_code ec;
    const char* files_to_remove[] = {
        "outputs/benchmark/out_simple.txt",
        "outputs/benchmark/cpp23_info.txt",
        "outputs/benchmark/sys_report.txt",
        "outputs/screen captures/screenshot.bmp"
    };
    for (const char* path : files_to_remove) {
        std::filesystem::remove(path, ec);
    }

    if (std::filesystem::exists("memory.db", ec)) {
        sqlite3* db = nullptr;
        if (sqlite3_open("memory.db", &db) == SQLITE_OK && db != nullptr) {
            sqlite3_exec(
                db,
                "DELETE FROM memory WHERE key IN ('calc_5', 'fallback_val') OR "
                "((key || ' ' || content) LIKE '%theme%' AND (key || ' ' || content) LIKE '%dark%');",
                nullptr, nullptr, nullptr);
        }
        if (db != nullptr) {
            sqlite3_close(db);
        }
    }

    if (clean_vector && std::filesystem::exists("vector_memory.db", ec)) {
        sqlite3* vdb = nullptr;
        if (sqlite3_open("vector_memory.db", &vdb) == SQLITE_OK && vdb != nullptr) {
            sqlite3_exec(
                vdb,
                "DELETE FROM embeddings WHERE text LIKE '%Apple is a fruit%';",
                nullptr, nullptr, nullptr);
        }
        if (vdb != nullptr) {
            sqlite3_close(vdb);
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    // Auto-detect and fix current working directory if run from build/ or build/Release/
    if (!std::filesystem::exists("benchmark/tasks.json")) {
        if (std::filesystem::exists("../benchmark/tasks.json")) {
            std::filesystem::current_path("..");
        } else if (std::filesystem::exists("../../benchmark/tasks.json")) {
            std::filesystem::current_path("../..");
        }
    }

    std::println("=== OOP AI Agent Framework 2026 - Batch Benchmark Runner ===");

    std::string tasks_file = "benchmark/tasks.json";
    bool enable_gui = false;
    bool use_mock = false;
    if (const char* env_mock = std::getenv("LLM_MOCK")) {
        std::string mock_val = env_mock;
        if (mock_val == "1" || mock_val == "true") {
            use_mock = true;
        }
    }
    agent::LLMConfig config;
    bool has_explicit_url = (std::getenv("LLM_BASE_URL") != nullptr);
    bool has_explicit_model = (std::getenv("LLM_MODEL") != nullptr);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::println("Usage: run_eval [OPTIONS] [tasks_file]");
            std::println("  --enable-gui           Enable GUI automation task");
            std::println("  --mock                 Run deterministic offline suite with MockLLMClient");
            std::println("  --base-url=URL          LLM server base URL");
            std::println("  --model=MODEL           LLM model name");
            std::println("  tasks_file              Path to JSON tasks file (default: benchmark/tasks.json)");
            std::println("\nEnvironment variables: LLM_BASE_URL, LLM_MODEL, LLM_MOCK");
            return 0;
        } else if (arg == "--enable-gui") {
            enable_gui = true;
        } else if (arg == "--mock") {
            use_mock = true;
        } else if (arg.rfind("--base-url=", 0) == 0) {
            config.base_url = arg.substr(11);
            has_explicit_url = true;
        } else if (arg == "--base-url" && i + 1 < argc) {
            config.base_url = argv[++i];
            has_explicit_url = true;
        } else if (arg.rfind("--model=", 0) == 0) {
            config.model_name = arg.substr(8);
            has_explicit_model = true;
        } else if (arg == "--model" && i + 1 < argc) {
            config.model_name = argv[++i];
            has_explicit_model = true;
        } else if (arg.rfind("--timeout=", 0) == 0) {
            config.timeout_seconds = std::stoi(arg.substr(10));
        } else {
            tasks_file = arg;
        }
    }

    if (config.base_url.empty() && std::getenv("LLM_BASE_URL")) config.base_url = std::getenv("LLM_BASE_URL");
    if (config.model_name.empty() && std::getenv("LLM_MODEL")) config.model_name = std::getenv("LLM_MODEL");

    if (use_mock) {
        if (config.base_url.empty() || !has_explicit_url) config.base_url = "mock://offline";
        if (config.model_name.empty() || !has_explicit_model) config.model_name = "mock-llm";
    } else {
        if (config.base_url.empty() || config.model_name.empty()) {
            std::cerr << "Error: LLM_BASE_URL and LLM_MODEL environment variables or CLI flags must be set.\n";
            return 1;
        }

        auto client_check = std::make_shared<agent::OllamaClient>();
        if (!client_check->ping(config.base_url, 3)) {
            agent::print_llm_unreachable_error(config.base_url);
            return 1;
        }
    }

    std::println("Using LLM Base URL : {}", config.base_url);
    std::println("Using LLM Model    : {}", config.model_name);

    if (!std::filesystem::exists(tasks_file)) {
        std::println("Error: Tasks file standard path '{}' not found.", tasks_file);
        return 1;
    }

    std::ifstream file(tasks_file);
    nlohmann::json root;
    file >> root;

    std::vector<agent::harness::TaskSpec> tasks;
    for (const auto& item : root) {
        agent::harness::TaskSpec spec;
        spec.task_id = item.value("task_id", "");
        spec.prompt = item.value("prompt", "");
        if (item.contains("expected_keywords")) {
            spec.expected_keywords = item["expected_keywords"].get<std::vector<std::string>>();
        }
        spec.test_command = item.value("test_command", "");
        spec.category = item.value("category", "general");
        
        if (spec.task_id == "task_bonus_gui" && !enable_gui) {
            std::println("Skipping GUI task [{}] because --enable-gui was not provided.", spec.task_id);
            continue;
        }
        if (use_mock && (spec.task_id == "task-med-02" || spec.task_id == "task_bonus_vector")) {
            std::println("[MOCK] Skipping task [{}] (requires live external HTTP/embedding service).", spec.task_id);
            continue;
        }
        
        tasks.push_back(spec);
    }

    std::println("Loaded {} tasks from benchmark configuration.", tasks.size());

    // Setup Agent environment
    std::shared_ptr<agent::LLMClient> client;
    if (use_mock) {
        client = std::make_shared<agent::MockLLMClient>();
    } else {
        client = std::make_shared<agent::OllamaClient>();
    }
    auto registry = std::make_shared<agent::ToolRegistry>();
    registry->register_tool(std::make_shared<agent::ExecTool>());
    registry->register_tool(std::make_shared<agent::ReadFileTool>());
    registry->register_tool(std::make_shared<agent::WriteFileTool>());
    registry->register_tool(std::make_shared<agent::WebSearchTool>());
    registry->register_tool(std::make_shared<agent::MemorySaveTool>());
    registry->register_tool(std::make_shared<agent::MemorySearchTool>());
    registry->register_tool(std::make_shared<agent::CalculatorTool>());
    registry->register_tool(std::make_shared<agent::DateTimeTool>());
    registry->register_tool(std::make_shared<agent::SystemInfoTool>());
    registry->register_tool(std::make_shared<agent::RegexExtractorTool>());
    registry->register_tool(std::make_shared<agent::VectorMemoryTool>());
    auto sub_manager = std::make_shared<agent::SubAgentManager>();
    auto spawner = [sub_manager, client, registry](const std::string& id, const std::string& task) {
        auto sub_loop = std::make_shared<agent::AgentLoop>(client, registry, nullptr, 12);
        sub_manager->spawn_agent(id, sub_loop, task);
    };
    auto sender = [sub_manager](const std::string& id, const std::string& msg) { sub_manager->send_message(id, msg); };
    auto receiver = [sub_manager](const std::string& id) { return sub_manager->receive_message(id); };
    registry->register_tool(std::make_shared<agent::SubAgentTool>(spawner, sender, receiver));
    if (enable_gui) {
        registry->register_tool(std::make_shared<agent::GuiTool>());
    }

    auto skill_loader = std::make_shared<agent::SkillLoader>();
    skill_loader->load_skills("skills");

    auto agent_loop = std::make_shared<agent::AgentLoop>(client, registry, skill_loader);
    agent::harness::HarnessRunner runner(agent_loop, "trajectories");

    std::size_t passed_count = 0;
    std::size_t total_tokens = 0;
    double total_time_ms = 0.0;

    clean_benchmark_artifacts(!use_mock);

    for (const auto& task : tasks) {
        std::println("\n[{}] Running Task [{}] (Category: {}): {}", agent::utils::get_local_timestamp(), task.task_id, task.category, task.prompt);
        
        auto summary = runner.run_task(task, config);

        total_tokens = agent::cpp26::safe_add(total_tokens, summary.trajectory.total_tokens);
        total_time_ms += summary.trajectory.total_time_ms;

        bool success = true;
        if (!task.expected_keywords.empty()) {
            success = success && summary.keyword_eval.passed;
        }
        success = success && summary.functional_eval.passed;
        if (success) passed_count++;

        std::println("[{}]  -> Result: {} | Tokens: {} | Latency: {:.2f}ms | Feedback: {} | Functional: {}",
                     agent::utils::get_local_timestamp(),
                     success ? "PASSED" : "FAILED",
                     summary.trajectory.total_tokens,
                     summary.trajectory.total_time_ms,
                     summary.keyword_eval.feedback,
                     summary.functional_eval.feedback);
    }

    double success_rate = tasks.empty() ? 0.0 : (static_cast<double>(passed_count) / tasks.size()) * 100.0;
    std::println("\n=== BENCHMARK EVALUATION SUMMARY ===");
    std::println("Total Tasks Evaluated : {}", tasks.size());
    std::println("Passed Tasks          : {}", passed_count);
    std::println("Success Rate          : {:.2f}%", success_rate);
    std::println("Total Tokens Used     : {}", total_tokens);
    std::println("Total Latency Time    : {:.2f}ms", total_time_ms);

    nlohmann::json summary_json;
    summary_json["total_tasks"] = tasks.size();
    summary_json["passed_tasks"] = passed_count;
    summary_json["success_rate"] = success_rate;
    summary_json["total_tokens"] = total_tokens;
    summary_json["total_time_ms"] = total_time_ms;

    std::ofstream sum_file("trajectories/benchmark_summary.json");
    if (sum_file.is_open()) {
        sum_file << summary_json.dump(2);
        std::println("Saved benchmark summary to trajectories/benchmark_summary.json");
    }

    return passed_count == tasks.size() ? 0 : 1;
}
