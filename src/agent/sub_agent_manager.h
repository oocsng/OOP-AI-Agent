#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>

namespace agent {

class MessageQueue {
public:
    void push(const std::string& msg);
    std::string pop();
    void stop();
    bool empty() const;
private:
    std::queue<std::string> q_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stopped_{false};
};

class SubAgentManager {
public:
    SubAgentManager() = default;
    ~SubAgentManager();

    void spawn_agent(const std::string& agent_id, std::shared_ptr<class AgentLoop> loop, const std::string& task);
    void send_message(const std::string& to_id, const std::string& msg);
    std::string receive_message(const std::string& from_id);
    
    void join_all();

private:
    std::vector<std::jthread> workers_;
    std::unordered_map<std::string, std::shared_ptr<MessageQueue>> inboxes_;
    std::mutex map_mtx_;
};

} // namespace agent
