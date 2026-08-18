#include "memory_tool.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace agent {

static void init_db_schema(sqlite3* db) {
    const char* sql = "CREATE TABLE IF NOT EXISTS memory ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "key TEXT NOT NULL, "
                      "content TEXT NOT NULL, "
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char* err_msg = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
    if (err_msg) {
        sqlite3_free(err_msg);
    }
}

std::string MemorySaveTool::execute(const nlohmann::json& args) {
    if (!args.contains("key") || !args["key"].is_string() ||
        !args.contains("content") || !args["content"].is_string()) {
        return "Error: Missing string arguments 'key' and 'content'.";
    }

    sqlite3* raw_db = nullptr;
    if (sqlite3_open(db_path_.c_str(), &raw_db) != SQLITE_OK) {
        std::string err = raw_db ? sqlite3_errmsg(raw_db) : "Unknown error";
        if (raw_db) sqlite3_close(raw_db);
        return "Error opening SQLite database: " + err;
    }
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(raw_db, sqlite3_close);

    init_db_schema(db.get());

    std::string key = args["key"].get<std::string>();
    std::string content = args["content"].get<std::string>();

    const char* sql = "INSERT INTO memory (key, content) VALUES (?, ?);";
    sqlite3_stmt* raw_stmt = nullptr;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw_stmt, nullptr) != SQLITE_OK) {
        return "Error preparing SQLite statement: " + std::string(sqlite3_errmsg(db.get()));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw_stmt, sqlite3_finalize);

    sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, content.c_str(), -1, SQLITE_TRANSIENT);

    int step_res = sqlite3_step(stmt.get());

    if (step_res != SQLITE_DONE) {
        return "Error inserting fact into SQLite memory database.";
    }

    return "Successfully saved fact into memory: [" + key + "] -> " + content;
}

std::string MemorySearchTool::execute(const nlohmann::json& args) {
    if (!args.contains("query") || !args["query"].is_string()) {
        return "Error: Missing string argument 'query'.";
    }

    sqlite3* raw_db = nullptr;
    if (sqlite3_open(db_path_.c_str(), &raw_db) != SQLITE_OK) {
        std::string err = raw_db ? sqlite3_errmsg(raw_db) : "Unknown error";
        if (raw_db) sqlite3_close(raw_db);
        return "Error opening SQLite database: " + err;
    }
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(raw_db, sqlite3_close);

    init_db_schema(db.get());

    std::string query = "%" + args["query"].get<std::string>() + "%";
    const char* sql = "SELECT key, content, timestamp FROM memory WHERE key LIKE ? OR content LIKE ? ORDER BY id DESC LIMIT 10;";
    sqlite3_stmt* raw_stmt = nullptr;

    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw_stmt, nullptr) != SQLITE_OK) {
        return "Error preparing SQLite search statement: " + std::string(sqlite3_errmsg(db.get()));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(raw_stmt, sqlite3_finalize);

    sqlite3_bind_text(stmt.get(), 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, query.c_str(), -1, SQLITE_TRANSIENT);

    nlohmann::json results = nlohmann::json::array();
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));

        nlohmann::json item;
        item["key"] = key ? key : "";
        item["content"] = content ? content : "";
        item["timestamp"] = ts ? ts : "";
        results.push_back(item);
    }

    if (results.empty()) {
        return "No memory records found matching query.";
    }

    return results.dump(2);
}

} // namespace agent
