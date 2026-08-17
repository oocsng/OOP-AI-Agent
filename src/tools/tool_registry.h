#pragma once

#include "tool.h"
#include "registry.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>

namespace agent {

// ToolRegistry inherits generic Registry<Tool> (satisfies Table V generic template class requirement)
class ToolRegistry : public Registry<Tool> {
public:
    ToolRegistry() = default;

    // Convenience alias matching old API
    void register_tool(std::shared_ptr<Tool> tool) { register_item(std::move(tool)); }
    [[nodiscard]] std::shared_ptr<Tool> get_tool(const std::string& name) const;
    [[nodiscard]] bool has_tool(const std::string& name) const;

    void set_allow_list(const std::unordered_set<std::string>& allow_list);
    void set_deny_list(const std::unordered_set<std::string>& deny_list);
    void clear_policies();

    [[nodiscard]] bool is_tool_allowed(const std::string& tool_name) const;

    std::string execute_tool(const std::string& tool_name, const nlohmann::json& args);

    [[nodiscard]] std::string get_tools_schema_json() const;
    [[nodiscard]] std::vector<std::string> get_available_tool_names() const;
};

} // namespace agent
