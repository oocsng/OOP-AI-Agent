#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "dto/message.h"

#include <cstdlib>
#include <iostream>

namespace agent {

struct LLMConfig {
  std::string base_url{[] {
    const char* env_url = std::getenv("LLM_BASE_URL");
    if (!env_url || env_url[0] == '\0') {
      std::cerr << "[WARN] LLM_BASE_URL is not set. Defaulting to local: http://localhost:11434\n";
      std::cerr << "[INFO] If using a remote model (e.g., Colab), run this in PowerShell before executing:\n";
      std::cerr << "       $env:LLM_BASE_URL=\"https://your-ngrok-url.com\"\n";
      return std::string("http://localhost:11434");
    }
    return std::string(env_url);
  }()};
  std::string model_name{[] {
    const char* env_model = std::getenv("LLM_MODEL");
    if (!env_model || env_model[0] == '\0') {
      std::cerr << "[WARN] LLM_MODEL is not set. Defaulting to: qwen2.5:7b\n";
      std::cerr << "[INFO] To override, run: $env:LLM_MODEL=\"your-model-name\"\n";
      return std::string("qwen2.5:7b");
    }
    return std::string(env_model);
  }()};
  double temperature{0.2};
  int max_tokens{2048};
  int timeout_seconds{60};
};

struct LLMResponse {
  std::string content;
  std::string raw_json;
  int tokens_used{0};
};

class LLMException : public std::runtime_error {
 public:
  explicit LLMException(const std::string& message)
      : std::runtime_error(message) {}
};

class HTTPException : public LLMException {
  int status_code_;

 public:
  HTTPException(int code, const std::string& msg)
      : LLMException("HTTP Error " + std::to_string(code) + ": " + msg),
        status_code_(code) {}
  [[nodiscard]] int status_code() const noexcept { return status_code_; }
};

class ConnectionException : public LLMException {
 public:
  explicit ConnectionException(const std::string& msg)
      : LLMException("Connection Error: " + msg) {}
};

class JSONParseException : public LLMException {
 public:
  explicit JSONParseException(const std::string& msg)
      : LLMException("JSON Parse Error: " + msg) {}
};

inline void print_llm_unreachable_error(const std::string& base_url) {
  std::cerr << "\n==========================================================\n";
  std::cerr << "[CRITICAL ERROR] LLM Server Unreachable!\n";
  std::cerr << "Tried to connect to: " << base_url << "\n";
  std::cerr << "The tests/app cannot proceed. Please ensure:\n";
  std::cerr << " 1. If running locally, the Ollama app is open and running.\n";
  std::cerr << " 2. If using Colab/Remote, your $env:LLM_BASE_URL is correct and the server is awake.\n";
  std::cerr << "==========================================================\n\n";
}

class LLMClient {
 public:
  virtual ~LLMClient() = default;
  virtual LLMResponse chat(const std::vector<Message>& history,
                           const LLMConfig& config) = 0;
  virtual bool ping(const std::string& base_url = "", int timeout_seconds = 3) {
    (void)base_url;
    (void)timeout_seconds;
    return true;
  }
};

}  // namespace agent
