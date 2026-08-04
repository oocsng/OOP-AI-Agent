#include "exec_tool.h"
#include <cstdio>
#include <memory>
#include <array>

namespace agent {

std::string ExecTool::execute(const nlohmann::json& args) {
    if (!args.contains("command") || !args["command"].is_string()) {
        return "Error: Missing string argument 'command'.";
    }

    std::string command = args["command"].get<std::string>();
    std::string result;
    std::array<char, 256> buffer;

#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        return "Error: Failed to open process pipe for command execution.";
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }

#if defined(_WIN32)
    int exit_code = _pclose(pipe);
#else
    int exit_code = pclose(pipe);
#endif

    if (result.empty()) {
        result = "[Command completed with exit code " + std::to_string(exit_code) + "]";
    }
    return result;
}

} // namespace agent
