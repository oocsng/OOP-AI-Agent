#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace agent {

struct Message {
    std::string role; // "system", "user", "assistant", "tool"
    std::string content;
    std::vector<std::string> images; // Base64 encoded images

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Message, role, content, images)
};

} // namespace agent
