#include "session.h"
#include "../exceptions.h"
#include "../logger.h"
#include "../util/_json.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <sqlite3.h>

namespace openai_agents {
namespace memory {

// SQLite connection wrapper
class SQLiteConnection {
private:
    sqlite3* db_;
    std::string db_path_;
    std::mutex mutex_;

public:
    SQLiteConnection(const std::string& db_path) : db_(nullptr), db_path_(db_path) {
        int rc = sqlite3_open(db_path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::string error = sqlite3_errmsg(db_);
            sqlite3_close(db_);
            throw AgentsException("Failed to open SQLite database: " + error);
        }
        
        // Set WAL mode for better concurrency
        execute("PRAGMA journal_mode=WAL");
        execute("PRAGMA synchronous=NORMAL");
        execute("PRAGMA temp_store=memory");
        execute("PRAGMA mmap_size=268435456"); // 256MB
    }
    
    ~SQLiteConnection() {
        if (db_) {
            sqlite3_close(db_);
        }
    }
    
    void execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        char* error = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            std::string error_msg = error ? error : "Unknown error";
            sqlite3_free(error);
            throw AgentsException("SQLite error: " + error_msg);
        }
    }
    
    std::vector<std::vector<std::string>> query(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw AgentsException("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        }
        
        std::vector<std::vector<std::string>> results;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::vector<std::string> row;
            int column_count = sqlite3_column_count(stmt);
            for (int i = 0; i < column_count; i++) {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row.push_back(text ? text : "");
            }
            results.push_back(row);
        }
        
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            throw AgentsException("Query execution error: " + std::string(sqlite3_errmsg(db_)));
        }
        
        return results;
    }
    
    void execute_with_params(const std::string& sql, const std::vector<std::string>& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw AgentsException("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        }
        
        for (size_t i = 0; i < params.size(); i++) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
        }
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            throw AgentsException("Statement execution error: " + std::string(sqlite3_errmsg(db_)));
        }
    }
    
    void begin_transaction() {
        execute("BEGIN TRANSACTION");
    }
    
    void commit() {
        execute("COMMIT");
    }
    
    void rollback() {
        execute("ROLLBACK");
    }
    
    sqlite3* get_db() const { return db_; }
    const std::string& get_path() const { return db_path_; }
};

// Thread-local storage for SQLite connections
thread_local std::shared_ptr<SQLiteConnection> SQLiteSession::thread_connection_ = nullptr;

// SessionBase implementation
SessionBase::SessionBase(const std::string& session_id)
    : session_id_(session_id),
      created_at_(std::chrono::system_clock::now()),
      updated_at_(std::chrono::system_clock::now()) {
}

void SessionBase::set_metadata(const std::string& key, const std::any& value) {
    metadata_[key] = value;
    update_timestamp();
}

bool SessionBase::has_metadata(const std::string& key) const {
    return metadata_.find(key) != metadata_.end();
}

void SessionBase::update_timestamp() {
    updated_at_ = std::chrono::system_clock::now();
}

// Session convenience methods
std::vector<std::shared_ptr<Item>> Session::get_items_sync(std::optional<size_t> limit) {
    auto future = get_items(limit);
    return future.get();
}

void Session::add_items_sync(const std::vector<std::shared_ptr<Item>>& items) {
    auto future = add_items(items);
    future.wait();
}

std::shared_ptr<Item> Session::pop_item_sync() {
    auto future = pop_item();
    return future.get();
}

void Session::clear_session_sync() {
    auto future = clear_session();
    future.wait();
}

// SQLiteSession implementation
SQLiteSession::SQLiteSession(
    const std::string& session_id,
    const std::string& db_path,
    const std::string& sessions_table,
    const std::string& messages_table
) : SessionBase(session_id),
    db_path_(db_path),
    sessions_table_(sessions_table),
    messages_table_(messages_table),
    is_memory_db_(db_path == ":memory:") {
    
    init_database();
}

SQLiteSession::~SQLiteSession() {
    close();
}

