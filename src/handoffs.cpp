#include "handoffs.h"
#include "agent.h"
#include "run_context.h"
#include "exceptions.h"

namespace openai_agents {

// Handoff implementation
Handoff::Handoff(std::shared_ptr<Agent> target_agent, 
                 const std::optional<std::string>& context,
                 const std::map<std::string, std::any>& metadata)
    : target_agent_(target_agent), context_(context), metadata_(metadata) {
    if (!target_agent_) {
        throw AgentsException("Target agent cannot be null");
    }
}

void Handoff::add_filter(HandoffFilter filter) {
    filters_.push_back(filter);
}

void Handoff::add_filters(const std::vector<HandoffFilter>& filters) {
    filters_.insert(filters_.end(), filters.begin(), filters.end());
}

HandoffValidation Handoff::validate(std::shared_ptr<RunContext> context) const {
    if (!target_agent_) {
        return HandoffValidation{false, "Target agent is null"};
    }

    // Apply all filters
    for (const auto& filter : filters_) {
        try {
            if (!filter(target_agent_, context_.value_or(""), context)) {
                return HandoffValidation{false, "Handoff blocked by filter"};
            }
        } catch (const std::exception& e) {
            return HandoffValidation{false, "Filter error: " + std::string(e.what())};
        }
    }

    return HandoffValidation{true, ""};
}

HandoffResult Handoff::execute(std::shared_ptr<RunContext> context) const {
    auto validation = validate(context);
    if (!validation.is_valid) {
        throw AgentsException("Handoff validation failed: " + validation.error_message);
    }

    return HandoffResult{
        .agent = target_agent_,
        .context = context_.value_or(""),
        .metadata = metadata_
    };
}

// HandoffManager implementation
void HandoffManager::register_agent(const std::string& name, std::shared_ptr<Agent> agent) {
    if (!agent) {
        throw AgentsException("Cannot register null agent");
    }
    registered_agents_[name] = agent;
}

void HandoffManager::unregister_agent(const std::string& name) {
    registered_agents_.erase(name);
}

std::shared_ptr<Agent> HandoffManager::get_agent(const std::string& name) const {
    auto it = registered_agents_.find(name);
    if (it != registered_agents_.end()) {
        return it->second;
    }
    return nullptr;
}

void HandoffManager::add_global_filter(HandoffFilter filter) {
    global_filters_.push_back(filter);
}

void HandoffManager::clear_global_filters() {
    global_filters_.clear();
}

Handoff HandoffManager::create_handoff(const std::string& target_agent_name,
                                      const std::optional<std::string>& context,
                                      const std::map<std::string, std::any>& metadata) const {
    auto agent = get_agent(target_agent_name);
    if (!agent) {
        throw AgentsException("Agent not found: " + target_agent_name);
    }

    Handoff handoff(agent, context, metadata);
    handoff.add_filters(global_filters_);
    return handoff;
}

std::vector<HandoffResult> HandoffManager::execute_handoffs(
    const std::vector<Handoff>& handoffs,
    std::shared_ptr<RunContext> context
) const {
    std::vector<HandoffResult> results;
    results.reserve(handoffs.size());

    for (const auto& handoff : handoffs) {
        try {
            results.push_back(handoff.execute(context));
        } catch (const std::exception& e) {
            // Could collect errors or rethrow based on policy
            throw;
        }
    }

    return results;
}

std::vector<std::string> HandoffManager::list_agents() const {
    std::vector<std::string> names;
    names.reserve(registered_agents_.size());
    for (const auto& pair : registered_agents_) {
        names.push_back(pair.first);
    }
    return names;
}

bool HandoffManager::has_agent(const std::string& name) const {
    return registered_agents_.find(name) != registered_agents_.end();
}

// Helper functions
Handoff handoff_to(std::shared_ptr<Agent> agent, 
                   const std::string& context,
                   const std::map<std::string, std::any>& metadata) {
    return Handoff(agent, context.empty() ? std::nullopt : std::make_optional(context), metadata);
}

Handoff conditional_handoff(std::shared_ptr<Agent> agent,
                           std::function<bool(std::shared_ptr<RunContext>)> condition,
                           const std::string& context,
                           const std::map<std::string, std::any>& metadata) {
    Handoff handoff(agent, context.empty() ? std::nullopt : std::make_optional(context), metadata);
    
    handoff.add_filter([condition](std::shared_ptr<Agent> agent, const std::string& ctx, std::shared_ptr<RunContext> run_ctx) {
        return condition(run_ctx);
    });
    
    return handoff;
}

} // namespace openai_agents