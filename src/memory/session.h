#pragma once

/**
 * Memory and session management for conversation history
 */

#include "../items.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <future>
#include <map>
#include <any>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <fstream>

namespace openai_agents {
namespace memory {

// Forward declarations
class Item;

// Session interface for conversation history management
class Session {
public:
    virtual ~Session() = default;
    
    // Session identification
    virtual const std::string& get_session_id() const = 0;
    
    // Item management
    virtual std::future<std::vector<std::shared_ptr<Item>>> get_items(
        std::optional<size_t> limit = std::nullopt
    ) = 0;
    
    virtual std::future<void> add_items(
        const std::vector<std::shared_ptr<Item>>& items
    ) = 0;
    
    virtual std::future<std::shared_ptr<Item>> pop_item() = 0;
    
    virtual std::future<void> clear_session() = 0;
    
    // Synchronous convenience methods
    std::vector<std::shared_ptr<Item>> get_items_sync(
        std::optional<size_t> limit = std::nullopt
    );
    
    void add_items_sync(const std::vector<std::shared_ptr<Item>>& items);
    std::shared_ptr<Item> pop_item_sync();
    void clear_session_sync();
    
    // Session metadata
    virtual std::map<std::string, std::any> get_metadata() const = 0;
    virtual void set_metadata(const std::string& key, const std::any& value) = 0;
    virtual bool has_metadata(const std::string& key) const = 0;
    
    // Session statistics
    virtual size_t get_item_count() const = 0;
    virtual std::chrono::system_clock::time_point get_created_at() const = 0;
    virtual std::chrono::system_clock::time_point get_updated_at() const = 0;
};

// Abstract base class for session implementations
class SessionBase : public Session {
protected:
    std::string session_id_;
    std::map<std::string, std::any> metadata_;
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point updated_at_;

public:
    SessionBase(const std::string& session_id);
    
    // Session interface implementation
    const std::string& get_session_id() const override { return session_id_; }
    
    std::map<std::string, std::any> get_metadata() const override { return metadata_; }
    void set_metadata(const std::string& key, const std::any& value) override;
    bool has_metadata(const std::string& key) const override;
    
    std::chrono::system_clock::time_point get_created_at() const override { return created_at_; }
    std::chrono::system_clock::time_point get_updated_at() const override { return updated_at_; }

protected:
    void update_timestamp();
};

// SQLite-based session implementation
class SQLiteSession : public SessionBase {
private:
    std::string db_path_;
    std::string sessions_table_;
    std::string messages_table_;
    bool is_memory_db_;
    
    // Database connection management
    mutable std::mutex connection_mutex_;
    mutable std::shared_ptr<class SQLiteConnection> shared_connection_;
    
    // Thread-local storage for file-based databases
    thread_local static std::shared_ptr<class SQLiteConnection> thread_connection_;

public:
    SQLiteSession(
        const std::string& session_id,
        const std::string& db_path = ":memory:",
        const std::string& sessions_table = "agent_sessions",
        const std::string& messages_table = "agent_messages"
    );
    
    ~SQLiteSession();
    
    // Session interface implementation
    std::future<std::vector<std::shared_ptr<Item>>> get_items(
        std::optional<size_t> limit = std::nullopt
    ) override;
    
    std::future<void> add_items(
        const std::vector<std::shared_ptr<Item>>& items
    ) override;
    
    std::future<std::shared_ptr<Item>> pop_item() override;
    std::future<void> clear_session() override;
    
    size_t get_item_count() const override;
    
    // SQLite-specific methods
    void close();
    std::string get_db_path() const { return db_path_; }
    std::string get_sessions_table() const { return sessions_table_; }
    std::string get_messages_table() const { return messages_table_; }
    
    // Database maintenance
    void vacuum();
    void analyze();
    std::map<std::string, std::any> get_db_stats() const;

private:
    std::shared_ptr<class SQLiteConnection> get_connection() const;
    void init_database();
    void init_db_for_connection(std::shared_ptr<class SQLiteConnection> conn);
    
