#pragma once

#include "llm_client.h"
#include <string>
#include <vector>

namespace agent {

class OllamaClient : public LLMClient {
public:
    OllamaClient() = default;
    ~OllamaClient() override = default;

    LLMResponse chat(const std::vector<Message>& history, const LLMConfig& config) override;
    bool ping(const std::string& base_url = "", int timeout_seconds = 3) override;

    // Base64 helper for image encoding
    static std::string encode_file_base64(const std::string& file_path);
    static std::string build_ping_url(const std::string& base_url);
    static long effective_timeout_seconds(const LLMConfig& config);
};

} // namespace agent
