#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace agent {

// C++20 concept: T must provide a name() method returning std::string
template <typename T>
concept Named = requires(T t) {
    { t.name() } -> std::same_as<std::string>;
};

// Generic Registry<T> template class for named items with allow/deny policy
// ToolRegistry inherits this to satisfy Table V generic template class requirement
template <typename T>
    requires Named<T>
class Registry {
protected:
    std::unordered_map<std::string, std::shared_ptr<T>> items_;
    std::unordered_set<std::string> allow_list_;
    std::unordered_set<std::string> deny_list_;
    bool use_allow_list_{false};

public:
    Registry() = default;
    virtual ~Registry() = default;

    void register_item(std::shared_ptr<T> item) {
        if (!item) return;
        items_[item->name()] = item;
    }

    [[nodiscard]] std::shared_ptr<T> get_item(const std::string& name) const {
        auto it = items_.find(name);
        if (it != items_.end()) return it->second;
        return nullptr;
    }

    [[nodiscard]] bool has_item(const std::string& name) const {
        return items_.contains(name);
    }

    void set_allow_list(const std::unordered_set<std::string>& allow_list) {
        allow_list_ = allow_list;
        use_allow_list_ = true;
    }

    void set_deny_list(const std::unordered_set<std::string>& deny_list) {
        deny_list_ = deny_list;
    }

    void clear_policies() {
        allow_list_.clear();
        deny_list_.clear();
        use_allow_list_ = false;
    }

    [[nodiscard]] bool is_allowed(const std::string& name) const {
        if (deny_list_.contains(name)) return false;
        if (use_allow_list_) return allow_list_.contains(name);
        return true;
    }

    [[nodiscard]] std::vector<std::string> get_item_names() const {
        std::vector<std::string> names;
        names.reserve(items_.size());
        for (const auto& [name, _] : items_) {
            if (is_allowed(name)) names.push_back(name);
        }
        return names;
    }
};

} // namespace agent