void SQLiteSession::init_database() {
    if (is_memory_db_) {
        // For in-memory databases, use a shared connection
        std::lock_guard<std::mutex> lock(connection_mutex_);
        shared_connection_ = std::make_shared<SQLiteConnection>(db_path_);
        init_db_for_connection(shared_connection_);
    } else {
        // For file databases, initialize schema once
        auto init_conn = std::make_shared<SQLiteConnection>(db_path_);
        init_db_for_connection(init_conn);
    }
}

void SQLiteSession::init_db_for_connection(std::shared_ptr<SQLiteConnection> conn) {
    std::ostringstream sessions_sql;
    sessions_sql << "CREATE TABLE IF NOT EXISTS " << sessions_table_ << " ("
                 << "session_id TEXT PRIMARY KEY,"
                 << "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                 << "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                 << ")";
    
    std::ostringstream messages_sql;
    messages_sql << "CREATE TABLE IF NOT EXISTS " << messages_table_ << " ("
                 << "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 << "session_id TEXT NOT NULL,"
                 << "message_data TEXT NOT NULL,"
                 << "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                 << "FOREIGN KEY (session_id) REFERENCES " << sessions_table_ << " (session_id) ON DELETE CASCADE"
                 << ")";
    
    std::ostringstream index_sql;
    index_sql << "CREATE INDEX IF NOT EXISTS idx_" << messages_table_ << "_session_id "
              << "ON " << messages_table_ << " (session_id, created_at)";
    
    conn->execute(sessions_sql.str());
    conn->execute(messages_sql.str());
    conn->execute(index_sql.str());
}

std::shared_ptr<SQLiteConnection> SQLiteSession::get_connection() const {
    if (is_memory_db_) {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        return shared_connection_;
    } else {
        if (!thread_connection_) {
            thread_connection_ = std::make_shared<SQLiteConnection>(db_path_);
        }
        return thread_connection_;
    }
}

std::future<std::vector<std::shared_ptr<Item>>> SQLiteSession::get_items(std::optional<size_t> limit) {
    return std::async(std::launch::async, [this, limit]() {
        return get_items_internal(limit);
    });
}

std::vector<std::shared_ptr<Item>> SQLiteSession::get_items_internal(std::optional<size_t> limit) {
    auto conn = get_connection();
    
    std::ostringstream sql;
    if (!limit.has_value()) {
        sql << "SELECT message_data FROM " << messages_table_
            << " WHERE session_id = '" << session_id_ << "'"
            << " ORDER BY created_at ASC";
    } else {
        sql << "SELECT message_data FROM " << messages_table_
            << " WHERE session_id = '" << session_id_ << "'"
            << " ORDER BY created_at DESC LIMIT " << limit.value();
    }
    
    auto results = conn->query(sql.str());
    
    std::vector<std::shared_ptr<Item>> items;
    items.reserve(results.size());
    
    for (const auto& row : results) {
        if (!row.empty()) {
            try {
                // Parse JSON and convert to Item
                auto json = util::JsonParser::parse(row[0]);
                auto item = std::make_shared<MessageItem>("", ""); // Simplified for now
                // In a real implementation, would deserialize from JSON
                items.push_back(item);
            } catch (const std::exception& e) {
                auto logger = get_logger("SQLiteSession");
                logger->warning("Failed to parse item from database: " + std::string(e.what()));
            }
        }
    }
    
    // Reverse if we used DESC order for limit
    if (limit.has_value()) {
        std::reverse(items.begin(), items.end());
    }
    
    return items;
}

std::future<void> SQLiteSession::add_items(const std::vector<std::shared_ptr<Item>>& items) {
    return std::async(std::launch::async, [this, items]() {
        add_items_internal(items);
    });
}

