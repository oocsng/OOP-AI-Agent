#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace agent {

class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual nlohmann::json parameters_schema() const = 0;
    virtual std::string execute(const nlohmann::json& args) = 0;
};

} // namespace agent
