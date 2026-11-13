#include "agent_loop.h"

#include <iostream>
#include <regex>
#include <sstream>

#include "utils/time_utils.hpp"
#include "common/cpp_features.h"

namespace agent {

static std::string sanitize_and_format_observation(
    const std::string& raw_output) {
  std::string sanitized;
  sanitized.reserve(std::min<size_t>(raw_output.size(), 3100));

  // Collapse consecutive empty lines
  int newline_count = 0;
  for (char c : raw_output) {
    if (c == '\r') continue;
    if (c == '\n') {
      newline_count++;
      if (newline_count <= 2) {
        sanitized += '\n';
      }
    } else {
      newline_count = 0;
      sanitized += c;
    }
  }

  // Truncate payloads exceeding 3000 characters
  if (sanitized.length() > 3000) {
    sanitized = sanitized.substr(0, 3000) +
                "\n...[Output truncated to 3000 characters]";
  }

  // Target 6: Self-Correction Injection on tool failures
  bool is_error = (sanitized.rfind("Error", 0) == 0 ||
                   sanitized.find("Error:") != std::string::npos ||
                   sanitized.find("Exception") != std::string::npos ||
                   sanitized.find("Invalid") != std::string::npos);

  if (is_error) {
    return "Tool Result: " + sanitized +
           "\nSystem Guidance: The tool call failed. Analyze the error above, "
           "correct the parameters or select a different tool, and avoid "
           "repeating the identical failed action.";
  }

  return "Tool Result: " + sanitized;
}

std::string AgentLoop::build_system_prompt(const std::string& user_prompt) {
  std::string base_prompt =
      "You are an intelligent ReAct AI Agent operating in an iterative loop.\n"
      "Analyze tasks step by step and execute tools to achieve the goal.\n\n"
      "AVAILABLE TOOLS SCHEMA:\n";

  if (tool_registry_) {
    base_prompt += tool_registry_->get_tools_schema_json() + "\n\n";
  }

  base_prompt +=
      "RESPONSE FORMAT INSTRUCTIONS:\n"
      "For each turn, format your response EXACTLY as:\n"
      "Thought: <your step-by-step reasoning>\n"
      "Action: <tool_name>\n"
      "Action Input: <json_formatted_tool_args>\n\n"
      "MANDATORY TOOL EXECUTION RULES:\n"
      "1. If the task requires regex pattern extraction, you MUST call "
      "'regex_extractor_tool' (with 'text' and 'pattern' arguments).\n"
      "2. If the task requires calculation, web search, file read/write, or "
      "memory storage, you MUST execute the corresponding tool ('calculator', "
      "'web_search', 'read_file', 'write_file', 'memory_save', "
      "'memory_search').\n"
      "3. NEVER skip calling the requested tool. Always execute the tool "
      "first, inspect the Tool Result, and only then provide the Final "
      "Answer.\n"
      "4. CRITICAL: DO NOT use `gui_tool` (action `type` or `key_press`) to "
      "send text messages, greetings, or conversational answers to the user. "
      "General chat and answers MUST be returned directly as standard text "
      "without calling any tools.\n"
      "5. The `gui_tool` is STRICTLY reserved for GUI automation tasks on "
      "external desktop or browser applications.\n\n"
      "When you have finished executing all required tools, respond with:\n"
      "Thought: <final summary reasoning>\n"
      "Final Answer: <your final answer to the user>\n";

  if (skill_loader_) {
    base_prompt = skill_loader_->auto_inject(base_prompt, user_prompt);
  }

  return base_prompt;
}

ParsedAction AgentLoop::parse_llm_response(const std::string& response_text) {
  ParsedAction action;

  // Check for Final Answer
  size_t final_pos = response_text.find("Final Answer:");
  if (final_pos != std::string::npos) {
    action.is_final_answer = true;
    action.final_answer = response_text.substr(final_pos + 13);
    // Trim whitespace
    action.final_answer.erase(
        0, action.final_answer.find_first_not_of(" \t\r\n`"));
    action.final_answer.erase(action.final_answer.find_last_not_of(" \t\r\n`") +
                              1);

    size_t thought_pos = response_text.find("Thought:");
    if (thought_pos != std::string::npos && thought_pos < final_pos) {
      action.thought =
          response_text.substr(thought_pos + 8, final_pos - (thought_pos + 8));
    } else {
      action.thought = response_text.substr(0, final_pos);
    }
    return action;
  }

  // Extract Thought
  size_t thought_pos = response_text.find("Thought:");
  size_t action_pos = response_text.find("Action:");
  if (thought_pos != std::string::npos) {
    if (action_pos != std::string::npos && action_pos > thought_pos) {
      action.thought =
          response_text.substr(thought_pos + 8, action_pos - (thought_pos + 8));
    } else {
      action.thought = response_text.substr(thought_pos + 8);
    }
  }

  // Extract Action & Action Input
  if (action_pos != std::string::npos) {
    size_t input_pos = response_text.find("Action Input:", action_pos);
    if (input_pos != std::string::npos) {
      action.tool_name =
          response_text.substr(action_pos + 7, input_pos - (action_pos + 7));
      // Trim action name (including backticks)
      action.tool_name.erase(0, action.tool_name.find_first_not_of(" \t\r\n`"));
      action.tool_name.erase(action.tool_name.find_last_not_of(" \t\r\n`") + 1);

      std::string input_str = response_text.substr(input_pos + 13);

      // Truncate if LLM chained multiple actions or final answer
      size_t next_thought = input_str.find("\nThought:");
      if (next_thought != std::string::npos)
        input_str = input_str.substr(0, next_thought);
      size_t next_action = input_str.find("\nAction:");
      if (next_action != std::string::npos)
        input_str = input_str.substr(0, next_action);
      size_t next_final = input_str.find("\nFinal Answer:");
      if (next_final != std::string::npos)
        input_str = input_str.substr(0, next_final);

      input_str.erase(0, input_str.find_first_not_of(" \t\r\n"));
      input_str.erase(input_str.find_last_not_of(" \t\r\n") + 1);

      // Strip markdown code fences if present
      if (input_str.rfind("```", 0) == 0) {
        size_t nl = input_str.find_first_of("\r\n");
        if (nl != std::string::npos) {
          input_str = input_str.substr(nl + 1);
        } else {
          input_str = input_str.substr(3);
        }
        size_t end_fence = input_str.rfind("```");
        if (end_fence != std::string::npos) {
          input_str = input_str.substr(0, end_fence);
        }
        input_str.erase(0, input_str.find_first_not_of(" \t\r\n`"));
        input_str.erase(input_str.find_last_not_of(" \t\r\n`") + 1);
      }

      try {
        action.tool_args = nlohmann::json::parse(input_str);
      } catch (...) {
        try {
          std::string repaired;
          bool in_string = false;
          bool escaped = false;
          for (size_t i = 0; i < input_str.length(); ++i) {
            char c = input_str[i];
            if (c == '\\' && !escaped) {
              escaped = true;
              repaired += c;
            } else {
              if (c == '"' && !escaped) {
                in_string = !in_string;
              } else if (in_string && !escaped) {
                if (c == '\n') {
                  repaired += "\\n";
                  continue;
                }
                if (c == '\r') {
                  repaired += "\\r";
                  continue;
                }
                if (c == '\t') {
                  repaired += "\\t";
                  continue;
                }
              }
              escaped = false;
              repaired += c;
            }
          }
          action.tool_args = nlohmann::json::parse(repaired);
        } catch (...) {
          // If not raw JSON, try wrapping input into string argument
          action.tool_args = {{"raw_input", input_str}};
        }
      }
    }
  } else {
    // Fallback: try parsing JSON object from full response
    try {
      nlohmann::json raw_j = nlohmann::json::parse(response_text);
      if (raw_j.contains("action") && raw_j["action"].is_string()) {
        action.tool_name = raw_j["action"].get<std::string>();
        if (raw_j.contains("action_input")) {
          action.tool_args = raw_j["action_input"];
        }
      }
      if (raw_j.contains("thought")) {
        action.thought = raw_j["thought"].get<std::string>();
      }
      if (raw_j.contains("final_answer")) {
        action.is_final_answer = true;
        action.final_answer = raw_j["final_answer"].get<std::string>();
      }
    } catch (...) {
      // Unstructured text fallback
      action.is_final_answer = true;
      action.final_answer = response_text;
    }
  }

  return action;
}

bool AgentLoop::is_goal_achieved(const ParsedAction& action) {
  return action.is_final_answer;
}

LLMResponse AgentLoop::think(const std::vector<Message>& history,
                             const LLMConfig& config) {
  if (!client_) {
    throw std::runtime_error("LLMClient is not set in AgentLoop.");
  }
  return client_->chat(history, config);
}

std::string AgentLoop::act(const std::string& tool_name,
                           const nlohmann::json& tool_args) {
  std::string args_str = tool_args.dump();
  if (args_str.find("Action:") != std::string::npos ||
      args_str.find("Action Input:") != std::string::npos) {
    return "Error: Chained actions detected in tool arguments. You MUST "
           "execute EXACTLY ONE tool per turn and wait for the observation. Do "
           "NOT output 'Action:' inside the arguments.";
  }
  if (!tool_registry_) {
    return "Error: ToolRegistry is not set in AgentLoop.";
  }
  return tool_registry_->execute_tool(tool_name, tool_args);
}

void AgentLoop::clear_history() {
  history_.clear();
  loop_detector_.reset();
}

Trajectory AgentLoop::run(const std::string& task_id,
                          const std::string& user_prompt,
                          const LLMConfig& config) {
  Trajectory trajectory;
  trajectory.task_id = task_id;
  trajectory.model = config.model_name;
  trajectory.success = false;
  trajectory.timestamp = utils::get_iso8601_timestamp();

  auto start_time = std::chrono::high_resolution_clock::now();
  loop_detector_.reset();
  if (!is_interactive_cli_) {
    history_.clear();
  }

  if (history_.empty()) {
    std::string system_prompt = build_system_prompt(user_prompt);
    history_.push_back(
        {.role = "system", .content = system_prompt, .images = {}});
  }
  history_.push_back({.role = "user", .content = user_prompt, .images = {}});

  for (std::size_t step_idx = 1; step_idx <= max_steps_; ++step_idx) {
    auto step_start = std::chrono::high_resolution_clock::now();

    Step current_step;
    current_step.step_id = step_idx;
    current_step.timestamp = utils::get_iso8601_timestamp();

    LLMResponse llm_resp;
    try {
      llm_resp = think(history_, config);
    } catch (const std::exception& e) {
      current_step.thought =
          "LLM Communication Error: " + std::string(e.what());
      current_step.tool_result = "Execution stopped due to LLM error.";
      trajectory.steps.push_back(current_step);
      if (step_hook_) step_hook_(current_step);
      break;
    }

    current_step.tokens_used = llm_resp.tokens_used;
    trajectory.total_tokens = agent::cpp26::safe_add(trajectory.total_tokens, llm_resp.tokens_used);

    ParsedAction action = parse_llm_response(llm_resp.content);

    current_step.thought = action.thought;
    if (action.is_final_answer) {
      current_step.action = {{"type", "final_answer"},
                             {"tool", ""},
                             {"args", action.final_answer}};
    } else {
      current_step.action = {{"type", "tool_call"},
                             {"tool", action.tool_name},
                             {"args", action.tool_args}};
    }

    if (is_goal_achieved(action)) {
      current_step.tool_result = action.final_answer;
      auto step_end = std::chrono::high_resolution_clock::now();
      current_step.latency_ms =
          std::chrono::duration<double, std::milli>(step_end - step_start)
              .count();

      trajectory.steps.push_back(current_step);
      if (step_hook_) step_hook_(current_step);

      history_.push_back(
          {.role = "assistant", .content = llm_resp.content, .images = {}});

      trajectory.success = true;
      break;
    }

    // Loop Detection Check
    LoopCheckResult loop_check =
        loop_detector_.record_and_check(action.tool_name, action.tool_args);
    if (loop_check.status == LoopStatus::CriticalStop) {
      current_step.tool_result = "[LOOP DETECTOR STOP] " + loop_check.message;
      auto step_end = std::chrono::high_resolution_clock::now();
      current_step.latency_ms =
          std::chrono::duration<double, std::milli>(step_end - step_start)
              .count();

      trajectory.steps.push_back(current_step);
      if (step_hook_) step_hook_(current_step);
      break;
    }

    // Act step
    std::string tool_output;
    try {
      tool_output = act(action.tool_name, action.tool_args);
    } catch (const std::exception& e) {
      tool_output = "Error executing tool: " + std::string(e.what());
    }
    current_step.tool_result = tool_output;

    auto step_end = std::chrono::high_resolution_clock::now();
    current_step.latency_ms =
        std::chrono::duration<double, std::milli>(step_end - step_start)
            .count();

    trajectory.steps.push_back(current_step);
    if (step_hook_) step_hook_(current_step);

    // Observe: add assistant response and tool output to history
    std::string obs_content = sanitize_and_format_observation(tool_output);
    if (loop_check.status == LoopStatus::Warning) {
      obs_content += "\n[LOOP DETECTOR WARNING] " + loop_check.message +
                     " Do not repeat the same tool call with identical arguments; "
                     "change parameters, pick a different tool, or provide Final Answer.";
    }
    history_.push_back(
        {.role = "assistant", .content = llm_resp.content, .images = {}});
    history_.push_back({.role = "tool", .content = obs_content, .images = {}});
  }

  auto total_end = std::chrono::high_resolution_clock::now();
  trajectory.total_time_ms =
      std::chrono::duration<double, std::milli>(total_end - start_time).count();
  trajectory.execution_duration_ms =
      utils::get_formatted_duration_ms(start_time, total_end);

  return trajectory;
}

}  // namespace agent
