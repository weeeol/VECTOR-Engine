# VECTOR Engine - AI Agent Guidelines

## Tech Stack
- **Language**: C++17 / C++20
- **Build System**: CMake (3.10+)
- **Graphics/Hardware API**: SDL2, SDL2_image, SDL2_ttf
- **Architecture**: Object-Oriented, Fixed-Timestep Game Loop

## Coding Conventions
1. **Namespaces**: 
   - All engine-level code MUST live inside the `VECTOR::` namespace.
   - All game-specific code MUST live inside the `Game::` namespace.
2. **Naming Rules**:
   - **Classes & Structs**: `PascalCase` (e.g., `GameObject`, `InputManager`).
   - **Methods/Functions**: `PascalCase` (e.g., `Update()`, `Render()`, `GetSpeed()`).
   - **Member Variables**: MUST be prefixed with `m_` and use `PascalCase` (e.g., `m_Velocity`, `m_Score1`).
   - **Local Variables**: `camelCase` (e.g., `deltaTime`, `isEnterPressed`).
3. **Memory Management**: 
   - Favor Modern C++ paradigms. Use `std::unique_ptr` and `std::shared_ptr` over raw pointers for ownership. 
   - Raw pointers should only be used for non-owning references (e.g., passing `Renderer* renderer`).
4. **Logging**: 
   - NEVER use `std::cout` or `std::cerr`. 
   - ALWAYS use the built-in macros: `VECTOR_LOG_INFO()`, `VECTOR_LOG_WARN()`, `VECTOR_LOG_ERROR()`.

## Explicit "DO NOT" Instructions
- **Do not mix domains**: Engine logic (`src/Engine/`) must never `#include` or depend on Game logic (`src/Game/`). The Engine must remain completely game-agnostic.
- **Do not load resources every frame**: Any texture, font, or audio file must be loaded once during initialization and cached (e.g., the `m_Fonts` map in `Renderer.cpp`).
- **Do not use `SDL_GetTicks()` for physics**: Always use the high-resolution `SDL_GetPerformanceCounter()` for `deltaTime` calculations to prevent micro-stuttering.

## Modularity Rules
- **Keep Functions Small**: No single function or constructor should exceed 50 lines of code. Extract logic into private helper methods.
- **Entity Factories**: Use Factory classes (e.g., `EntityFactory`) for Entity assembly instead of placing creation logic directly in Scenes.
- **Utility Classes**: Move all reusable math, physics, or common setup functions to Utility files (e.g., `PhysicsUtils.cpp`).
- **Separation of Concerns**: Keep Game State logic separated from rendering and initialization logic.
