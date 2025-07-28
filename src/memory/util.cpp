#include "util.h"
#include "../exceptions.h"
#include "../logger.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

namespace openai_agents {
namespace memory {

// SessionUtils implementation
bool SessionUtils::validate_session_id(const std::string& session_id) {
    if (session_id.empty() || session_id.length() > 255) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, hyphens, underscores)
    std::regex valid_pattern("^[a-zA-Z0-9_-]+$");
    return std::regex_match(session_id, valid_pattern);
}

std::string SessionUtils::normalize_session_id(const std::string& session_id) {
    std::string normalized = session_id;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Replace invalid characters with underscores
    std::regex invalid_chars("[^a-zA-Z0-9_-]");
    normalized = std::regex_replace(normalized, invalid_chars, "_");
    
    // Trim to max length
    if (normalized.length() > 255) {
        normalized = normalized.substr(0, 255);
    }
    
    return normalized;
}

std::string SessionUtils::generate_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << "session_" << timestamp << "_";
    
    // Add random hex suffix
    for (int i = 0; i < 8; i++) {
        oss << std::hex << dis(gen);
    }
    
    return oss.str();
}

void SessionUtils::migrate_session(
    std::shared_ptr<Session> source,
    std::shared_ptr<Session> target,
    bool clear_source
) {
    if (!source || !target) {
        throw AgentsException("Source and target sessions must not be null");
    }
    
    if (source->get_session_id() == target->get_session_id()) {
        throw AgentsException("Cannot migrate session to itself");
    }
    
    // Get all items from source
    auto items = source->get_items_sync();
    
    if (!items.empty()) {
        // Add items to target
        target->add_items_sync(items);
        
        // Copy metadata
        auto source_metadata = source->get_metadata();
        for (const auto& [key, value] : source_metadata) {
            target->set_metadata(key, value);
        }
        
        // Clear source if requested
        if (clear_source) {
            source->clear_session_sync();
        }
    }
}

SessionUtils::SessionStats SessionUtils::analyze_session(std::shared_ptr<Session> session) {
    if (!session) {
        throw AgentsException("Session cannot be null");
    }
    
    SessionStats stats;
    stats.total_items = session->get_item_count();
    stats.total_size_bytes = 0;
    stats.oldest_item = std::chrono::system_clock::now();
    stats.newest_item = std::chrono::system_clock::time_point::min();
    
    auto items = session->get_items_sync();
    
    for (const auto& item : items) {
        if (item) {
            // Estimate size (simplified)
            auto item_str = item->to_string();
            stats.total_size_bytes += item_str.length();
            
            // Count by type
            std::string type_name;
            switch (item->get_type()) {
            case ItemType::Message: type_name = "message"; break;
            case ItemType::Tool: type_name = "tool"; break;
            case ItemType::Response: type_name = "response"; break;
            case ItemType::Image: type_name = "image"; break;
            case ItemType::File: type_name = "file"; break;
            case ItemType::Custom: type_name = "custom"; break;
            }
            stats.item_type_counts[type_name]++;
        }
    }
    
    if (!items.empty()) {
        // For simplification, use session timestamps
        stats.oldest_item = session->get_created_at();
        stats.newest_item = session->get_updated_at();
    }
    
    return stats;
}

std::map<std::string, SessionUtils::SessionStats> SessionUtils::analyze_all_sessions(SessionManager& manager) {
    std::map<std::string, SessionStats> all_stats;
    
    auto session_ids = manager.list_session_ids();
    for (const auto& session_id : session_ids) {
        auto session = manager.get_session(session_id);
        if (session) {
            all_stats[session_id] = analyze_session(session);
        }
    }
    
    return all_stats;
}

size_t SessionUtils::cleanup_empty_sessions(SessionManager& manager) {
    auto session_ids = manager.list_session_ids();
    size_t removed_count = 0;
    
    for (const auto& session_id : session_ids) {
        auto session = manager.get_session(session_id);
        if (session && session->get_item_count() == 0) {
            manager.remove_session(session_id);
            removed_count++;
        }
    }
    
    return removed_count;
}

