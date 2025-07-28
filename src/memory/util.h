#pragma once

/**
 * Memory utilities and helpers
 */

#include "session.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <any>

namespace openai_agents {
namespace memory {

// Session utilities
class SessionUtils {
public:
    // Session validation
    static bool validate_session_id(const std::string& session_id);
    static std::string normalize_session_id(const std::string& session_id);
    static std::string generate_session_id();
    
    // Session migration
    static void migrate_session(
        std::shared_ptr<Session> source,
        std::shared_ptr<Session> target,
        bool clear_source = false
    );
    
    // Batch operations
    static std::future<void> backup_sessions(
        const std::vector<std::shared_ptr<Session>>& sessions,
        const std::string& backup_path
    );
    
    static std::future<std::vector<std::shared_ptr<Session>>> restore_sessions(
        const std::string& backup_path,
        SessionFactory::SessionType target_type = SessionFactory::SessionType::Auto
    );
    
    // Session statistics
    struct SessionStats {
        size_t total_items;
        size_t total_size_bytes;
        std::chrono::system_clock::time_point oldest_item;
        std::chrono::system_clock::time_point newest_item;
        std::map<std::string, size_t> item_type_counts;
    };
    
    static SessionStats analyze_session(std::shared_ptr<Session> session);
    static std::map<std::string, SessionStats> analyze_all_sessions(SessionManager& manager);
    
    // Session cleanup
    static size_t cleanup_empty_sessions(SessionManager& manager);
    static size_t cleanup_old_sessions(
        SessionManager& manager,
        std::chrono::hours max_age
    );
    
    // Session merging
    static std::shared_ptr<Session> merge_sessions(
        const std::vector<std::shared_ptr<Session>>& sessions,
        const std::string& target_session_id,
        SessionFactory::SessionType target_type = SessionFactory::SessionType::Auto
    );
};

// Session event listener interface
class SessionEventListener {
public:
    virtual ~SessionEventListener() = default;
    
    virtual void on_session_created(const std::string& session_id) {}
    virtual void on_session_destroyed(const std::string& session_id) {}
    virtual void on_items_added(const std::string& session_id, size_t count) {}
    virtual void on_items_removed(const std::string& session_id, size_t count) {}
    virtual void on_session_cleared(const std::string& session_id) {}
};

// Observable session manager
class ObservableSessionManager : public SessionManager {
private:
    std::vector<std::shared_ptr<SessionEventListener>> listeners_;
    mutable std::shared_mutex listeners_mutex_;

public:
    ObservableSessionManager(
        const std::string& default_db_path = ":memory:",
        const std::string& default_sessions_table = "agent_sessions",
        const std::string& default_messages_table = "agent_messages"
    );
    
    // Listener management
    void add_listener(std::shared_ptr<SessionEventListener> listener);
    void remove_listener(std::shared_ptr<SessionEventListener> listener);
    void clear_listeners();
    
    // Override parent methods to emit events
    std::shared_ptr<Session> create_session(const std::string& session_id) override;
    void remove_session(const std::string& session_id) override;
    void clear_all_sessions() override;

private:
    void emit_session_created(const std::string& session_id);
    void emit_session_destroyed(const std::string& session_id);
};

// Session cache for improved performance
class SessionCache {
private:
    std::map<std::string, std::shared_ptr<Session>> cache_;
    mutable std::shared_mutex cache_mutex_;
    size_t max_size_;
    std::chrono::minutes ttl_;
    std::map<std::string, std::chrono::system_clock::time_point> access_times_;

public:
    SessionCache(size_t max_size = 100, std::chrono::minutes ttl = std::chrono::minutes(30));
    
    // Cache operations
    std::shared_ptr<Session> get(const std::string& session_id);
    void put(const std::string& session_id, std::shared_ptr<Session> session);
    void remove(const std::string& session_id);
    void clear();
    
    // Cache management
    void cleanup_expired();
    size_t size() const;
    double hit_rate() const;
    
    // Configuration
    void set_max_size(size_t max_size) { max_size_ = max_size; }
    void set_ttl(std::chrono::minutes ttl) { ttl_ = ttl; }

private:
    void evict_oldest();
    bool is_expired(const std::string& session_id) const;
    
    mutable size_t hits_ = 0;
    mutable size_t misses_ = 0;
};

// Cached session manager
class CachedSessionManager : public SessionManager {
private:
    std::unique_ptr<SessionCache> cache_;

public:
    CachedSessionManager(
        size_t cache_size = 100,
        std::chrono::minutes cache_ttl = std::chrono::minutes(30),
        const std::string& default_db_path = ":memory:",
        const std::string& default_sessions_table = "agent_sessions",
        const std::string& default_messages_table = "agent_messages"
    );
    
    // Override parent methods to use cache
    std::shared_ptr<Session> get_session(const std::string& session_id) override;
    std::shared_ptr<Session> get_or_create_session(const std::string& session_id) override;
    void remove_session(const std::string& session_id) override;
    void clear_all_sessions() override;
    
    // Cache management
    SessionCache& get_cache() { return *cache_; }
    void clear_cache() { cache_->clear(); }
    double get_cache_hit_rate() const { return cache_->hit_rate(); }
};

// Session pool for connection reuse
class SessionPool {
private:
    std::vector<std::shared_ptr<Session>> available_sessions_;
    std::map<std::string, std::shared_ptr<Session>> active_sessions_;
    mutable std::mutex pool_mutex_;
    SessionFactory::SessionType session_type_;
    std::map<std::string, std::string> session_options_;
    size_t max_pool_size_;

public:
    SessionPool(
        SessionFactory::SessionType session_type = SessionFactory::SessionType::SQLite,
        const std::map<std::string, std::string>& options = {},
        size_t max_pool_size = 50
    );
    
    ~SessionPool();
    
    // Pool operations
    std::shared_ptr<Session> acquire_session(const std::string& session_id);
    void release_session(const std::string& session_id);
    
    // Pool management
    void warm_up(size_t count);
    void cleanup();
    size_t active_count() const;
    size_t available_count() const;
    
    // Configuration
    void set_max_pool_size(size_t max_size) { max_pool_size_ = max_size; }
    size_t get_max_pool_size() const { return max_pool_size_; }

private:
    std::shared_ptr<Session> create_pooled_session(const std::string& session_id);
};

} // namespace memory
} // namespace openai_agents