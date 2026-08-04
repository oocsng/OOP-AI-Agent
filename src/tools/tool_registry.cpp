#include "tool_registry.h"
#include <stdexcept>

namespace agent {

void ToolRegistry::register_tool(std::shared_ptr<Tool> tool) {
    if (!tool) return;
    tools_[tool->name()] = tool;
}

static std::string normalize_name(std::string_view name) {
    std::string norm;
    norm.reserve(name.length());
    for (char c : name) {
        if (c == '-') {
            norm.push_back('_');
        } else {
            norm.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return norm;
}

std::shared_ptr<Tool> ToolRegistry::get_tool(const std::string& name) const {
    auto it = tools_.find(name);
    if (it != tools_.end()) {
        return it->second;
    }
    
    std::string norm = normalize_name(name);
    for (const auto& [t_name, tool] : tools_) {
        std::string norm_t = normalize_name(t_name);
        if (norm_t == norm) {
            return tool;
        }
    }

    if (norm.length() > 5 && norm.substr(norm.length() - 5) == "_tool") {
        std::string stripped = norm.substr(0, norm.length() - 5);
        for (const auto& [t_name, tool] : tools_) {
            std::string norm_t = normalize_name(t_name);
            if (norm_t == stripped) {
                return tool;
            }
        }
    }
    return nullptr;
}

bool ToolRegistry::has_tool(const std::string& name) const {
    return get_tool(name) != nullptr;
}

void ToolRegistry::set_allow_list(const std::unordered_set<std::string>& allow_list) {
    allow_list_ = allow_list;
    use_allow_list_ = true;
}

void ToolRegistry::set_deny_list(const std::unordered_set<std::string>& deny_list) {
    deny_list_ = deny_list;
}

void ToolRegistry::clear_policies() {
    allow_list_.clear();
    deny_list_.clear();
    use_allow_list_ = false;
}

bool ToolRegistry::is_tool_allowed(const std::string& tool_name) const {
    if (deny_list_.contains(tool_name)) {
        return false;
    }
    if (use_allow_list_) {
        return allow_list_.contains(tool_name);
    }
    return true;
}

std::string ToolRegistry::execute_tool(const std::string& tool_name, const nlohmann::json& args) {
    auto tool = get_tool(tool_name);
    if (!tool) {
        return "Error: Tool '" + tool_name + "' not found in registry.";
    }
    std::string actual_name = tool->name();
    if (!is_tool_allowed(actual_name)) {
        return "Error: Execution of tool '" + actual_name + "' is denied by safety policy.";
    }
    try {
        return tool->execute(args);
    } catch (const std::exception& e) {
        return "Error executing tool '" + actual_name + "': " + e.what();
    }
}

std::string ToolRegistry::get_tools_schema_json() const {
    nlohmann::json schema_array = nlohmann::json::array();
    for (const auto& [name, tool] : tools_) {
        if (is_tool_allowed(name)) {
            nlohmann::json tool_info;
            tool_info["name"] = tool->name();
            tool_info["description"] = tool->description();
            tool_info["parameters"] = tool->parameters_schema();
            schema_array.push_back(tool_info);
        }
    }
    return schema_array.dump(2);
}

std::vector<std::string> ToolRegistry::get_available_tool_names() const {
    std::vector<std::string> names;
    for (const auto& [name, tool] : tools_) {
        if (is_tool_allowed(name)) {
            names.push_back(name);
        }
    }
    return names;
}

} // namespace agent
