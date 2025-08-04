#include "__init__.h"
#include <unordered_map>
#include <stdexcept>

namespace openai_agents {
namespace extensions {

// Static member definitions
std::unordered_map<std::string, HandoffFilter> ExtensionRegistry::filters_;
std::unordered_map<std::string, std::string> ExtensionRegistry::prompt_templates_;

void ExtensionRegistry::register_filter(const std::string& name, HandoffFilter filter) {
    filters_[name] = filter;
}

HandoffFilter ExtensionRegistry::get_filter(const std::string& name) {
    auto it = filters_.find(name);
    if (it == filters_.end()) {
        throw std::runtime_error("Filter not found: " + name);
    }
    return it->second;
}

std::vector<std::string> ExtensionRegistry::list_filters() {
    std::vector<std::string> names;
    names.reserve(filters_.size());
    
    for (const auto& [name, filter] : filters_) {
        names.push_back(name);
    }
    
    return names;
}

void ExtensionRegistry::register_prompt_template(const std::string& name, const std::string& template_text) {
    prompt_templates_[name] = template_text;
}

std::string ExtensionRegistry::get_prompt_template(const std::string& name) {
    auto it = prompt_templates_.find(name);
    if (it == prompt_templates_.end()) {
        throw std::runtime_error("Prompt template not found: " + name);
    }
    return it->second;
}

std::vector<std::string> ExtensionRegistry::list_prompt_templates() {
    std::vector<std::string> names;
    names.reserve(prompt_templates_.size());
    
    for (const auto& [name, template_text] : prompt_templates_) {
        names.push_back(name);
    }
    
    return names;
}

} // namespace extensions
} // namespace openai_agents