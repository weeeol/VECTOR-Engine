# VECTOR Engine - AI Agent Guidelines

## Tech Stack
- **Language**: C++17 / C++20
- **Build System**: CMake (3.10+)
- **Graphics/Hardware API**: OpenGL 3.3 Core, SDL2, SDL2_ttf, SDL2_image
- **Physics**: Bullet3
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
- **Do not use raw OpenGL calls in game logic**: Always use `VECTOR::Mesh`, `VECTOR::Shader`, and `VECTOR::Texture2D` instead of manually generating VAOs/VBOs in scenes.
- **Do not use `SDL_GetTicks()` for physics**: Always use the high-resolution `SDL_GetPerformanceCounter()` for `deltaTime` calculations to prevent micro-stuttering.
