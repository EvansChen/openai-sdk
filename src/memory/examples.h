#pragma once

/**
 * Memory module examples and usage demonstrations
 */

#include "session.h"
#include "util.h"
#include "../items.h"
#include <iostream>
#include <cassert>

namespace openai_agents {
namespace memory {
namespace examples {

// Example: Basic session usage
void basic_session_example() {
    std::cout << "=== Basic Session Example ===" << std::endl;
    
    // Create a memory session
    auto session = SessionFactory::create_memory_session("example_session_1");
    
    // Add some messages
    std::vector<std::shared_ptr<Item>> messages;
    messages.push_back(create_user_message("Hello, how are you?"));
    messages.push_back(create_assistant_message("I'm doing well, thank you! How can I help you today?"));
    messages.push_back(create_user_message("Can you help me with a programming question?"));
    
    session->add_items_sync(messages);
    
    std::cout << "Added " << messages.size() << " messages to session" << std::endl;
    std::cout << "Session now contains " << session->get_item_count() << " items" << std::endl;
    
    // Retrieve conversation history
    auto history = session->get_items_sync();
    std::cout << "\nConversation history:" << std::endl;
    for (size_t i = 0; i < history.size(); i++) {
        std::cout << i + 1 << ". " << history[i]->to_string() << std::endl;
    }
    
    // Pop the last message
    auto last_message = session->pop_item_sync();
    if (last_message) {
        std::cout << "\nPopped message: " << last_message->to_string() << std::endl;
        std::cout << "Session now contains " << session->get_item_count() << " items" << std::endl;
    }
    
    std::cout << std::endl;
}

// Example: SQLite session usage
void sqlite_session_example() {
    std::cout << "=== SQLite Session Example ===" << std::endl;
    
    // Create a file-based SQLite session
    auto session = SessionFactory::create_sqlite_session(
        "persistent_session_1", 
        "example_conversations.db"
    );
    
    // Add messages with metadata
    session->set_metadata("user_id", std::string("user123"));
    session->set_metadata("conversation_type", std::string("programming_help"));
    session->set_metadata("priority", 5);
    
    std::vector<std::shared_ptr<Item>> messages;
    messages.push_back(create_system_message("You are a helpful programming assistant."));
    messages.push_back(create_user_message("How do I sort a vector in C++?"));
    messages.push_back(create_assistant_message("You can use std::sort() from the <algorithm> header..."));
    
    session->add_items_sync(messages);
    
    std::cout << "Created persistent session with " << session->get_item_count() << " items" << std::endl;
    std::cout << "Session ID: " << session->get_session_id() << std::endl;
    std::cout << "Database path: " << session->get_db_path() << std::endl;
    
    // Show metadata
    auto metadata = session->get_metadata();
    std::cout << "\nSession metadata:" << std::endl;
    for (const auto& [key, value] : metadata) {
        std::cout << "  " << key << ": ";
        try {
            std::cout << std::any_cast<std::string>(value);
        } catch (const std::bad_any_cast&) {
            try {
                std::cout << std::any_cast<int>(value);
            } catch (const std::bad_any_cast&) {
                std::cout << "<unknown type>";
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
}

// Example: Session manager usage
void session_manager_example() {
    std::cout << "=== Session Manager Example ===" << std::endl;
    
    auto& manager = get_global_session_manager();
    
    // Create multiple sessions
    auto session1 = manager.get_or_create_session("chat_session_1");
    auto session2 = manager.get_or_create_session("chat_session_2");
    auto session3 = manager.get_or_create_session("chat_session_3");
    
    // Add different content to each session
    session1->add_items_sync({create_user_message("Session 1 message")});
    session2->add_items_sync({
        create_user_message("Session 2 message 1"),
        create_assistant_message("Session 2 response")
    });
    session3->add_items_sync({
        create_user_message("Session 3 message 1"),
        create_assistant_message("Session 3 response 1"),
        create_user_message("Session 3 message 2")
    });
    
    std::cout << "Created " << manager.get_session_count() << " sessions" << std::endl;
    
    // List all sessions
    auto session_ids = manager.list_session_ids();
    std::cout << "\nActive sessions:" << std::endl;
    for (const auto& id : session_ids) {
        auto session = manager.get_session(id);
        if (session) {
            std::cout << "  " << id << ": " << session->get_item_count() << " items" << std::endl;
        }
    }
    
    // Cleanup empty sessions
    size_t removed = SessionUtils::cleanup_empty_sessions(manager);
    std::cout << "\nRemoved " << removed << " empty sessions" << std::endl;
    std::cout << "Remaining sessions: " << manager.get_session_count() << std::endl;
    
    std::cout << std::endl;
}

// Example: Session analytics
void session_analytics_example() {
    std::cout << "=== Session Analytics Example ===" << std::endl;
    
    auto session = SessionFactory::create_memory_session("analytics_session");
    
    // Add various types of items
    std::vector<std::shared_ptr<Item>> items;
    items.push_back(create_user_message("Hello"));
    items.push_back(create_assistant_message("Hi there!"));
    items.push_back(create_user_message("What's the weather like?"));
    items.push_back(create_assistant_message("I don't have access to weather data."));
    
    session->add_items_sync(items);
    
    // Analyze the session
    auto stats = SessionUtils::analyze_session(session);
    
    std::cout << "Session Statistics:" << std::endl;
    std::cout << "  Total items: " << stats.total_items << std::endl;
    std::cout << "  Total size: " << stats.total_size_bytes << " bytes" << std::endl;
    std::cout << "  Item types:" << std::endl;
    
    for (const auto& [type, count] : stats.item_type_counts) {
        std::cout << "    " << type << ": " << count << std::endl;
    }
    
    std::cout << std::endl;
}

// Example: Session caching
void session_caching_example() {
    std::cout << "=== Session Caching Example ===" << std::endl;
    
    // Create a cached session manager
    CachedSessionManager cached_manager(
        10,  // cache size
        std::chrono::minutes(5),  // TTL
        ":memory:"  // in-memory SQLite
    );
    
    // Create and access sessions
    for (int i = 1; i <= 15; i++) {
        std::string session_id = "cached_session_" + std::to_string(i);
        auto session = cached_manager.get_or_create_session(session_id);
        session->add_items_sync({create_user_message("Message " + std::to_string(i))});
    }
    
    std::cout << "Created 15 sessions through cached manager" << std::endl;
    std::cout << "Cache hit rate: " << (cached_manager.get_cache_hit_rate() * 100) << "%" << std::endl;
    std::cout << "Cache size: " << cached_manager.get_cache().size() << std::endl;
    
    // Access some sessions again to demonstrate caching
    for (int i = 1; i <= 5; i++) {
        std::string session_id = "cached_session_" + std::to_string(i);
        auto session = cached_manager.get_session(session_id);
        // This should hit the cache for recent sessions
    }
    
    std::cout << "After accessing first 5 sessions again:" << std::endl;
    std::cout << "Cache hit rate: " << (cached_manager.get_cache_hit_rate() * 100) << "%" << std::endl;
    
    std::cout << std::endl;
}

// Example: Session pooling
void session_pooling_example() {
    std::cout << "=== Session Pooling Example ===" << std::endl;
    
    // Create a session pool
    SessionPool pool(
        SessionFactory::SessionType::Memory,
        {},  // no special options
        5    // max pool size
    );
    
    // Warm up the pool
    pool.warm_up(3);
    std::cout << "Warmed up pool with 3 sessions" << std::endl;
    std::cout << "Available sessions: " << pool.available_count() << std::endl;
    
    // Acquire some sessions
    std::vector<std::shared_ptr<Session>> acquired_sessions;
    for (int i = 1; i <= 4; i++) {
        std::string session_id = "pooled_session_" + std::to_string(i);
        auto session = pool.acquire_session(session_id);
        session->add_items_sync({create_user_message("Pooled message " + std::to_string(i))});
        acquired_sessions.push_back(session);
    }
    
    std::cout << "Acquired 4 sessions from pool" << std::endl;
    std::cout << "Active sessions: " << pool.active_count() << std::endl;
    std::cout << "Available sessions: " << pool.available_count() << std::endl;
    
    // Release some sessions back to pool
    for (int i = 1; i <= 2; i++) {
        std::string session_id = "pooled_session_" + std::to_string(i);
        pool.release_session(session_id);
    }
    
    std::cout << "Released 2 sessions back to pool" << std::endl;
    std::cout << "Active sessions: " << pool.active_count() << std::endl;
    std::cout << "Available sessions: " << pool.available_count() << std::endl;
    
    std::cout << std::endl;
}

// Run all examples
void run_all_examples() {
    std::cout << "OpenAI Agents Memory Module Examples" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    try {
        basic_session_example();
        sqlite_session_example();
        session_manager_example();
        session_analytics_example();
        session_caching_example();
        session_pooling_example();
        
        std::cout << "All examples completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error running examples: " << e.what() << std::endl;
    }
}

} // namespace examples
} // namespace memory
} // namespace openai_agents