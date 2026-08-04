#pragma once

#include "tool.h"

namespace agent {

class ExecTool : public Tool {
public:
    std::string name() const override { return "exec_tool"; }
    std::string description() const override { return "Executes shell command and returns stdout/stderr."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"command", {{"type", "string"}, {"description", "Shell command to run"}}}
            }},
            {"required", nlohmann::json::array({"command"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
