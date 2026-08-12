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
    
    // Force inject -NoProfile for PowerShell
    size_t pos;
    while ((pos = command.find("powershell.exe ")) != std::string::npos) {
        if (command.find("powershell.exe -NoProfile ") != pos) {
            command.replace(pos, 15, "powershell.exe -NoProfile ");
        } else {
            break;
        }
    }
    while ((pos = command.find("powershell ")) != std::string::npos) {
        command.replace(pos, 11, "powershell.exe -NoProfile ");
    }

    std::string result;
    std::array<char, 256> buffer;

#if defined(_WIN32)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        return "Error: Failed to open process pipe for command execution.";
    }

    try {
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    } catch (...) {
        return "Error: Exception occurred while reading command output.";
    }

    int exit_code = -1;
#if defined(_WIN32)
    exit_code = _pclose(pipe.release());
#else
    exit_code = pclose(pipe.release());
#endif

    if (result.empty()) {
        result = "[Command completed with exit code " + std::to_string(exit_code) + "]";
    }
    return result;
}

} // namespace agent
