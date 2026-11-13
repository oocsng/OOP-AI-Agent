#include "sub_agent_manager.h"
#include "agent_loop.h"
#include <iostream>

namespace agent {

void MessageQueue::push(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    q_.push(msg);
    cv_.notify_one();
}

std::string MessageQueue::pop() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return !q_.empty() || stopped_; });
    if (q_.empty()) return "";
    std::string val = q_.front();
    q_.pop();
    return val;
}

bool MessageQueue::empty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return q_.empty();
}

void MessageQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        stopped_ = true;
    }
    cv_.notify_all();
}

SubAgentManager::~SubAgentManager() {
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        for (auto& pair : inboxes_) {
            pair.second->stop();
        }
    }
    join_all();
}

void SubAgentManager::spawn_agent(const std::string& agent_id, std::shared_ptr<AgentLoop> loop, const std::string& task) {
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        if (inboxes_.find(agent_id) == inboxes_.end()) {
            inboxes_[agent_id] = std::make_shared<MessageQueue>();
        }
    }
    
    workers_.emplace_back([this, agent_id, loop, task](std::stop_token /*stoken*/) {
        // Run the agent loop in background
        // Real implementation would pass a custom config or allow passing messages mid-task
        LLMConfig config; 
        try {
            loop->run(agent_id, task, config);
        } catch(...) {
            // handle error
        }
    });
}

void SubAgentManager::send_message(const std::string& to_id, const std::string& msg) {
    std::lock_guard<std::mutex> lock(map_mtx_);
    if (inboxes_.find(to_id) == inboxes_.end()) {
        inboxes_[to_id] = std::make_shared<MessageQueue>();
    }
    inboxes_[to_id]->push(msg);
}

std::string SubAgentManager::receive_message(const std::string& from_id) {
    std::shared_ptr<MessageQueue> q;
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        if (inboxes_.find(from_id) != inboxes_.end()) {
            q = inboxes_[from_id];
        }
    }
    if (q) {
        return q->pop();
    }
    return "";
}

void SubAgentManager::join_all() {
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
}

} // namespace agent
