#include "agent/sub_agent_manager.h"
#include "agent/agent_loop.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>

using namespace agent;

void test_thread_safe_message_queue() {
    MessageQueue queue;
    assert(queue.empty());

    const int num_producers = 4;
    const int msgs_per_producer = 25;
    std::vector<std::thread> producers;

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&queue, i, msgs_per_producer]() {
            for (int j = 0; j < msgs_per_producer; ++j) {
                queue.push("msg_" + std::to_string(i) + "_" + std::to_string(j));
            }
        });
    }

    int received_count = 0;
    std::vector<std::string> received_messages;
    std::mutex res_mtx;

    std::thread consumer([&queue, &received_count, &received_messages, &res_mtx, total = num_producers * msgs_per_producer]() {
        for (int i = 0; i < total; ++i) {
            std::string msg = queue.pop();
            std::lock_guard<std::mutex> lock(res_mtx);
            received_messages.push_back(msg);
            received_count++;
        }
    });

    for (auto& t : producers) {
        t.join();
    }
    consumer.join();

    assert(received_count == num_producers * msgs_per_producer);
    assert(queue.empty());
}

void test_sub_agent_manager_messaging() {
    SubAgentManager manager;

    manager.send_message("agent_b", "Hello Agent B from main");
    std::string msg = manager.receive_message("agent_b");
    assert(msg == "Hello Agent B from main");

    // Test spawning thread
    bool thread_ran = false;
    std::thread t([&manager, &thread_ran]() {
        manager.send_message("worker_inbox", "Task Completed");
        thread_ran = true;
    });
    t.join();

    assert(thread_ran);
    std::string worker_msg = manager.receive_message("worker_inbox");
    assert(worker_msg == "Task Completed");
}

int main() {
    std::cout << "[RUNNING] test_bonus_multi_agent..." << std::endl;
    test_thread_safe_message_queue();
    test_sub_agent_manager_messaging();
    std::cout << "[PASSED] test_bonus_multi_agent" << std::endl;
    return 0;
}
