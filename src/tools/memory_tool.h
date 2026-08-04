#pragma once

#include "tool.h"
#include <sqlite3.h>
#include <string>

namespace agent {

class MemorySaveTool : public Tool {
private:
    std::string db_path_;
public:
    explicit MemorySaveTool(std::string db_path = "memory.db") : db_path_(std::move(db_path)) {}

    std::string name() const override { return "memory_save"; }
    std::string description() const override { return "Saves a key-value fact into persistent SQLite database."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"key", {{"type", "string"}, {"description", "Fact key or category"}}},
                {"content", {{"type", "string"}, {"description", "Fact detail or value"}}}
            }},
            {"required", nlohmann::json::array({"key", "content"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

class MemorySearchTool : public Tool {
private:
    std::string db_path_;
public:
    explicit MemorySearchTool(std::string db_path = "memory.db") : db_path_(std::move(db_path)) {}

    std::string name() const override { return "memory_search"; }
    std::string description() const override { return "Searches facts in SQLite memory database by query keyword."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Keyword query string to search"}}}
            }},
            {"required", nlohmann::json::array({"query"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
