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

    // Base64 helper for image encoding
    static std::string encode_file_base64(const std::string& file_path);
};

} // namespace agent