size_t SessionUtils::cleanup_old_sessions(
    SessionManager& manager,
    std::chrono::hours max_age
) {
    auto cutoff_time = std::chrono::system_clock::now() - max_age;
    auto session_ids = manager.list_session_ids();
    size_t removed_count = 0;
    
    for (const auto& session_id : session_ids) {
        auto session = manager.get_session(session_id);
        if (session && session->get_updated_at() < cutoff_time) {
            manager.remove_session(session_id);
            removed_count++;
        }
    }
    
    return removed_count;
}

// ObservableSessionManager implementation
ObservableSessionManager::ObservableSessionManager(
    const std::string& default_db_path,
    const std::string& default_sessions_table,
    const std::string& default_messages_table
) : SessionManager(default_db_path, default_sessions_table, default_messages_table) {
}

void ObservableSessionManager::add_listener(std::shared_ptr<SessionEventListener> listener) {
    std::unique_lock<std::shared_mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
}

void ObservableSessionManager::remove_listener(std::shared_ptr<SessionEventListener> listener) {
    std::unique_lock<std::shared_mutex> lock(listeners_mutex_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end()
    );
}

void ObservableSessionManager::clear_listeners() {
    std::unique_lock<std::shared_mutex> lock(listeners_mutex_);
    listeners_.clear();
}

std::shared_ptr<Session> ObservableSessionManager::create_session(const std::string& session_id) {
    auto session = SessionManager::create_session(session_id);
    emit_session_created(session_id);
    return session;
}

void ObservableSessionManager::remove_session(const std::string& session_id) {
    SessionManager::remove_session(session_id);
    emit_session_destroyed(session_id);
}

void ObservableSessionManager::clear_all_sessions() {
    auto session_ids = list_session_ids();
    SessionManager::clear_all_sessions();
    
    for (const auto& session_id : session_ids) {
        emit_session_destroyed(session_id);
    }
}

void ObservableSessionManager::emit_session_created(const std::string& session_id) {
    std::shared_lock<std::shared_mutex> lock(listeners_mutex_);
    for (auto& listener : listeners_) {
        if (listener) {
            listener->on_session_created(session_id);
        }
    }
}

void ObservableSessionManager::emit_session_destroyed(const std::string& session_id) {
    std::shared_lock<std::shared_mutex> lock(listeners_mutex_);
    for (auto& listener : listeners_) {
        if (listener) {
            listener->on_session_destroyed(session_id);
        }
    }
}

// SessionCache implementation
SessionCache::SessionCache(size_t max_size, std::chrono::minutes ttl)
    : max_size_(max_size), ttl_(ttl) {
}

std::shared_ptr<Session> SessionCache::get(const std::string& session_id) {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto it = cache_.find(session_id);
    if (it != cache_.end() && !is_expired(session_id)) {
        access_times_[session_id] = std::chrono::system_clock::now();
        hits_++;
        return it->second;
    }
    
    misses_++;
    return nullptr;
}

void SessionCache::put(const std::string& session_id, std::shared_ptr<Session> session) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    if (cache_.size() >= max_size_) {
        evict_oldest();
    }
    
    cache_[session_id] = session;
    access_times_[session_id] = std::chrono::system_clock::now();
}

void SessionCache::remove(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    cache_.erase(session_id);
    access_times_.erase(session_id);
}

void SessionCache::clear() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    cache_.clear();
    access_times_.clear();
    hits_ = 0;
    misses_ = 0;
}