    // Internal synchronous operations
    std::vector<std::shared_ptr<Item>> get_items_internal(std::optional<size_t> limit);
    void add_items_internal(const std::vector<std::shared_ptr<Item>>& items);
    std::shared_ptr<Item> pop_item_internal();
    void clear_session_internal();
    size_t get_item_count_internal() const;
};

// In-memory session implementation
class MemorySession : public SessionBase {
private:
    std::vector<std::shared_ptr<Item>> items_;
    mutable std::shared_mutex items_mutex_;

public:
    MemorySession(const std::string& session_id);
    
    // Session interface implementation
    std::future<std::vector<std::shared_ptr<Item>>> get_items(
        std::optional<size_t> limit = std::nullopt
    ) override;
    
    std::future<void> add_items(
        const std::vector<std::shared_ptr<Item>>& items
    ) override;
    
    std::future<std::shared_ptr<Item>> pop_item() override;
    std::future<void> clear_session() override;
    
    size_t get_item_count() const override;
    
    // Memory-specific methods
    void reserve_capacity(size_t capacity);
    size_t get_capacity() const;

private:
    // Internal synchronous operations
    std::vector<std::shared_ptr<Item>> get_items_internal(std::optional<size_t> limit);
    void add_items_internal(const std::vector<std::shared_ptr<Item>>& items);
    std::shared_ptr<Item> pop_item_internal();
    void clear_session_internal();
};

// Session manager for handling multiple sessions
class SessionManager {
private:
    std::map<std::string, std::shared_ptr<Session>> sessions_;
    mutable std::shared_mutex sessions_mutex_;
    std::string default_db_path_;
    std::string default_sessions_table_;
    std::string default_messages_table_;

public:
    SessionManager(
        const std::string& default_db_path = ":memory:",
        const std::string& default_sessions_table = "agent_sessions",
        const std::string& default_messages_table = "agent_messages"
    );
    
    // Session creation and retrieval
    std::shared_ptr<Session> get_session(const std::string& session_id);
    std::shared_ptr<Session> create_session(const std::string& session_id);
    std::shared_ptr<Session> get_or_create_session(const std::string& session_id);
    
    // Session types
    std::shared_ptr<SQLiteSession> create_sqlite_session(
        const std::string& session_id,
        const std::string& db_path = ""
    );
    
    std::shared_ptr<MemorySession> create_memory_session(
        const std::string& session_id
    );
    
    // Session management
    bool has_session(const std::string& session_id) const;
    void remove_session(const std::string& session_id);
    void clear_all_sessions();
    
    std::vector<std::string> list_session_ids() const;
    size_t get_session_count() const;
    
    // Batch operations
    std::future<void> clear_all_sessions_async();
    std::future<std::map<std::string, size_t>> get_all_session_stats();
    
    // Configuration
    void set_default_db_path(const std::string& db_path) { default_db_path_ = db_path; }
    const std::string& get_default_db_path() const { return default_db_path_; }
    
    void set_default_tables(const std::string& sessions_table, const std::string& messages_table);
};

// Session factory
class SessionFactory {
public:
    static std::shared_ptr<Session> create_sqlite_session(
        const std::string& session_id,
        const std::string& db_path = ":memory:",
        const std::string& sessions_table = "agent_sessions",
        const std::string& messages_table = "agent_messages"
    );
    
    static std::shared_ptr<Session> create_memory_session(
        const std::string& session_id
    );
    
    static std::shared_ptr<Session> create_default_session(
        const std::string& session_id
    );
    
    // Session type detection
    enum class SessionType {
        Memory,
        SQLite,
        Auto
    };
    
    static std::shared_ptr<Session> create_session(
        const std::string& session_id,
        SessionType type = SessionType::Auto,
        const std::map<std::string, std::string>& options = {}
    );
};

// Global session manager
SessionManager& get_global_session_manager();

// Convenience functions
std::shared_ptr<Session> get_session(const std::string& session_id);
std::shared_ptr<Session> create_session(const std::string& session_id);
std::shared_ptr<Session> get_or_create_session(const std::string& session_id);

} // namespace memory
} // namespace openai_agents