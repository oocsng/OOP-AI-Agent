#pragma once

#include <concepts>
#include <variant>
#include <optional>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <numeric>
#include <limits>
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

// Forward declarations
template <std::integral T>
constexpr T safe_add(T a, T b) noexcept;

template <std::integral T>
constexpr T safe_sub(T a, T b) noexcept;

template <std::integral T, std::integral U>
requires (!std::is_same_v<T, U>)
constexpr T safe_add(T a, U b) noexcept;

template <std::integral T, std::integral U>
requires (!std::is_same_v<T, U>)
constexpr T safe_sub(T a, U b) noexcept;

// C++26 saturation arithmetic with clamped fallback
template <std::integral T>
constexpr T safe_add(T a, T b) noexcept {
#if defined(__glibcxx_saturation_arithmetic)
    return std::saturating_add(a, b);
#elif defined(__cpp_lib_saturation_arithmetic)
    return std::add_sat(a, b);
#else
    if constexpr (std::is_unsigned_v<T>) {
        if (std::numeric_limits<T>::max() - a < b) {
            return std::numeric_limits<T>::max();
        }
        return a + b;
    } else {
        if (b > 0 && a > std::numeric_limits<T>::max() - b) {
            return std::numeric_limits<T>::max();
        }
        if (b < 0 && a < std::numeric_limits<T>::min() - b) {
            return std::numeric_limits<T>::min();
        }
        return a + b;
    }
#endif
}

template <std::integral T>
constexpr T safe_sub(T a, T b) noexcept {
#if defined(__glibcxx_saturation_arithmetic)
    return std::saturating_sub(a, b);
#elif defined(__cpp_lib_saturation_arithmetic)
    return std::sub_sat(a, b);
#else
    if constexpr (std::is_unsigned_v<T>) {
        if (a < b) {
            return 0;
        }
        return a - b;
    } else {
        if (b < 0 && a > std::numeric_limits<T>::max() + b) {
            return std::numeric_limits<T>::max();
        }
        if (b > 0 && a < std::numeric_limits<T>::min() + b) {
            return std::numeric_limits<T>::min();
        }
        return a - b;
    }
#endif
}

// Overload for mixed integral types (e.g., std::size_t total_tokens + int tokens_used)
template <std::integral T, std::integral U>
requires (!std::is_same_v<T, U>)
constexpr T safe_add(T a, U b) noexcept {
    if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>) {
        if (b < 0) {
            using UnsignedU = std::make_unsigned_t<U>;
            UnsignedU abs_b = static_cast<UnsignedU>(-(b + 1)) + 1;
            if constexpr (sizeof(T) >= sizeof(UnsignedU)) {
                return safe_sub<T>(a, static_cast<T>(abs_b));
            } else {
                if (abs_b > static_cast<UnsignedU>(a)) return 0;
                return static_cast<T>(static_cast<UnsignedU>(a) - abs_b);
            }
        } else {
            using UnsignedU = std::make_unsigned_t<U>;
            UnsignedU ub = static_cast<UnsignedU>(b);
            if constexpr (sizeof(T) >= sizeof(UnsignedU)) {
                return safe_add<T>(a, static_cast<T>(ub));
            } else {
                UnsignedU res = safe_add<UnsignedU>(static_cast<UnsignedU>(a), ub);
                if (res > static_cast<UnsignedU>(std::numeric_limits<T>::max())) {
                    return std::numeric_limits<T>::max();
                }
                return static_cast<T>(res);
            }
        }
    } else if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>) {
        if (b > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max())) {
            return std::numeric_limits<T>::max();
        }
        return safe_add<T>(a, static_cast<T>(b));
    } else {
        if constexpr (sizeof(T) >= sizeof(U)) {
            return safe_add<T>(a, static_cast<T>(b));
        } else {
            auto res = safe_add<U>(static_cast<U>(a), b);
            if (res > static_cast<U>(std::numeric_limits<T>::max())) {
                return std::numeric_limits<T>::max();
            }
            if constexpr (std::is_signed_v<T>) {
                if (res < static_cast<U>(std::numeric_limits<T>::min())) {
                    return std::numeric_limits<T>::min();
                }
            }
            return static_cast<T>(res);
        }
    }
}

template <std::integral T, std::integral U>
requires (!std::is_same_v<T, U>)
constexpr T safe_sub(T a, U b) noexcept {
    if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>) {
        if (b < 0) {
            using UnsignedU = std::make_unsigned_t<U>;
            UnsignedU abs_b = static_cast<UnsignedU>(-(b + 1)) + 1;
            return safe_add<T>(a, static_cast<T>(abs_b));
        } else {
            using UnsignedU = std::make_unsigned_t<U>;
            return safe_sub<T>(a, static_cast<T>(static_cast<UnsignedU>(b)));
        }
    } else if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>) {
        if (b > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max())) {
            return std::numeric_limits<T>::min();
        }
        return safe_sub<T>(a, static_cast<T>(b));
    } else {
        if constexpr (sizeof(T) >= sizeof(U)) {
            return safe_sub<T>(a, static_cast<T>(b));
        } else {
            auto res = safe_sub<U>(static_cast<U>(a), b);
            if (res > static_cast<U>(std::numeric_limits<T>::max())) {
                return std::numeric_limits<T>::max();
            }
            if constexpr (std::is_signed_v<T>) {
                if (res < static_cast<U>(std::numeric_limits<T>::min())) {
                    return std::numeric_limits<T>::min();
                }
            }
            return static_cast<T>(res);
        }
    }
}

// C++26 static_assert without message (Wait, C++17 did that. C++26 adds = delete("reason"))
void deleted_function() = delete("This function is intentionally deleted to demonstrate a C++26 feature");

} // namespace agent::cpp26