void SessionCache::cleanup_expired() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto it = cache_.begin();
    
    while (it != cache_.end()) {
        auto access_it = access_times_.find(it->first);
        if (access_it != access_times_.end() && 
            (now - access_it->second) > ttl_) {
            access_times_.erase(access_it);
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t SessionCache::size() const {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    return cache_.size();
}

double SessionCache::hit_rate() const {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    size_t total = hits_ + misses_;
    return total > 0 ? static_cast<double>(hits_) / total : 0.0;
}

void SessionCache::evict_oldest() {
    if (access_times_.empty()) return;
    
    auto oldest = std::min_element(
        access_times_.begin(), 
        access_times_.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        }
    );
    
    if (oldest != access_times_.end()) {
        cache_.erase(oldest->first);
        access_times_.erase(oldest);
    }
}

bool SessionCache::is_expired(const std::string& session_id) const {
    auto access_it = access_times_.find(session_id);
    if (access_it == access_times_.end()) return true;
    
    auto now = std::chrono::system_clock::now();
    return (now - access_it->second) > ttl_;
}

// CachedSessionManager implementation
CachedSessionManager::CachedSessionManager(
    size_t cache_size,
    std::chrono::minutes cache_ttl,
    const std::string& default_db_path,
    const std::string& default_sessions_table,
    const std::string& default_messages_table
) : SessionManager(default_db_path, default_sessions_table, default_messages_table),
    cache_(std::make_unique<SessionCache>(cache_size, cache_ttl)) {
}

std::shared_ptr<Session> CachedSessionManager::get_session(const std::string& session_id) {
    // Try cache first
    auto session = cache_->get(session_id);
    if (session) {
        return session;
    }
    
    // Load from storage
    session = SessionManager::get_session(session_id);
    if (session) {
        cache_->put(session_id, session);
    }
    
    return session;
}

std::shared_ptr<Session> CachedSessionManager::get_or_create_session(const std::string& session_id) {
    auto session = get_session(session_id);
    if (!session) {
        session = SessionManager::create_session(session_id);
        cache_->put(session_id, session);
    }
    return session;
}

void CachedSessionManager::remove_session(const std::string& session_id) {
    cache_->remove(session_id);
    SessionManager::remove_session(session_id);
}

void CachedSessionManager::clear_all_sessions() {
    cache_->clear();
    SessionManager::clear_all_sessions();
}

// SessionPool implementation
SessionPool::SessionPool(
    SessionFactory::SessionType session_type,
    const std::map<std::string, std::string>& options,
    size_t max_pool_size
) : session_type_(session_type), session_options_(options), max_pool_size_(max_pool_size) {
}

SessionPool::~SessionPool() {
    cleanup();
}

std::shared_ptr<Session> SessionPool::acquire_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Check if session is already active
    auto active_it = active_sessions_.find(session_id);
    if (active_it != active_sessions_.end()) {
        return active_it->second;
    }
    
    // Try to reuse from available sessions
    std::shared_ptr<Session> session;
    if (!available_sessions_.empty()) {
        session = available_sessions_.back();
        available_sessions_.pop_back();
        
        // Reset session for new use
        session->clear_session_sync();
    } else {
        // Create new session
        session = create_pooled_session(session_id);
    }
    
    active_sessions_[session_id] = session;
    return session;
}

void SessionPool::release_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto active_it = active_sessions_.find(session_id);
    if (active_it != active_sessions_.end()) {
        auto session = active_it->second;
        active_sessions_.erase(active_it);
        
        // Return to pool if under limit
        if (available_sessions_.size() < max_pool_size_) {
            available_sessions_.push_back(session);
        }
        // Otherwise let it be destroyed
    }
}

void SessionPool::warm_up(size_t count) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    for (size_t i = 0; i < count && available_sessions_.size() < max_pool_size_; i++) {
        auto session = create_pooled_session("pool_session_" + std::to_string(i));
        available_sessions_.push_back(session);
    }
}

void SessionPool::cleanup() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    available_sessions_.clear();
    active_sessions_.clear();
}

size_t SessionPool::active_count() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return active_sessions_.size();
}

size_t SessionPool::available_count() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return available_sessions_.size();
}

std::shared_ptr<Session> SessionPool::create_pooled_session(const std::string& session_id) {
    return SessionFactory::create_session(session_id, session_type_, session_options_);
}

} // namespace memory
} // namespace openai_agents