void SQLiteSession::add_items_internal(const std::vector<std::shared_ptr<Item>>& items) {
    if (items.empty()) return;
    
    auto conn = get_connection();
    
    try {
        conn->begin_transaction();
        
        // Ensure session exists
        std::ostringstream session_sql;
        session_sql << "INSERT OR IGNORE INTO " << sessions_table_ << " (session_id) VALUES (?)";
        conn->execute_with_params(session_sql.str(), {session_id_});
        
        // Add items
        std::ostringstream item_sql;
        item_sql << "INSERT INTO " << messages_table_ << " (session_id, message_data) VALUES (?, ?)";
        
        for (const auto& item : items) {
            // Serialize item to JSON
            auto json_dict = item->to_dict();
            util::JsonBuilder builder;
            for (const auto& [key, value] : json_dict) {
                builder.set(key, util::JsonUtils::from_any(value));
            }
            std::string json_str = builder.to_string();
            
            conn->execute_with_params(item_sql.str(), {session_id_, json_str});
        }
        
        // Update session timestamp
        std::ostringstream update_sql;
        update_sql << "UPDATE " << sessions_table_ << " SET updated_at = CURRENT_TIMESTAMP WHERE session_id = ?";
        conn->execute_with_params(update_sql.str(), {session_id_});
        
        conn->commit();
        update_timestamp();
        
    } catch (const std::exception& e) {
        conn->rollback();
        throw;
    }
}

std::future<std::shared_ptr<Item>> SQLiteSession::pop_item() {
    return std::async(std::launch::async, [this]() {
        return pop_item_internal();
    });
}

std::shared_ptr<Item> SQLiteSession::pop_item_internal() {
    auto conn = get_connection();
    
    // First, get the most recent item
    std::ostringstream select_sql;
    select_sql << "SELECT id, message_data FROM " << messages_table_
               << " WHERE session_id = '" << session_id_ << "'"
               << " ORDER BY created_at DESC LIMIT 1";
    
    auto results = conn->query(select_sql.str());
    
    if (results.empty()) {
        return nullptr;
    }
    
    std::string item_id = results[0][0];
    std::string message_data = results[0][1];
    
    // Delete the item
    std::ostringstream delete_sql;
    delete_sql << "DELETE FROM " << messages_table_ << " WHERE id = ?";
    conn->execute_with_params(delete_sql.str(), {item_id});
    
    // Parse and return the item
    try {
        auto json = util::JsonParser::parse(message_data);
        auto item = std::make_shared<MessageItem>("", ""); // Simplified for now
        // In a real implementation, would deserialize from JSON
        update_timestamp();
        return item;
    } catch (const std::exception& e) {
        auto logger = get_logger("SQLiteSession");
        logger->warning("Failed to parse popped item: " + std::string(e.what()));
        return nullptr;
    }
}

std::future<void> SQLiteSession::clear_session() {
    return std::async(std::launch::async, [this]() {
        clear_session_internal();
    });
}

void SQLiteSession::clear_session_internal() {
    auto conn = get_connection();
    
    conn->begin_transaction();
    try {
        std::ostringstream messages_sql;
        messages_sql << "DELETE FROM " << messages_table_ << " WHERE session_id = ?";
        conn->execute_with_params(messages_sql.str(), {session_id_});
        
        std::ostringstream sessions_sql;
        sessions_sql << "DELETE FROM " << sessions_table_ << " WHERE session_id = ?";
        conn->execute_with_params(sessions_sql.str(), {session_id_});
        
        conn->commit();
        update_timestamp();
        
    } catch (const std::exception& e) {
        conn->rollback();
        throw;
    }
}

size_t SQLiteSession::get_item_count() const {
    return get_item_count_internal();
}

size_t SQLiteSession::get_item_count_internal() const {
    auto conn = get_connection();
    
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM " << messages_table_ << " WHERE session_id = '" << session_id_ << "'";
    
    auto results = conn->query(sql.str());
    if (!results.empty() && !results[0].empty()) {
        return std::stoul(results[0][0]);
    }
    return 0;
}

void SQLiteSession::close() {
    if (is_memory_db_) {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        shared_connection_.reset();
    } else {
        thread_connection_.reset();
    }
}

