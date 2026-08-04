#pragma once

#include "tool.h"

namespace agent {

class ReadFileTool : public Tool {
public:
    std::string name() const override { return "read_file"; }
    std::string description() const override { return "Reads content of a file from disk."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"path", {{"type", "string"}, {"description", "File path to read"}}}
            }},
            {"required", nlohmann::json::array({"path"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

class WriteFileTool : public Tool {
public:
    std::string name() const override { return "write_file"; }
    std::string description() const override { return "Writes string content to a file on disk."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"path", {{"type", "string"}, {"description", "File path to write"}}},
                {"content", {{"type", "string"}, {"description", "Content to write"}}}
            }},
            {"required", nlohmann::json::array({"path", "content"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
