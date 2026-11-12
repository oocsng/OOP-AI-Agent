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