void SQLiteSession::vacuum() {
    auto conn = get_connection();
    conn->execute("VACUUM");
}

void SQLiteSession::analyze() {
    auto conn = get_connection();
    conn->execute("ANALYZE");
}

std::map<std::string, std::any> SQLiteSession::get_db_stats() const {
    auto conn = get_connection();
    std::map<std::string, std::any> stats;
    
    try {
        auto results = conn->query("SELECT COUNT(*) FROM " + sessions_table_);
        if (!results.empty() && !results[0].empty()) {
            stats["total_sessions"] = std::stoi(results[0][0]);
        }
        
        results = conn->query("SELECT COUNT(*) FROM " + messages_table_);
        if (!results.empty() && !results[0].empty()) {
            stats["total_messages"] = std::stoi(results[0][0]);
        }
        
        stats["db_path"] = db_path_;
        stats["is_memory_db"] = is_memory_db_;
        
    } catch (const std::exception& e) {
        stats["error"] = std::string(e.what());
    }
    
    return stats;
}

// MemorySession implementation
MemorySession::MemorySession(const std::string& session_id) : SessionBase(session_id) {
}

std::future<std::vector<std::shared_ptr<Item>>> MemorySession::get_items(std::optional<size_t> limit) {
    return std::async(std::launch::async, [this, limit]() {
        return get_items_internal(limit);
    });
}

std::vector<std::shared_ptr<Item>> MemorySession::get_items_internal(std::optional<size_t> limit) {
    std::shared_lock<std::shared_mutex> lock(items_mutex_);
    
    if (!limit.has_value()) {
        return items_;
    }
    
    size_t start_idx = items_.size() > limit.value() ? items_.size() - limit.value() : 0;
    return std::vector<std::shared_ptr<Item>>(items_.begin() + start_idx, items_.end());
}

std::future<void> MemorySession::add_items(const std::vector<std::shared_ptr<Item>>& items) {
    return std::async(std::launch::async, [this, items]() {
        add_items_internal(items);
    });
}

void MemorySession::add_items_internal(const std::vector<std::shared_ptr<Item>>& items) {
    std::unique_lock<std::shared_mutex> lock(items_mutex_);
    items_.insert(items_.end(), items.begin(), items.end());
    update_timestamp();
}

std::future<std::shared_ptr<Item>> MemorySession::pop_item() {
    return std::async(std::launch::async, [this]() {
        return pop_item_internal();
    });
}

std::shared_ptr<Item> MemorySession::pop_item_internal() {
    std::unique_lock<std::shared_mutex> lock(items_mutex_);
    
    if (items_.empty()) {
        return nullptr;
    }
    
    auto item = items_.back();
    items_.pop_back();
    update_timestamp();
    return item;
}

std::future<void> MemorySession::clear_session() {
    return std::async(std::launch::async, [this]() {
        clear_session_internal();
    });
}

void MemorySession::clear_session_internal() {
    std::unique_lock<std::shared_mutex> lock(items_mutex_);
    items_.clear();
    update_timestamp();
}

size_t MemorySession::get_item_count() const {
    std::shared_lock<std::shared_mutex> lock(items_mutex_);
    return items_.size();
}

void MemorySession::reserve_capacity(size_t capacity) {
    std::unique_lock<std::shared_mutex> lock(items_mutex_);
    items_.reserve(capacity);
}

size_t MemorySession::get_capacity() const {
    std::shared_lock<std::shared_mutex> lock(items_mutex_);
    return items_.capacity();
}

// SessionManager implementation
SessionManager::SessionManager(
    const std::string& default_db_path,
    const std::string& default_sessions_table,
    const std::string& default_messages_table
) : default_db_path_(default_db_path),
    default_sessions_table_(default_sessions_table),
    default_messages_table_(default_messages_table) {
}

std::shared_ptr<Session> SessionManager::get_session(const std::string& session_id) {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    return it != sessions_.end() ? it->second : nullptr;
}

