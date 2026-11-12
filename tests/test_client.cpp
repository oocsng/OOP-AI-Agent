#include "client/ollama_client.h"
#include "client/mock_llm_client.h"
#include "test_framework.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace agent;

void test_config_and_exceptions() {
    LLMConfig config;
    const char* env_url = std::getenv("LLM_BASE_URL");
    const char* env_model = std::getenv("LLM_MODEL");
    std::string expected_url = (env_url && env_url[0] != '\0') ? env_url : "http://localhost:11434";
    std::string expected_model = (env_model && env_model[0] != '\0') ? env_model : "qwen2.5:7b";

    TEST_ASSERT(config.base_url == expected_url, "Default base_url should match env or be empty");
    TEST_ASSERT(config.model_name == expected_model, "Default model_name should match env or be empty");
    TEST_ASSERT(config.temperature == 0.2, "Default temperature mismatch");
    TEST_ASSERT(config.max_tokens == 2048, "Default max_tokens mismatch");

    // Verify exception hierarchy
    HTTPException http_err(404, "Not Found");
    TEST_ASSERT(http_err.status_code() == 404, "Status code mismatch");
    TEST_ASSERT(std::string(http_err.what()).find("404") != std::string::npos, "Exception message missing 404");

    ConnectionException conn_err("Refused");
    TEST_ASSERT(std::string(conn_err.what()).find("Connection Error") != std::string::npos, "Exception message mismatch");

    JSONParseException json_err("Malformed");
    TEST_ASSERT(std::string(json_err.what()).find("JSON Parse Error") != std::string::npos, "Exception message mismatch");
}

void test_offline_connection_failure() {
    OllamaClient client;
    LLMConfig config;
    config.base_url = "http://localhost:59999"; // Invalid port to ensure offline connection error
    config.model_name = "test_model"; // Ensure model is set so it doesn't fail early
    config.timeout_seconds = 1;

    std::vector<Message> history = {{"user", "Hello", {}}};
    bool caught = false;
    try {
        client.chat(history, config);
    } catch (const ConnectionException& e) {
        caught = true;
    } catch (const LLMException& e) {
        caught = true;
    }
    TEST_ASSERT(caught == true, "Should have caught connection error");
}

void test_base64_encoding() {
    std::string test_file = "temp_test_image.txt";
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ofs << "ABC123Test";
    }

    std::string encoded = OllamaClient::encode_file_base64(test_file);
    TEST_ASSERT(!encoded.empty(), "Base64 string should not be empty");
    TEST_ASSERT(encoded == "QUJDMTIzVGVzdA==", "Base64 string mismatch");

    std::filesystem::remove(test_file);
}

void test_ping_health_check() {
    OllamaClient client;
    bool reached = client.ping("http://localhost:59999", 1);
    TEST_ASSERT(!reached, "Ping to offline/invalid port should return false");
}

void test_mock_llm_client() {
    MockLLMClient mock;
    LLMConfig config;

    // Verify ping returns true
    TEST_ASSERT(mock.ping("http://offline-dummy:1234", 1) == true, "MockLLMClient ping must return true");

    // Test canned response override
    mock.set_canned_response("Canned response 123");
    std::vector<Message> history = {{"user", "hello", {}}};
    LLMResponse resp = mock.chat(history, config);
    TEST_ASSERT(resp.content == "Canned response 123", "Canned response mismatch");
    TEST_ASSERT(resp.tokens_used > 0, "Tokens used should be > 0");

    // Clear canned response
    mock.clear_canned_response();

    // Test automatic formatting - user math prompt
    history = {{"user", "calculate 2 + 2", {}}};
    resp = mock.chat(history, config);
    TEST_ASSERT(resp.content.find("Action: calculator") != std::string::npos, "Math prompt should trigger calculator action");
    TEST_ASSERT(resp.content.find("Action Input:") != std::string::npos, "Math prompt should have Action Input");

    // Test automatic formatting - user file prompt
    history = {{"user", "read file test.txt", {}}};
    resp = mock.chat(history, config);
    TEST_ASSERT(resp.content.find("Action: read_file") != std::string::npos, "File prompt should trigger read_file action");

    // Test automatic formatting - tool observation / result
    history.push_back({.role = "assistant", .content = resp.content, .images = {}});
    history.push_back({.role = "tool", .content = "File contents here", .images = {}});
    resp = mock.chat(history, config);
    TEST_ASSERT(resp.content.find("Final Answer:") != std::string::npos, "Tool result should trigger Final Answer");

    // Test automatic formatting - default response
    history = {{"user", "Tell me a joke", {}}};
    resp = mock.chat(history, config);
    TEST_ASSERT(resp.content.find("Final Answer:") != std::string::npos, "Default prompt should trigger Final Answer");
}

int main() {
    TestSuite suite("LLM CLIENT MODULE");
    suite.run("Config & Exceptions", test_config_and_exceptions);
    suite.run("Offline Connection Failure", test_offline_connection_failure);
    suite.run("Ping Health Check", test_ping_health_check);
    suite.run("Base64 Encoding", test_base64_encoding);
    suite.run("Mock LLM Client", test_mock_llm_client);
    return suite.print_summary();
}
