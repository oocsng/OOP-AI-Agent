#include "vector_memory_tool.h"
#include <curl/curl.h>
#include <iostream>
#include <cmath>

namespace agent {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

VectorMemoryTool::VectorMemoryTool() { init_db(); }
VectorMemoryTool::~VectorMemoryTool() = default;

void VectorMemoryTool::init_db() {
    sqlite3* raw_db = nullptr;
    if (sqlite3_open("vector_memory.db", &raw_db) == SQLITE_OK && raw_db != nullptr) {
        db_.reset(raw_db);
        const char* sql = "CREATE TABLE IF NOT EXISTS embeddings (id INTEGER PRIMARY KEY, text TEXT, vector_json TEXT);";
        sqlite3_exec(db_.get(), sql, nullptr, nullptr, nullptr);
    } else if (raw_db != nullptr) {
        sqlite3_close(raw_db);
    }
}

std::vector<float> VectorMemoryTool::generate_embedding(const std::string& text) const {
    std::vector<float> embed;
    CURL* raw_curl = curl_easy_init();
    if (!raw_curl) return embed;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(raw_curl, curl_easy_cleanup);

    std::string response_string;
    const char* env_url = std::getenv("EMBEDDING_BASE_URL");
    if (!env_url || env_url[0] == '\0') {
        env_url = std::getenv("LLM_BASE_URL");
        if (!env_url || env_url[0] == '\0') {
            env_url = "http://localhost:11434";
        }
        std::cerr << "[WARN] EMBEDDING_BASE_URL is not set. Defaulting to: " << env_url << "\n";
        std::cerr << "[INFO] To override, run: $env:EMBEDDING_BASE_URL=\"https://your-ngrok-url.com\"\n";
    }
    std::string base_url = env_url;
    if (base_url.back() == '/') base_url.pop_back();
    if (!base_url.ends_with("/api/embeddings") && !base_url.ends_with("/v1/embeddings")) {
        base_url += "/api/embeddings";
    }
    curl_easy_setopt(curl.get(), CURLOPT_URL, base_url.c_str());
    
    const char* env_model = std::getenv("EMBEDDING_MODEL");
    if (!env_model || env_model[0] == '\0') {
        std::cerr << "[WARN] EMBEDDING_MODEL is not set. Defaulting to: nomic-embed-text\n";
        std::cerr << "[INFO] To override, run: $env:EMBEDDING_MODEL=\"your-model-name\"\n";
        env_model = "nomic-embed-text";
    }
    std::string model_name = env_model;

    nlohmann::json payload;
    payload["model"] = model_name;
    payload["prompt"] = text;
    std::string json_str = payload.dump();

    struct curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw_headers, curl_slist_free_all);

    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl.get());
    long http_code = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        std::cerr << "[VectorMemory] ERROR: LLM server is offline or unreachable. Details: " << curl_easy_strerror(res) << "\n";
    } else if (http_code != 200) {
        std::cerr << "[VectorMemory] ERROR: Failed to get embeddings. HTTP Code: " << http_code << "\n";
    } else {
        try {
            auto j = nlohmann::json::parse(response_string);
            if (j.contains("embedding")) {
                for (auto& val : j["embedding"]) {
                    embed.push_back(val.get<float>());
                }
            }
        } catch (...) {}
    }
    return embed;
}

double VectorMemoryTool::cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || a.size() != b.size()) return 0.0;
    double dot = 0.0, denom_a = 0.0, denom_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        denom_a += a[i] * a[i];
        denom_b += b[i] * b[i];
    }
    double denom = std::sqrt(denom_a) * std::sqrt(denom_b);
    if (denom == 0.0) return 0.0;
    return dot / denom;
}

std::string VectorMemoryTool::execute(const nlohmann::json& args) {
    if (!args.contains("action") || !args.contains("text")) return "Error: missing arguments";
    if (!db_) return "Error: Database not initialized.";
    std::string action = args["action"];
    std::string text = args["text"];

    if (action == "save") {
        auto emb = generate_embedding(text);
        if (emb.empty()) return "Error: failed to generate embedding";
        
        nlohmann::json e_json = emb;
        std::string sql = "INSERT INTO embeddings (text, vector_json) VALUES (?, ?);";
        sqlite3_stmt* raw_stmt = nullptr;
        if (sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &raw_stmt, nullptr) == SQLITE_OK) {
            std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw_stmt, sqlite3_finalize);
            sqlite3_bind_text(stmt.get(), 1, text.c_str(), -1, SQLITE_TRANSIENT);
            std::string ej_str = e_json.dump();
            sqlite3_bind_text(stmt.get(), 2, ej_str.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
                return "Error: Database insert failed.";
            }
            return "Saved semantic memory successfully.";
        }
        return "Database error.";
    } else if (action == "search") {
        auto query_emb = generate_embedding(text);
        if (query_emb.empty()) return "Error: embedding failed";
        
        std::string sql = "SELECT text, vector_json FROM embeddings;";
        sqlite3_stmt* raw_stmt = nullptr;
        std::string best_match = "";
        double best_score = -1.0;
        
        if (sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &raw_stmt, nullptr) == SQLITE_OK) {
            std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw_stmt, sqlite3_finalize);
            while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
                const char* raw_col0 = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
                const char* raw_col1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
                std::string mem_text = raw_col0 ? raw_col0 : "";
                std::string mem_json = raw_col1 ? raw_col1 : "";
                try {
                    auto j = nlohmann::json::parse(mem_json);
                    std::vector<float> mem_emb;
                    for (auto& val : j) mem_emb.push_back(val.get<float>());
                    
                    double score = cosine_similarity(query_emb, mem_emb);
                    if (score > best_score) {
                        best_score = score;
                        best_match = mem_text;
                    }
                } catch(...) {}
            }
        }
        if (best_score > 0.6) {
            return "Best match (score " + std::to_string(best_score) + "): " + best_match;
        }
        return "No relevant memory found.";
    }
    return "Unknown action.";
}

} // namespace agent
