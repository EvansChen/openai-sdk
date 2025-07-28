#pragma once

/**
 * Memory module - Session management and conversation history
 * 
 * This module provides session management capabilities for maintaining
 * conversation history between agent interactions. It includes both
 * in-memory and persistent SQLite-based storage options.
 */

// Core session interface and implementations
#include "session.h"
#include "util.h"
#include "examples.h"

namespace openai_agents {
namespace memory {

// Re-export key classes for convenience
using Session = Session;
using SessionBase = SessionBase;
using SQLiteSession = SQLiteSession;
using MemorySession = MemorySession;
using SessionManager = SessionManager;
using SessionFactory = SessionFactory;

// Utility classes
using SessionUtils = SessionUtils;
using SessionEventListener = SessionEventListener;
using ObservableSessionManager = ObservableSessionManager;
using SessionCache = SessionCache;
using CachedSessionManager = CachedSessionManager;
using SessionPool = SessionPool;

// Type aliases for compatibility
using TResponseInputItem = std::shared_ptr<Item>;

// Convenience functions
using get_global_session_manager = get_global_session_manager;
using get_session = get_session;
using create_session = create_session;
using get_or_create_session = get_or_create_session;

} // namespace memory
} // namespace openai_agents