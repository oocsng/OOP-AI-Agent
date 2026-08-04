#include "custom_tool2.h"
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace agent {

std::string SystemInfoTool::execute(const nlohmann::json& /*args*/) {
    nlohmann::json info;

#if defined(_WIN32)
    info["os"] = "Windows";
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info["total_ram_mb"] = memInfo.ullTotalPhys / (1024 * 1024);
        info["avail_ram_mb"] = memInfo.ullAvailPhys / (1024 * 1024);
    }
#elif defined(__linux__)
    info["os"] = "Linux";
#elif defined(__APPLE__)
    info["os"] = "macOS";
#else
    info["os"] = "Unknown";
#endif

    info["cpu_threads"] = std::thread::hardware_concurrency();
    return info.dump(2);
}

} // namespace agent
