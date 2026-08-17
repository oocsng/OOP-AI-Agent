#pragma once

#include <string>
#include <filesystem>

namespace agent::harness {

// Abstract base for task execution environment setup/teardown (Strategy Pattern)
class Environment {
public:
    virtual ~Environment() = default;
    virtual void setup() = 0;
    virtual void teardown() = 0;
    [[nodiscard]] virtual std::string get_working_directory() const = 0;
};

// Runs tasks directly in the host workspace without isolation
class NativeEnvironment : public Environment {
public:
    void setup() override {}
    void teardown() override {}
    [[nodiscard]] std::string get_working_directory() const override {
        return std::filesystem::current_path().string();
    }
};

// Runs tasks inside an isolated temporary sandbox directory
class SandboxEnvironment : public Environment {
public:
    explicit SandboxEnvironment(std::filesystem::path sandbox_dir = "sandbox_tmp")
        : sandbox_dir_(std::move(sandbox_dir)) {}

    void setup() override {
        std::filesystem::create_directories(sandbox_dir_);
    }

    void teardown() override {
        std::error_code ec;
        std::filesystem::remove_all(sandbox_dir_, ec);
    }

    [[nodiscard]] std::string get_working_directory() const override {
        return sandbox_dir_.string();
    }

private:
    std::filesystem::path sandbox_dir_;
};

} // namespace agent::harness
