#include <print>
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include <curl/curl.h>
#include <sqlite3.h>

#include "client/ollama_client.h"
#include "tools/tool_registry.h"
#include "tools/exec_tool.h"
#include "tools/file_tool.h"
#include "tools/web_tool.h"
#include "tools/memory_tool.h"
#include "tools/calculator_tool.h"
#include "tools/custom_tool1.h"
#include "tools/custom_tool2.h"
#include "tools/custom_tool3.h"
#include "tools/gui_tool.h"
#include "tools/vector_memory_tool.h"

#include "agent/skill_loader.h"
#include "agent/agent_loop.h"
#include "dto/trajectory.h"
#include "dto/step.h"

void run_interactive_chat() {
    std::println("=== OOP AI Agent Framework 2026 ===");
    std::println("Interactive CLI mode started. Type 'exit' or 'quit' to stop.\n");

    auto client = std::make_shared<agent::OllamaClient>();
    auto registry = std::make_shared<agent::ToolRegistry>();
    
    registry->register_tool(std::make_shared<agent::ExecTool>());
    registry->register_tool(std::make_shared<agent::ReadFileTool>());
    registry->register_tool(std::make_shared<agent::WriteFileTool>());
    registry->register_tool(std::make_shared<agent::WebSearchTool>());
    registry->register_tool(std::make_shared<agent::MemorySaveTool>("agent_memory.db"));
    registry->register_tool(std::make_shared<agent::MemorySearchTool>("agent_memory.db"));
    registry->register_tool(std::make_shared<agent::CalculatorTool>());
    registry->register_tool(std::make_shared<agent::DateTimeTool>());
    registry->register_tool(std::make_shared<agent::SystemInfoTool>());
    registry->register_tool(std::make_shared<agent::RegexExtractorTool>());
    registry->register_tool(std::make_shared<agent::VectorMemoryTool>());
    registry->register_tool(std::make_shared<agent::GuiTool>());

    auto skill_loader = std::make_shared<agent::SkillLoader>();
    if (std::filesystem::exists("skills")) {
        skill_loader->load_skills("skills");
    }

    agent::AgentLoop agent_loop(client, registry, skill_loader, 15);
    agent_loop.set_interactive_cli(true);

    agent::LLMConfig config;
    if (const char* base_url = std::getenv("LLM_BASE_URL")) config.base_url = base_url;
    if (const char* model = std::getenv("LLM_MODEL")) config.model_name = model;

    if (config.base_url.empty() || config.model_name.empty()) {
        std::cerr << "[ERROR] LLM_BASE_URL and LLM_MODEL environment variables must be set.\n";
        std::cerr << "Example: $env:LLM_BASE_URL=\"http://localhost:11434\"; $env:LLM_MODEL=\"qwen2.5:7b\"\n";
        return;
    }
    
    std::cout << "[INFO] Using Base URL: " << config.base_url << "\n";
    std::cout << "[INFO] Using Model: " << config.model_name << "\n";

    int task_counter = 1;
    while (true) {
        std::print("\nUser> ");
        std::string input;
        if (!std::getline(std::cin, input)) break;
        if (input == "exit" || input == "quit") break;
        if (input.empty()) continue;

        std::string task_id = "task_" + std::to_string(task_counter++);
        
        agent_loop.set_step_hook([](const agent::Step& step) {
            std::println("\n[Agent] Thought: {}", step.thought);
            if (!step.action.empty() && step.action.contains("tool") && !step.action["tool"].get<std::string>().empty()) {
                std::println("[Agent] Action: {} | Args: {}", step.action["tool"].get<std::string>(), step.action["args"].dump());
            }
        });

        std::println("\n[Agent] Thinking...");
        auto traj = agent_loop.run(task_id, input, config);
        
        if (traj.success && !traj.steps.empty()) {
            std::println("\n[Result]: {}", traj.steps.back().tool_result);
        } else {
            std::println("\n[Error]: Task failed or reached maximum steps.");
            if (!traj.steps.empty()) {
                std::println("Last tool result: {}", traj.steps.back().tool_result);
            }
        }
    }
}

int main() {
    if (!std::filesystem::exists("skills") && std::filesystem::exists("../skills")) {
        std::filesystem::current_path("..");
    } else if (!std::filesystem::exists("skills") && std::filesystem::exists("../../skills")) {
        std::filesystem::current_path("../..");
    }

    // Initialize required global states
    curl_global_init(CURL_GLOBAL_ALL);

    run_interactive_chat();

    curl_global_cleanup();
    return 0;
}
