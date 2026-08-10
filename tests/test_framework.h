#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <iomanip>

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #cond + " - " + msg); \
    }

struct TestResult {
    std::string name;
    bool passed;
    double latency_ms;
    std::string error_msg;
};

class TestSuite {
public:
    TestSuite(std::string name) : suite_name(std::move(name)) {}

    template <typename Func>
    void run(const std::string& name, Func f) {
        auto start = std::chrono::high_resolution_clock::now();
        bool passed = true;
        std::string error_msg = "";
        try {
            f();
        } catch (const std::exception& e) {
            passed = false;
            error_msg = e.what();
        } catch (...) {
            passed = false;
            error_msg = "Unknown error";
        }
        auto end = std::chrono::high_resolution_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        results.push_back({name, passed, latency_ms, error_msg});
        if (!passed) {
            all_passed = false;
        }
    }

    int print_summary() {
        std::cout << "==================================================\n";
        std::cout << " UNIT TEST RESULTS: " << std::left << std::setw(30) << suite_name << "\n";
        std::cout << "==================================================\n";

        int passed_count = 0;
        for (const auto& res : results) {
            std::cout << (res.passed ? "\033[32m[PASS]\033[0m" : "\033[31m[FAIL]\033[0m") 
                      << " " << std::left << std::setw(30) << res.name 
                      << " (" << std::fixed << std::setprecision(2) << res.latency_ms << " ms)\n";
            if (res.passed) {
                passed_count++;
            } else {
                std::cout << "       -> Error: " << res.error_msg << "\n";
            }
        }

        std::cout << "--------------------------------------------------\n";
        double success_rate = results.empty() ? 0.0 : (100.0 * passed_count / results.size());
        std::cout << "Summary: " << passed_count << "/" << results.size() 
                  << " Tests PASSED (" << std::fixed << std::setprecision(0) << success_rate << "% Success Rate)\n";
        std::cout << "==================================================\n";
        return all_passed ? 0 : 1;
    }

private:
    std::string suite_name;
    std::vector<TestResult> results;
    bool all_passed = true;
};
