#pragma once

#include "tool.h"

namespace agent {

class DateTimeTool : public Tool {
public:
    std::string name() const override { return "get_system_time"; }
    std::string description() const override { return "Returns current system date, time, and timezone."; }
    nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {}},
            {"required", nlohmann::json::array()}
        };
    }
    std::string execute(const nlohmann::json& args) override;
};

} // namespace agent
