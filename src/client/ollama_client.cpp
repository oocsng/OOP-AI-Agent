#include "ollama_client.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace agent {

static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::string OllamaClient::encode_file_base64(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw LLMException("Failed to open image file for base64 encoding: " + file_path);
    }
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = buffer.size();
    size_t pos = 0;

    while (in_len--) {
        char_array_3[i++] = buffer[pos++];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; ++i) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; ++j) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; ++j) ret += base64_chars[char_array_4[j]];
        while (i++ < 3) ret += '=';
    }

    return ret;
}

LLMResponse OllamaClient::chat(const std::vector<Message>& history, const LLMConfig& config) {
    CURL* raw_curl = curl_easy_init();
    if (!raw_curl) {
        throw ConnectionException("Failed to initialize libcurl instance.");
    }
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(raw_curl, curl_easy_cleanup);

    nlohmann::json payload;
    if (config.model_name.empty()) throw ConnectionException("LLM model name is not configured.");
    payload["model"] = config.model_name;
    payload["stream"] = false;
    payload["options"] = {
        {"temperature", config.temperature},
        {"num_predict", config.max_tokens}
    };

    nlohmann::json messages_json = nlohmann::json::array();
    for (const auto& msg : history) {
        nlohmann::json m_obj;
        m_obj["role"] = msg.role;
        m_obj["content"] = msg.content;
        if (!msg.images.empty()) {
            m_obj["images"] = msg.images;
        }
        messages_json.push_back(m_obj);
    }
    payload["messages"] = messages_json;

    std::string request_body = payload.dump();
    std::string response_body;

    if (config.base_url.empty()) throw ConnectionException("LLM base URL is not configured.");
    std::string endpoint = config.base_url;
    if (endpoint.back() == '/') {
        endpoint.pop_back();
    }
    if (!endpoint.ends_with("/api/chat") && !endpoint.ends_with("/v1/chat/completions")) {
        endpoint += "/api/chat";
    }

    struct curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw_headers, curl_slist_free_all);

    curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(config.timeout_seconds));

    CURLcode res = curl_easy_perform(curl.get());
    long http_code = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        throw ConnectionException(std::string("LLM server is offline or unreachable. Details: ") + curl_easy_strerror(res));
    }

    if (http_code != 200) {
        if (http_code == 404) {
            throw HTTPException(static_cast<int>(http_code), "Model not found on remote server. Details: " + response_body);
        }
        throw HTTPException(static_cast<int>(http_code), response_body);
    }

    try {
        nlohmann::json resp_json = nlohmann::json::parse(response_body);
        LLMResponse response;
        response.raw_json = response_body;

        if (resp_json.contains("message") && resp_json["message"].contains("content")) {
            response.content = resp_json["message"]["content"].get<std::string>();
        } else if (resp_json.contains("choices") && !resp_json["choices"].empty() &&
                   resp_json["choices"][0].contains("message")) {
            response.content = resp_json["choices"][0]["message"]["content"].get<std::string>();
        } else {
            throw JSONParseException("Missing message.content field in JSON response.");
        }

        if (response.content.empty()) {
            throw JSONParseException("LLM returned an empty string content.");
        }

        int total_tokens = 0;
        if (resp_json.contains("eval_count")) {
            total_tokens += resp_json["eval_count"].get<int>();
        }
        if (resp_json.contains("prompt_eval_count")) {
            total_tokens += resp_json["prompt_eval_count"].get<int>();
        }
        if (resp_json.contains("usage") && resp_json["usage"].contains("total_tokens")) {
            total_tokens = resp_json["usage"]["total_tokens"].get<int>();
        }
        response.tokens_used = total_tokens;

        return response;
    } catch (const nlohmann::json::parse_error& e) {
        throw JSONParseException(std::string("Failed to parse Ollama response: ") + e.what());
    }
}

} // namespace agent
