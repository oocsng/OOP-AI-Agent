#include "agent/loop_detector.h"
#include "test_framework.h"
#include <iostream>

using namespace agent;

void test_generic_repeat_loop() {
    LoopDetector detector(/*repeat_warning=*/2, /*repeat_stop=*/3, /*pingpong_stop=*/2);

    nlohmann::json args = {{"path", "file.txt"}};

    // Call 1: Normal
    auto r1 = detector.record_and_check("read_file", args);
    TEST_ASSERT(r1.status == LoopStatus::Normal, "Call 1 should be normal");

    // Call 2: Repeat Warning
    auto r2 = detector.record_and_check("read_file", args);
    TEST_ASSERT(r2.status == LoopStatus::Warning, "Call 2 should be warning");

    // Call 3: Repeat Critical Stop
    auto r3 = detector.record_and_check("read_file", args);
    TEST_ASSERT(r3.status == LoopStatus::CriticalStop, "Call 3 should be critical stop");
}

void test_ping_pong_loop() {
    LoopDetector detector(/*repeat_warning=*/5, /*repeat_stop=*/10, /*pingpong_stop=*/2);

    nlohmann::json args_a = {{"query", "search1"}};
    nlohmann::json args_b = {{"path", "out.txt"}};

    // Pair 1: Tool A, Tool B
    auto r1 = detector.record_and_check("web_search", args_a);
    TEST_ASSERT(r1.status == LoopStatus::Normal, "Pair 1 web_search should be normal");

    auto r2 = detector.record_and_check("write_file", args_b);
    TEST_ASSERT(r2.status == LoopStatus::Normal, "Pair 1 write_file should be normal");

    // Pair 2: Tool A, Tool B
    auto r3 = detector.record_and_check("web_search", args_a);
    TEST_ASSERT(r3.status == LoopStatus::Normal, "Pair 2 web_search should be normal");

    auto r4 = detector.record_and_check("write_file", args_b);
    TEST_ASSERT(r4.status == LoopStatus::CriticalStop, "Pair 2 write_file should be critical stop");
    TEST_ASSERT(r4.message.find("Ping-Pong") != std::string::npos, "Ping-Pong message missing");
}

void test_reset_and_thresholds() {
    LoopDetector detector(/*repeat_warning=*/2, /*repeat_stop=*/3, /*pingpong_stop=*/2);
    nlohmann::json args = {{"x", 1}};

    detector.record_and_check("tool", args);
    detector.record_and_check("tool", args);
    TEST_ASSERT(detector.get_history().size() == 2, "History size should be 2");

    detector.reset();
    TEST_ASSERT(detector.get_history().empty(), "History should be empty after reset");

    auto r_after_reset = detector.record_and_check("tool", args);
    TEST_ASSERT(r_after_reset.status == LoopStatus::Normal, "After reset, call should be normal");
}

int main() {
    TestSuite suite("LOOP DETECTOR MODULE");
    suite.run("Generic Repeat Loop", test_generic_repeat_loop);
    suite.run("Ping-Pong Loop", test_ping_pong_loop);
    suite.run("Reset & Thresholds", test_reset_and_thresholds);
    return suite.print_summary();
}
