#pragma once

#include "tool.h"

namespace agent {

class RegexExtractorTool : public Tool {
public:
    std::string name() const override { return "regex_extractor_tool"; }
    std::string description() const override { return "Extracts matching regex patterns from target text string."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"text", {{"type", "string"}, {"description", "Text content to search"}}},
                {"pattern", {{"type", "string"}, {"description", "Regex pattern to match"}}}
            }},
            {"required", nlohmann::json::array({"text", "pattern"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
