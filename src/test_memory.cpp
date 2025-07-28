#include "../memory/__init__.h"
#include <iostream>
#include <cassert>

using namespace openai_agents::memory;

int main() {
    std::cout << "Testing OpenAI Agents Memory Module" << std::endl;
    std::cout << "===================================" << std::endl;
    
    try {
        // Test basic memory session
        std::cout << "\n1. Testing basic memory session..." << std::endl;
        auto memory_session = SessionFactory::create_memory_session("test_memory");
        assert(memory_session != nullptr);
        assert(memory_session->get_session_id() == "test_memory");
        assert(memory_session->get_item_count() == 0);
        std::cout << "   ✓ Memory session created successfully" << std::endl;
        
        // Test SQLite session
        std::cout << "\n2. Testing SQLite session..." << std::endl;
        auto sqlite_session = SessionFactory::create_sqlite_session("test_sqlite", ":memory:");
        assert(sqlite_session != nullptr);
        assert(sqlite_session->get_session_id() == "test_sqlite");
        std::cout << "   ✓ SQLite session created successfully" << std::endl;
        
        // Test session manager
        std::cout << "\n3. Testing session manager..." << std::endl;
        auto& manager = get_global_session_manager();
        auto managed_session = manager.get_or_create_session("managed_test");
        assert(managed_session != nullptr);
        assert(manager.get_session_count() > 0);
        std::cout << "   ✓ Session manager working correctly" << std::endl;
        
        // Test session utilities
        std::cout << "\n4. Testing session utilities..." << std::endl;
        bool is_valid = SessionUtils::validate_session_id("valid_id_123");
        assert(is_valid == true);
        
        bool is_invalid = SessionUtils::validate_session_id("invalid id with spaces");
        assert(is_invalid == false);
        std::cout << "   ✓ Session utilities working correctly" << std::endl;
        
        // Test session cache
        std::cout << "\n5. Testing session cache..." << std::endl;
        SessionCache cache(5, std::chrono::minutes(1));
        cache.put("cache_test", memory_session);
        auto cached_session = cache.get("cache_test");
        assert(cached_session != nullptr);
        assert(cached_session->get_session_id() == "test_memory");
        std::cout << "   ✓ Session cache working correctly" << std::endl;
        
        std::cout << "\n✅ All memory module tests passed!" << std::endl;
        
        // Run examples if available
        std::cout << "\n6. Running memory module examples..." << std::endl;
        examples::run_all_examples();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with error: " << e.what() << std::endl;
        return 1;
    }
}