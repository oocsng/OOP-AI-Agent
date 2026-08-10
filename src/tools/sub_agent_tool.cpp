#include "sub_agent_tool.h"

namespace agent {

SubAgentTool::SubAgentTool(AgentSpawner spawn, MessageSender send, MessageReceiver recv)
    : spawn_(std::move(spawn)), send_(std::move(send)), recv_(std::move(recv)) {}

std::string SubAgentTool::execute(const nlohmann::json& args) {
    if (!args.contains("action")) return "Error: missing action";
    std::string action = args["action"];

    if (action == "spawn") {
        if (!args.contains("id") || !args.contains("task")) return "Error: missing id/task";
        if (spawn_) spawn_(args["id"], args["task"]);
        return "Spawned sub-agent: " + args["id"].get<std::string>();
    } else if (action == "send") {
        if (!args.contains("id") || !args.contains("msg")) return "Error: missing id/msg";
        if (send_) send_(args["id"], args["msg"]);
        return "Message sent to " + args["id"].get<std::string>();
    } else if (action == "receive") {
        if (!args.contains("id")) return "Error: missing id";
        if (recv_) return "Received: " + recv_(args["id"]);
        return "Error: receive callback not set";
    }
    return "Unknown action";
}

} // namespace agent
