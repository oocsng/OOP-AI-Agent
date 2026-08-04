#pragma once

#include "tool.h"

namespace agent {

class WebSearchTool : public Tool {
public:
    std::string name() const override { return "web_search"; }
    std::string description() const override { return "Searches DuckDuckGo / web for query results."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Search query string"}}}
            }},
            {"required", nlohmann::json::array({"query"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
