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
VectorMemoryTool::~VectorMemoryTool() { if (db_) sqlite3_close(db_); }

void VectorMemoryTool::init_db() {
    if (sqlite3_open("vector_memory.db", &db_) == SQLITE_OK) {
        const char* sql = "CREATE TABLE IF NOT EXISTS embeddings (id INTEGER PRIMARY KEY, text TEXT, vector_json TEXT);";
        sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    }
}

std::vector<float> VectorMemoryTool::generate_embedding(const std::string& text) const {
    std::vector<float> embed;
    CURL* curl = curl_easy_init();
    if (curl) {
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
        curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
        
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

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

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
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
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
    return dot / (std::sqrt(denom_a) * std::sqrt(denom_b));
}

std::string VectorMemoryTool::execute(const nlohmann::json& args) {
    if (!args.contains("action") || !args.contains("text")) return "Error: missing arguments";
    std::string action = args["action"];
    std::string text = args["text"];

    if (action == "save") {
        auto emb = generate_embedding(text);
        if (emb.empty()) return "Error: failed to generate embedding";
        
        nlohmann::json e_json = emb;
        std::string sql = "INSERT INTO embeddings (text, vector_json) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);
            std::string ej_str = e_json.dump();
            sqlite3_bind_text(stmt, 2, ej_str.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return "Saved semantic memory successfully.";
        }
        return "Database error.";
    } else if (action == "search") {
        auto query_emb = generate_embedding(text);
        if (query_emb.empty()) return "Error: embedding failed";
        
        std::string sql = "SELECT text, vector_json FROM embeddings;";
        sqlite3_stmt* stmt;
        std::string best_match = "";
        double best_score = -1.0;
        
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::string mem_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                std::string mem_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
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
            sqlite3_finalize(stmt);
        }
        if (best_score > 0.6) {
            return "Best match (score " + std::to_string(best_score) + "): " + best_match;
        }
        return "No relevant memory found.";
    }
    return "Unknown action.";
}

} // namespace agent
