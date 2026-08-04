#pragma once

#include "tool.h"

namespace agent {

class CalculatorTool : public Tool {
public:
    std::string name() const override { return "calculator"; }
    std::string description() const override { return "Evaluates arithmetic expression (supports +, -, *, /, ^, parentheses)."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Arithmetic expression to evaluate, e.g. '(3 + 4) * 2^3'"}}}
            }},
            {"required", nlohmann::json::array({"expression"})}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
