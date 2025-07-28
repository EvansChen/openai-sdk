#include "items.h"
#include "tool.h"
#include <sstream>

namespace openai_agents {

// MessageItem implementation
MessageItem::MessageItem(const std::string& role, const std::string& content, 
                         const std::optional<std::string>& name,
                         const std::map<std::string, std::any>& metadata)
    : role_(role), content_(content), name_(name), metadata_(metadata) {}

std::string MessageItem::to_string() const {
    std::ostringstream oss;
    oss << "[" << role_ << "]";
    if (name_) {
        oss << " (" << *name_ << ")";
    }
    oss << ": " << content_;
    return oss.str();
}

std::map<std::string, std::any> MessageItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = std::string("message");
    dict["role"] = role_;
    dict["content"] = content_;
    if (name_) {
        dict["name"] = *name_;
    }
    if (!metadata_.empty()) {
        dict["metadata"] = metadata_;
    }
    return dict;
}

// ToolCallItem implementation
ToolCallItem::ToolCallItem(const std::string& tool_call_id, const std::string& function_name,
                           const std::string& arguments, std::shared_ptr<Tool> tool)
    : tool_call_id_(tool_call_id), function_name_(function_name), arguments_(arguments), tool_(tool) {}

std::string ToolCallItem::to_string() const {
    std::ostringstream oss;
    oss << "[TOOL_CALL] " << function_name_ << "(" << arguments_ << ")";
    return oss.str();
}

std::map<std::string, std::any> ToolCallItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = std::string("tool_call");
    dict["tool_call_id"] = tool_call_id_;
    dict["function_name"] = function_name_;
    dict["arguments"] = arguments_;
    return dict;
}

// ToolResponseItem implementation
ToolResponseItem::ToolResponseItem(const std::string& tool_call_id, const std::string& content, bool is_error)
    : tool_call_id_(tool_call_id), content_(content), is_error_(is_error) {}

std::string ToolResponseItem::to_string() const {
    std::ostringstream oss;
    oss << "[TOOL_RESPONSE" << (is_error_ ? "_ERROR" : "") << "] " << content_;
    return oss.str();
}

std::map<std::string, std::any> ToolResponseItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = std::string("tool_response");
    dict["tool_call_id"] = tool_call_id_;
    dict["content"] = content_;
    dict["is_error"] = is_error_;
    return dict;
}

// ImageItem implementation
ImageItem::ImageItem(const std::string& url, 
                     const std::optional<std::string>& detail,
                     const std::optional<std::string>& mime_type)
    : url_(url), detail_(detail), mime_type_(mime_type) {}

std::string ImageItem::to_string() const {
    std::ostringstream oss;
    oss << "[IMAGE] " << url_;
    if (detail_) {
        oss << " (detail: " << *detail_ << ")";
    }
    return oss.str();
}

std::map<std::string, std::any> ImageItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = std::string("image");
    dict["url"] = url_;
    if (detail_) {
        dict["detail"] = *detail_;
    }
    if (mime_type_) {
        dict["mime_type"] = *mime_type_;
    }
    return dict;
}

// FileItem implementation
FileItem::FileItem(const std::string& path, const std::string& filename,
                   const std::optional<std::string>& mime_type,
                   const std::optional<size_t>& size)
    : path_(path), filename_(filename), mime_type_(mime_type), size_(size) {}

std::string FileItem::to_string() const {
    std::ostringstream oss;
    oss << "[FILE] " << filename_ << " (" << path_ << ")";
    if (size_) {
        oss << " [" << *size_ << " bytes]";
    }
    return oss.str();
}

std::map<std::string, std::any> FileItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = std::string("file");
    dict["path"] = path_;
    dict["filename"] = filename_;
    if (mime_type_) {
        dict["mime_type"] = *mime_type_;
    }
    if (size_) {
        dict["size"] = *size_;
    }
    return dict;
}

// CustomItem implementation
CustomItem::CustomItem(const std::string& type_name, const std::map<std::string, std::any>& data)
    : type_name_(type_name), data_(data) {}

std::string CustomItem::to_string() const {
    std::ostringstream oss;
    oss << "[" << type_name_ << "]";
    return oss.str();
}

std::map<std::string, std::any> CustomItem::to_dict() const {
    std::map<std::string, std::any> dict;
    dict["type"] = type_name_;
    for (const auto& pair : data_) {
        dict[pair.first] = pair.second;
    }
    return dict;
}

// ItemCollection implementation
void ItemCollection::add_item(std::shared_ptr<Item> item) {
    if (item) {
        items_.push_back(item);
    }
}

void ItemCollection::add_message(const std::string& role, const std::string& content, 
                                 const std::optional<std::string>& name) {
    add_item(std::make_shared<MessageItem>(role, content, name));
}

void ItemCollection::add_tool_call(const std::string& tool_call_id, const std::string& function_name,
                                   const std::string& arguments, std::shared_ptr<Tool> tool) {
    add_item(std::make_shared<ToolCallItem>(tool_call_id, function_name, arguments, tool));
}

void ItemCollection::add_tool_response(const std::string& tool_call_id, const std::string& content, bool is_error) {
    add_item(std::make_shared<ToolResponseItem>(tool_call_id, content, is_error));
}

std::vector<std::shared_ptr<Item>> ItemCollection::filter_by_type(ItemType type) const {
    std::vector<std::shared_ptr<Item>> filtered;
    for (const auto& item : items_) {
        if (item->get_type() == type) {
            filtered.push_back(item);
        }
    }
    return filtered;
}

std::vector<std::shared_ptr<MessageItem>> ItemCollection::get_messages() const {
    std::vector<std::shared_ptr<MessageItem>> messages;
    for (const auto& item : items_) {
        if (auto msg = std::dynamic_pointer_cast<MessageItem>(item)) {
            messages.push_back(msg);
        }
    }
    return messages;
}

std::vector<std::shared_ptr<ToolCallItem>> ItemCollection::get_tool_calls() const {
    std::vector<std::shared_ptr<ToolCallItem>> tool_calls;
    for (const auto& item : items_) {
        if (auto call = std::dynamic_pointer_cast<ToolCallItem>(item)) {
            tool_calls.push_back(call);
        }
    }
    return tool_calls;
}

std::vector<std::map<std::string, std::any>> ItemCollection::to_dict_list() const {
    std::vector<std::map<std::string, std::any>> dicts;
    dicts.reserve(items_.size());
    for (const auto& item : items_) {
        dicts.push_back(item->to_dict());
    }
    return dicts;
}

std::string ItemCollection::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (i > 0) oss << "\n";
        oss << items_[i]->to_string();
    }
    return oss.str();
}

// Helper functions
std::shared_ptr<MessageItem> create_user_message(const std::string& content) {
    return std::make_shared<MessageItem>("user", content);
}

std::shared_ptr<MessageItem> create_assistant_message(const std::string& content) {
    return std::make_shared<MessageItem>("assistant", content);
}

std::shared_ptr<MessageItem> create_system_message(const std::string& content) {
    return std::make_shared<MessageItem>("system", content);
}

} // namespace openai_agents