std::shared_ptr<Session> SessionManager::create_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    auto session = std::make_shared<SQLiteSession>(
        session_id, default_db_path_, default_sessions_table_, default_messages_table_
    );
    sessions_[session_id] = session;
    return session;
}

std::shared_ptr<Session> SessionManager::get_or_create_session(const std::string& session_id) {
    auto session = get_session(session_id);
    return session ? session : create_session(session_id);
}

std::shared_ptr<SQLiteSession> SessionManager::create_sqlite_session(
    const std::string& session_id,
    const std::string& db_path
) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    std::string actual_db_path = db_path.empty() ? default_db_path_ : db_path;
    auto session = std::make_shared<SQLiteSession>(
        session_id, actual_db_path, default_sessions_table_, default_messages_table_
    );
    sessions_[session_id] = session;
    return session;
}

std::shared_ptr<MemorySession> SessionManager::create_memory_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    
    auto session = std::make_shared<MemorySession>(session_id);
    sessions_[session_id] = session;
    return session;
}

bool SessionManager::has_session(const std::string& session_id) const {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    return sessions_.find(session_id) != sessions_.end();
}

void SessionManager::remove_session(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    sessions_.erase(session_id);
}

void SessionManager::clear_all_sessions() {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    sessions_.clear();
}

std::vector<std::string> SessionManager::list_session_ids() const {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    std::vector<std::string> ids;
    ids.reserve(sessions_.size());
    for (const auto& [id, session] : sessions_) {
        ids.push_back(id);
    }
    return ids;
}

size_t SessionManager::get_session_count() const {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    return sessions_.size();
}

void SessionManager::set_default_tables(const std::string& sessions_table, const std::string& messages_table) {
    default_sessions_table_ = sessions_table;
    default_messages_table_ = messages_table;
}

// SessionFactory implementation
std::shared_ptr<Session> SessionFactory::create_sqlite_session(
    const std::string& session_id,
    const std::string& db_path,
    const std::string& sessions_table,
    const std::string& messages_table
) {
    return std::make_shared<SQLiteSession>(session_id, db_path, sessions_table, messages_table);
}

std::shared_ptr<Session> SessionFactory::create_memory_session(const std::string& session_id) {
    return std::make_shared<MemorySession>(session_id);
}

std::shared_ptr<Session> SessionFactory::create_default_session(const std::string& session_id) {
    return create_sqlite_session(session_id);
}

std::shared_ptr<Session> SessionFactory::create_session(
    const std::string& session_id,
    SessionType type,
    const std::map<std::string, std::string>& options
) {
    switch (type) {
    case SessionType::Memory:
        return create_memory_session(session_id);
    case SessionType::SQLite:
        {
            std::string db_path = ":memory:";
            std::string sessions_table = "agent_sessions";
            std::string messages_table = "agent_messages";
            
            auto db_it = options.find("db_path");
            if (db_it != options.end()) db_path = db_it->second;
            
            auto sessions_it = options.find("sessions_table");
            if (sessions_it != options.end()) sessions_table = sessions_it->second;
            
            auto messages_it = options.find("messages_table");
            if (messages_it != options.end()) messages_table = messages_it->second;
            
            return create_sqlite_session(session_id, db_path, sessions_table, messages_table);
        }
    case SessionType::Auto:
    default:
        return create_default_session(session_id);
    }
}

// Global session manager
static SessionManager global_session_manager;

SessionManager& get_global_session_manager() {
    return global_session_manager;
}

// Convenience functions
std::shared_ptr<Session> get_session(const std::string& session_id) {
    return get_global_session_manager().get_session(session_id);
}

std::shared_ptr<Session> create_session(const std::string& session_id) {
    return get_global_session_manager().create_session(session_id);
}

std::shared_ptr<Session> get_or_create_session(const std::string& session_id) {
    return get_global_session_manager().get_or_create_session(session_id);
}

} // namespace memory
} // namespace openai_agents