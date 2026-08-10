#pragma once

#include <concepts>
#include <variant>
#include <optional>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <nlohmann/json.hpp>
#include "tools/tool.h"

namespace agent::cpp20 {

// C++20 Concept to constrain Tool types in templates
template <typename T>
concept ValidTool = std::derived_from<T, Tool> && requires(T t, nlohmann::json j) {
    { t.name() } -> std::same_as<std::string>;
    { t.description() } -> std::same_as<std::string>;
    { t.execute(j) } -> std::same_as<std::string>;
};

} // namespace agent::cpp20

namespace agent::cpp17 {

// C++17 std::variant and std::visit for Action result representation
using ActionResult = std::variant<std::string, nlohmann::json, std::monostate>;

inline std::string format_action_result(const ActionResult& result) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, nlohmann::json>) {
            return arg.dump();
        } else {
            return "No result";
        }
    }, result);
}

// C++17 std::optional<T> helper
inline std::optional<std::string> safe_get_env(const char* env_name) {
    if (const char* val = std::getenv(env_name)) {
        return std::string(val);
    }
    return std::nullopt;
}

} // namespace agent::cpp17

namespace agent::cpp26 {

// C++26 static_assert without message (Wait, C++17 did that. C++26 adds = delete("reason"))
void deleted_function() = delete("This function is intentionally deleted to demonstrate a C++26 feature");

} // namespace agent::cpp26
