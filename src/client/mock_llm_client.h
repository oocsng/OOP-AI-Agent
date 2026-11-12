#pragma once

#include "llm_client.h"
#include <optional>
#include <string>
#include <vector>

namespace agent {

class MockLLMClient : public LLMClient {
public:
    MockLLMClient() = default;
    ~MockLLMClient() override = default;

    LLMResponse chat(const std::vector<Message>& history, const LLMConfig& config) override;
    bool ping(const std::string& base_url = "", int timeout_seconds = 3) override;

    void set_canned_response(const std::string& resp);
    void clear_canned_response();

private:
    std::optional<std::string> canned_response_;
};

} // namespace agent
