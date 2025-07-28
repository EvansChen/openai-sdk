#pragma once

/**
 * Agent items and message types
 */

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

namespace openai_agents {

// Forward declarations
class Tool;

// Base item types
enum class ItemType {
    Message,
    Tool,
    Response,
    Image,
    File,
    Custom
};

// Base item class
class Item {
public:
    virtual ~Item() = default;
    virtual ItemType get_type() const = 0;
    virtual std::string to_string() const = 0;
    virtual std::map<std::string, std::any> to_dict() const = 0;
};

// Message item
class MessageItem : public Item {
private:
    std::string role_;
    std::string content_;
    std::optional<std::string> name_;
    std::map<std::string, std::any> metadata_;

public:
    MessageItem(const std::string& role, const std::string& content, 
                const std::optional<std::string>& name = std::nullopt,
                const std::map<std::string, std::any>& metadata = {});

    ItemType get_type() const override { return ItemType::Message; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_role() const { return role_; }
    const std::string& get_content() const { return content_; }
    const std::optional<std::string>& get_name() const { return name_; }
    const std::map<std::string, std::any>& get_metadata() const { return metadata_; }

    // Setters
    void set_content(const std::string& content) { content_ = content; }
    void set_name(const std::optional<std::string>& name) { name_ = name; }
    void add_metadata(const std::string& key, const std::any& value) { metadata_[key] = value; }
};

// Tool call item
class ToolCallItem : public Item {
private:
    std::string tool_call_id_;
    std::string function_name_;
    std::string arguments_;
    std::shared_ptr<Tool> tool_;

public:
    ToolCallItem(const std::string& tool_call_id, const std::string& function_name,
                 const std::string& arguments, std::shared_ptr<Tool> tool = nullptr);

    ItemType get_type() const override { return ItemType::Tool; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_tool_call_id() const { return tool_call_id_; }
    const std::string& get_function_name() const { return function_name_; }
    const std::string& get_arguments() const { return arguments_; }
    std::shared_ptr<Tool> get_tool() const { return tool_; }
};

// Tool response item
class ToolResponseItem : public Item {
private:
    std::string tool_call_id_;
    std::string content_;
    bool is_error_;

public:
    ToolResponseItem(const std::string& tool_call_id, const std::string& content, bool is_error = false);

    ItemType get_type() const override { return ItemType::Response; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_tool_call_id() const { return tool_call_id_; }
    const std::string& get_content() const { return content_; }
    bool is_error() const { return is_error_; }
};

// Image item
class ImageItem : public Item {
private:
    std::string url_;
    std::optional<std::string> detail_;
    std::optional<std::string> mime_type_;

public:
    ImageItem(const std::string& url, 
              const std::optional<std::string>& detail = std::nullopt,
              const std::optional<std::string>& mime_type = std::nullopt);

    ItemType get_type() const override { return ItemType::Image; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_url() const { return url_; }
    const std::optional<std::string>& get_detail() const { return detail_; }
    const std::optional<std::string>& get_mime_type() const { return mime_type_; }
};

// File item
class FileItem : public Item {
private:
    std::string path_;
    std::string filename_;
    std::optional<std::string> mime_type_;
    std::optional<size_t> size_;

public:
    FileItem(const std::string& path, const std::string& filename,
             const std::optional<std::string>& mime_type = std::nullopt,
             const std::optional<size_t>& size = std::nullopt);

    ItemType get_type() const override { return ItemType::File; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_path() const { return path_; }
    const std::string& get_filename() const { return filename_; }
    const std::optional<std::string>& get_mime_type() const { return mime_type_; }
    const std::optional<size_t>& get_size() const { return size_; }
};

// Custom item for extensibility
class CustomItem : public Item {
private:
    std::string type_name_;
    std::map<std::string, std::any> data_;

public:
    CustomItem(const std::string& type_name, const std::map<std::string, std::any>& data);

    ItemType get_type() const override { return ItemType::Custom; }
    std::string to_string() const override;
    std::map<std::string, std::any> to_dict() const override;

    // Getters
    const std::string& get_type_name() const { return type_name_; }
    const std::map<std::string, std::any>& get_data() const { return data_; }
};

// Item collection
class ItemCollection {
private:
    std::vector<std::shared_ptr<Item>> items_;

public:
    void add_item(std::shared_ptr<Item> item);
    void add_message(const std::string& role, const std::string& content, 
                     const std::optional<std::string>& name = std::nullopt);
    void add_tool_call(const std::string& tool_call_id, const std::string& function_name,
                       const std::string& arguments, std::shared_ptr<Tool> tool = nullptr);
    void add_tool_response(const std::string& tool_call_id, const std::string& content, bool is_error = false);

    // Access
    const std::vector<std::shared_ptr<Item>>& get_items() const { return items_; }
    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    
    // Filtering
    std::vector<std::shared_ptr<Item>> filter_by_type(ItemType type) const;
    std::vector<std::shared_ptr<MessageItem>> get_messages() const;
    std::vector<std::shared_ptr<ToolCallItem>> get_tool_calls() const;

    // Conversion
    std::vector<std::map<std::string, std::any>> to_dict_list() const;
    std::string to_string() const;

    // Clear
    void clear() { items_.clear(); }
};

// Helper functions
std::shared_ptr<MessageItem> create_user_message(const std::string& content);
std::shared_ptr<MessageItem> create_assistant_message(const std::string& content);
std::shared_ptr<MessageItem> create_system_message(const std::string& content);

} // namespace openai_agents