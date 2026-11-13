#include "mock_llm_client.h"
#include <algorithm>
#include <cctype>

namespace agent {

bool MockLLMClient::ping(const std::string& base_url, int timeout_seconds) {
    (void)base_url;
    (void)timeout_seconds;
    return true;
}

void MockLLMClient::set_canned_response(const std::string& resp) {
    canned_response_ = resp;
}

void MockLLMClient::clear_canned_response() {
    canned_response_.reset();
}

LLMResponse MockLLMClient::chat(const std::vector<Message>& history, const LLMConfig& config) {
    (void)config;
    LLMResponse resp;
    resp.tokens_used = 10;
    resp.raw_json = "{}";

    if (canned_response_.has_value()) {
        resp.content = *canned_response_;
        return resp;
    }

    if (history.empty()) {
        resp.content = "Thought: I will answer the user request directly.\nFinal Answer: Mock response to user request.";
        return resp;
    }

    const auto& last = history.back();
    std::string role_lower = last.role;
    std::transform(role_lower.begin(), role_lower.end(), role_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::string content_lower = last.content;
    std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::string user_lower;
    int tool_turns = 0;
    for (const auto& msg : history) {
        std::string msg_role = msg.role;
        std::transform(msg_role.begin(), msg_role.end(), msg_role.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (msg_role == "user" && user_lower.empty()) {
            user_lower = msg.content;
            std::transform(user_lower.begin(), user_lower.end(), user_lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        } else if (msg_role == "tool") {
            ++tool_turns;
        }
    }

    if (user_lower.find("(15 + 25) * 4") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: I need to calculate (15 + 25) * 4.\nAction: calculator\nAction Input: {\"expression\": \"(15 + 25) * 4\"}";
        } else {
            resp.content = "Thought: I have received the calculator result.\nFinal Answer: Using calculator, the result is 160.";
        }
        return resp;
    }

    if (user_lower.find("out_simple.txt") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: I will write the greeting to outputs/benchmark/out_simple.txt.\nAction: write_file\nAction Input: {\"path\": \"outputs/benchmark/out_simple.txt\", \"content\": \"Hello AI Agent Framework 2026\"}";
        } else {
            resp.content = "Thought: File written successfully.\nFinal Answer: Used write_file to write to outputs/benchmark/out_simple.txt.";
        }
        return resp;
    }

    if (user_lower.find("current system time") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: I need to get the current system time.\nAction: get_system_time\nAction Input: {}";
        } else {
            resp.content = "Thought: Received system time.\nFinal Answer: Called get_system_time and retrieved timestamp: " + last.content;
        }
        return resp;
    }

    if (user_lower.find("system_info_tool") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: I will inspect system info.\nAction: system_info_tool\nAction Input: {}";
        } else {
            resp.content = "Thought: Received system info.\nFinal Answer: Executed system_info_tool to inspect cpu_threads: " + last.content;
        }
        return resp;
    }

    if (user_lower.find("calc_5") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: First compute (100 - 35) / 5.\nAction: calculator\nAction Input: {\"expression\": \"(100 - 35) / 5\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Now save 13 under key calc_5.\nAction: memory_save\nAction Input: {\"key\": \"calc_5\", \"content\": \"13\"}";
        } else {
            resp.content = "Thought: Value saved.\nFinal Answer: Calculated 13 with calculator and saved via memory_save to calc_5.";
        }
        return resp;
    }

    if (user_lower.find("version.txt") != std::string::npos &&
        user_lower.find("regex") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: First read version.txt.\nAction: read_file\nAction Input: {\"path\": \"version.txt\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Extract the date with regex_extractor_tool.\nAction: regex_extractor_tool\nAction Input: {\"text\": \"OOP AI Agent Framework 2026 Release Date: 2026-11-13\", \"pattern\": \"\\\\d{4}-\\\\d{2}-\\\\d{2}\"}";
        } else {
            resp.content = "Thought: Extracted release date.\nFinal Answer: Used read_file and regex_extractor_tool to extract 2026-11-13.";
        }
        return resp;
    }

    if (user_lower.find("theme=dark") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: Save theme=dark to memory.\nAction: memory_save\nAction Input: {\"key\": \"theme\", \"content\": \"theme=dark\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Search memory for theme.\nAction: memory_search\nAction Input: {\"query\": \"theme\"}";
        } else {
            resp.content = "Thought: Confirmed stored preference.\nFinal Answer: Used memory_save and memory_search for theme=dark.";
        }
        return resp;
    }

    if (user_lower.find("non_existent.txt") != std::string::npos &&
        user_lower.find("fallback_val") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: Try reading non_existent.txt.\nAction: read_file\nAction Input: {\"path\": \"non_existent.txt\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Fallback to computing 2^10.\nAction: calculator\nAction Input: {\"expression\": \"2^10\"}";
        } else if (tool_turns == 2) {
            resp.content = "Thought: Save 1024 to fallback_val.\nAction: memory_save\nAction Input: {\"key\": \"fallback_val\", \"content\": \"1024\"}";
        } else {
            resp.content = "Thought: Fallback complete.\nFinal Answer: After read_file failed, used calculator to get 1024 and saved with memory_save.";
        }
        return resp;
    }

    if (user_lower.find("win32_processor") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: Query CPU via PowerShell.\nAction: exec_tool\nAction Input: {\"command\": \"powershell.exe -NoProfile -Command \\\"(Get-CimInstance Win32_Processor).Name\\\"\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Query RAM via PowerShell.\nAction: exec_tool\nAction Input: {\"command\": \"powershell.exe -NoProfile -Command \\\"$s=0; foreach($m in Get-CimInstance Win32_PhysicalMemory){$s+=$m.Capacity}; [math]::Round($s/1GB)\\\"\"}";
        } else if (tool_turns == 2) {
            resp.content = "Thought: Query GPU via PowerShell.\nAction: exec_tool\nAction Input: {\"command\": \"powershell.exe -NoProfile -Command \\\"(Get-CimInstance Win32_VideoController)[0].Name\\\"\"}";
        } else if (tool_turns == 3) {
            resp.content = "Thought: Write system report.\nAction: write_file\nAction Input: {\"path\": \"outputs/benchmark/sys_report.txt\", \"content\": \"CPU: MockCPU\\nRAM: 16 GB\\nGPU: MockGPU\\n\"}";
        } else {
            resp.content = "Thought: System report written.\nFinal Answer: Executed exec_tool commands and write_file to outputs/benchmark/sys_report.txt.";
        }
        return resp;
    }

    if (user_lower.find("worker1") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: Spawn worker1.\nAction: sub_agent\nAction Input: {\"action\": \"spawn\", \"id\": \"worker1\", \"task\": \"Respond to ping\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Send message to worker1.\nAction: sub_agent\nAction Input: {\"action\": \"send\", \"id\": \"worker1\", \"msg\": \"Hello worker1\"}";
        } else {
            resp.content = "Thought: Sub-agent workflow complete.\nFinal Answer: Used sub_agent to spawn worker1 and send a message.";
        }
        return resp;
    }

    if (user_lower.find("gui_tool") != std::string::npos &&
        user_lower.find("screenshot") != std::string::npos) {
        if (tool_turns == 0) {
            resp.content = "Thought: Capture screenshot first.\nAction: gui_tool\nAction Input: {\"action\": \"screenshot\"}";
        } else if (tool_turns == 1) {
            resp.content = "Thought: Click at 10,10.\nAction: gui_tool\nAction Input: {\"action\": \"click\", \"x\": 10, \"y\": 10}";
        } else {
            resp.content = "Thought: GUI actions complete.\nFinal Answer: Used gui_tool screenshot and click at 10,10.";
        }
        return resp;
    }

    if (role_lower == "tool" ||
        content_lower.find("observation") != std::string::npos ||
        content_lower.find("tool result") != std::string::npos) {
        resp.content = "Thought: I have received the tool result.\nFinal Answer: Task completed successfully.";
        return resp;
    }

    if (content_lower.find("calculate") != std::string::npos ||
        content_lower.find("+") != std::string::npos ||
        content_lower.find("*") != std::string::npos ||
        content_lower.find("math") != std::string::npos) {
        resp.content = "Thought: I need to calculate this expression.\nAction: calculator\nAction Input: {\"expression\": \"2 + 2\"}";
        return resp;
    }

    if (content_lower.find("file") != std::string::npos ||
        content_lower.find("read") != std::string::npos) {
        resp.content = "Thought: I need to read the file.\nAction: read_file\nAction Input: {\"path\": \"test.txt\"}";
        return resp;
    }

    if (content_lower.find("result") != std::string::npos) {
        resp.content = "Thought: I have received the tool result.\nFinal Answer: Task completed successfully.";
        return resp;
    }

    resp.content = "Thought: I will answer the user request directly.\nFinal Answer: Mock response to user request.";
    return resp;
}

} // namespace agent
