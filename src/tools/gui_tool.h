#pragma once
#include "tool.h"
#include <string>
#include <vector>

namespace agent {

class GuiTool : public Tool {
public:
    [[nodiscard]] std::string name() const override { return "gui_tool"; }
    [[nodiscard]] std::string description() const override {
        return "Perform GUI automation actions: action='screenshot', action='click' (x, y), action='type' (text), action='key' (key: 'enter', 'ctrl+c', 'ctrl+a'), action='sleep' (ms), action='clipboard'";
    }
    std::string execute(const nlohmann::json& args) override;
    [[nodiscard]] nlohmann::json parameters_schema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"action", {
                    {"type", "string"},
                    {"enum", nlohmann::json::array({"screenshot", "click", "type", "key", "sleep", "clipboard"})},
                    {"description", "Required GUI action: 'screenshot', 'click', 'type', 'key', 'sleep', 'clipboard'"}
                }},
                {"x", {{"type", "integer"}, {"description", "X pixel coordinate for click action"}}},
                {"y", {{"type", "integer"}, {"description", "Y pixel coordinate for click action"}}},
                {"text", {{"type", "string"}, {"description", "Text string to type"}}},
                {"key", {{"type", "string"}, {"description", "Key name to press, e.g. 'enter', 'ctrl+c', 'ctrl+a'"}}},
                {"ms", {{"type", "integer"}, {"description", "Milliseconds to sleep"}}}
            }},
            {"required", nlohmann::json::array({"action"})}
        };
    }

public:
    static std::string encode_base64(const std::vector<unsigned char>& data);
    static std::string get_clipboard_text();

private:
    std::string capture_screenshot(const std::string& output_path);
};

} // namespace agent
