#include "tools/gui_tool.h"
#include "tools/tool_registry.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace agent;

void test_base64_screenshot_encoding() {
    std::vector<unsigned char> raw_pixels = {'A', 'B', 'C', 'D', 'E', 'F'};
    std::string b64 = GuiTool::encode_base64(raw_pixels);
    assert(!b64.empty());
    assert(b64 == "QUJDREVGI==" || b64 == "QUJDREVG");
}

void test_gui_action_parsing_and_execution() {
    GuiTool gui;
    assert(gui.name() == "gui_tool");
    assert(!gui.description().empty());

    // Test missing action
    std::string err_res = gui.execute({});
    assert(err_res.find("Error") != std::string::npos);

    // Test click execution / parsing
    nlohmann::json click_args = {{"action", "click"}, {"x", 100}, {"y", 200}};
    std::string click_res = gui.execute(click_args);
    assert(click_res.find("Clicked") != std::string::npos || click_res.find("executed") != std::string::npos);

    // Test type execution / parsing
    nlohmann::json type_args = {{"action", "type"}, {"text", "Hello GUI"}};
    std::string type_res = gui.execute(type_args);
    assert(type_res.find("Typed") != std::string::npos || type_res.find("executed") != std::string::npos);

#ifdef _WIN32
    nlohmann::json bad_shot = {
        {"action", "screenshot"},
        {"path", "outputs/screen captures/::invalid_filename::.bmp"}
    };
    std::string bad_shot_res = gui.execute(bad_shot);
    assert(bad_shot_res.find("Error:") != std::string::npos);
#endif
}

void test_gui_tool_registration() {
    ToolRegistry registry;
    auto gui = std::make_shared<GuiTool>();
    registry.register_tool(gui);

    assert(registry.has_tool("gui_tool"));
    assert(registry.get_tool("gui_tool") != nullptr);
    std::string schema = registry.get_tools_schema_json();
    assert(schema.find("gui_tool") != std::string::npos);
}

int main() {
    std::cout << "[RUNNING] test_bonus_gui..." << std::endl;
    test_base64_screenshot_encoding();
    test_gui_action_parsing_and_execution();
    test_gui_tool_registration();
    std::cout << "[PASSED] test_bonus_gui" << std::endl;
    return 0;
}
