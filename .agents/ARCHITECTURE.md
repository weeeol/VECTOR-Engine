# VECTOR Engine Architecture

## Overview
VECTOR (Velocity Engine for C++ Texturing and Object Rendering) is a custom 2D game engine built on top of the SDL2 hardware abstraction layer. It provides a rigid, modular framework for building 2D games, featuring a high-resolution fixed-timestep physics loop, dynamic rendering, and component-driven entities. The repository currently includes a fully playable implementation of Pong featuring dynamic AI and advanced collision physics.

## Folder Structure
```text
VECTOR-Engine/
├── assets/                 # Binary assets (fonts, sprites)
├── build/                  # CMake build artifacts [IGNORED]
├── src/
│   ├── main.cpp            # Application Entry Point
│   ├── Engine/             # Game-Agnostic Core Engine
│   │   ├── Core/           # App Loop, High-Res Timers, Logger
│   │   ├── Graphics/       # SDL2 Renderer, Texture caching, Fonts
│   │   ├── Input/          # Keyboard/Mouse state tracking
│   │   └── Math/           # Vector math, AABB, GameObject Base
│   └── Game/               # Game-Specific Implementation
│       ├── Core/           # PongGame logic, Game States, Scoring
│       └── Entities/       # Player Paddle, AI Paddle, Ball
```

## Core Data Flow & Game Loop
1. **Entry**: The application starts in `src/main.cpp`, which instantiates `Game::PongGame` (derived from `VECTOR::Application`) and calls `Run()`.
2. **The Loop**: `Application::Run()` manages a high-resolution fixed-timestep loop:
   - **Input**: Polls SDL events and updates the `InputManager`.
   - **Physics/Logic (Fixed Step)**: Calls `Update(deltaTime)` identically across hardware. The Game overrides this to update entities (`Paddle`, `Ball`), run AI logic, and resolve `CheckCollisions()`.
   - **Rendering (Variable Step)**: Calls `Render()`. The Game clears the screen, draws the entities via the `Renderer` subsystem, and presents the SDL buffer.
