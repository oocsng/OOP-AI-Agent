#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "dto/message.h"

namespace agent {

struct LLMConfig {
  std::string base_url{"http://localhost:11434"};
  std::string model_name{"qwen2.5:7b"};
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

class LLMClient {
 public:
  virtual ~LLMClient() = default;
  virtual LLMResponse chat(const std::vector<Message>& history,
                           const LLMConfig& config) = 0;
};

}  // namespace agent
