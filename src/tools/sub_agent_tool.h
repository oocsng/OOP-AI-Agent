#pragma once
#include "tool.h"
#include <string>
#include <functional>

namespace agent {

using AgentSpawner = std::function<void(const std::string&, const std::string&)>;
using MessageSender = std::function<void(const std::string&, const std::string&)>;
using MessageReceiver = std::function<std::string(const std::string&)>;

class SubAgentTool : public Tool {
public:
    SubAgentTool(AgentSpawner spawn, MessageSender send, MessageReceiver recv);
    [[nodiscard]] std::string name() const override { return "sub_agent"; }
    [[nodiscard]] std::string description() const override {
        return "Spawn and communicate with sub-agents. action='spawn', id='...', task='...' OR action='send', id='...', msg='...' OR action='receive', id='...'";
    }
    std::string execute(const nlohmann::json& args) override;
    [[nodiscard]] nlohmann::json parameters_schema() const override {
        return nlohmann::json::object();
    }
private:
    AgentSpawner spawn_;
    MessageSender send_;
    MessageReceiver recv_;
};

} // namespace agent
