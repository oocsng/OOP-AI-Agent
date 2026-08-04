#pragma once

#include "tool.h"

namespace agent {

class SystemInfoTool : public Tool {
public:
    std::string name() const override { return "system_info_tool"; }
    std::string description() const override { return "Returns host operating system and hardware information."; }
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
