#include "custom_tool1.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace agent {

std::string DateTimeTool::execute(const nlohmann::json& /*args*/) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    struct tm timeinfo{};
#if defined(_WIN32)
    localtime_s(&timeinfo, &now_c);
#else
    localtime_r(&now_c, &timeinfo);
#endif
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");

    nlohmann::json res;
    res["timestamp"] = ss.str();
    res["unix_epoch"] = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return res.dump(2);
}

} // namespace agent
