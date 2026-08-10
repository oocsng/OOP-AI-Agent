#pragma once

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace agent::utils {

inline std::string get_iso8601_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    now_c += 7 * 3600; // GMT+7
    std::tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &now_c);
#else
    gmtime_r(&now_c, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S+07:00");
    return ss.str();
}

inline std::string get_local_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    now_c += 7 * 3600; // GMT+7
    std::tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &now_c);
#else
    gmtime_r(&now_c, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

template<typename TimePoint>
inline double get_formatted_duration_ms(TimePoint start_time, TimePoint end_time) {
    return std::chrono::duration<double, std::milli>(end_time - start_time).count();
}

} // namespace agent::utils
