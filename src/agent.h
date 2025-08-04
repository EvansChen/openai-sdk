// gurong  2025-7-27
#pragma once
#include <string>
#include <vector>

namespace agents {
class Agent {
public:
    virtual ~Agent() = default;

    Agent(const std::string& name,
           const std::string& instructions = "你是一个牛逼的AI助手，帮助用户完成任务。",
           const std::string& handoff_description = "")
        : name_(name), handoff_description_(handoff_description), instructions_(instructions) {}

    virtual void run(const std::string& input);

    virtual void stream(std::string& output, const std::string& delta) = 0;

    virtual void set_sub_agents(std::vector<Agent*>& sub_agents) = 0;
protected:
    std::string name_;
    std::string handoff_description_;
    std::string instructions_;

    std::vector<Agent*> sub_agents_;
};

} // namespace agents