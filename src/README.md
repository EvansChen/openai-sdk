# C++ Translation of OpenAI SDK Python Files

This directory contains the C++ translation of the Python OpenAI SDK located in the `python/` directory.

## Structure

The C++ code maintains the same directory structure as the Python version:

- `src/` - Main source directory (equivalent to `python/`)
  - `extensions/` - Extensions and additional components
  - `mcp/` - MCP (Model Context Protocol) related code
  - `memory/` - Memory management and session handling
  - `models/` - Model implementations and providers
  - `realtime/` - Real-time communication components
  - `tracing/` - Tracing and observability
  - `util/` - Utility functions and helpers
  - `voice/` - Voice processing components

## Key Files Translated

### Core Files
- `__init__.py` → `openai_agents.h` (main header)
- `version.py` → `version.h/cpp`
- `exceptions.py` → `exceptions.h/cpp`
- `usage.py` → `usage.h/cpp`
- `model_settings.py` → `model_settings.h/cpp`
- `function_schema.py` → `function_schema.h/cpp`

### Main Components
- `agent.py` → `agent.h/cpp`
- `tool.py` → `tool.h/cpp`
- `run.py` → `run.h/cpp`
- `result.py` → `result.h/cpp`
- `logger.py` → `logger.h/cpp`

### Models
- `models/interface.py` → `models/interface.h`
- `models/openai_chatcompletions.py` → `models/openai_chatcompletions.h`
- `models/openai_provider.py` → `models/openai_provider.h`
- `models/openai_responses.py` → `models/openai_responses.h`

## Usage

Include the main header to access all functionality:

```cpp
#include "openai_agents.h"

using namespace openai_agents;

// Set up OpenAI API key
set_default_openai_key("your-api-key");

// Create and use agents
auto agent = std::make_shared<Agent<std::string>>("my-agent");
auto result = agent->run("Hello, world!");
```

## Build Requirements

- C++17 or later
- Standard library with `<optional>`, `<variant>`, `<any>`
- JSON library (recommended: nlohmann/json)
- HTTP client library (recommended: libcurl or similar)

## Implementation Notes

The C++ version provides:

1. **Type Safety**: Strong typing using templates and type-safe containers
2. **Memory Management**: Smart pointers for automatic memory management
3. **Exception Safety**: RAII and proper exception handling
4. **Modern C++**: Uses C++17 features like `std::optional`, `std::variant`
5. **Cross-Platform**: Standard library only, no platform-specific dependencies

## Missing Features

Some Python-specific features are not directly translatable:

- Dynamic typing (replaced with `std::any` and templates)
- Python decorators (replaced with factory functions)
- Runtime introspection (simplified schema generation)
- Duck typing (replaced with explicit interfaces)

## Future Work

- Complete implementation of all stub methods
- Add JSON serialization/deserialization
- Implement HTTP client integration
- Add comprehensive test suite
- Performance optimizations