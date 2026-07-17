# VECTOR Engine Architecture

## Overview
VECTOR (Velocity Engine for C++ Texturing and Object Rendering) is a custom 3D game engine built on top of the SDL2 hardware abstraction layer, OpenGL 3.3 Core Profile, and Bullet3 Physics. Originally a 2D engine, it has been pivoted into a robust framework for building 3D games. It features variable-timestep physics integration, dynamic 3D rendering with custom shaders and meshes, scene management, event messaging, and a data-oriented component-driven entity system. The repository currently includes a 3D First-Person Shooter (FPS) prototype showcasing mouse-look mechanics, rigid body movement, and 2D UI overlays.

## Folder Structure
```text
VECTOR-Engine/
├── assets/                 # Binary assets (fonts, models, textures, audio)
├── include/                # Public Headers for Engine and Game
│   ├── Engine/             # Game-Agnostic Core Engine headers
│   └── Game/               # Game-Specific Implementation headers
├── src/                    # Source files
│   ├── main.cpp            # Application Entry Point
│   ├── Engine/             # Game-Agnostic Core Engine sources
│   └── Game/               # Game-Specific Implementation sources
```

## Core Data Flow & Game Loop
1. **Entry**: The application starts in `src/main.cpp`, which instantiates `Game::PongGame` (derived from `VECTOR::Application`) and calls `Run()`.
2. **Initialization**: `PongGame::OnInit()` subscribes to engine events, starts the BGM, and pushes the initial `MainMenuScene` onto the `SceneManager` stack.
3. **The Loop**: `Application::Run()` manages the core game loop:
   - **Input**: Polls SDL events and updates the `InputManager` (keyboard state, mouse position, clicks, and relative mouse delta).
   - **Physics/Logic (Variable Step)**: Calculates `deltaTime` and calls `Update(deltaTime)`. The `SceneManager` routes this to the active `Scene`. The `GameplayScene` owns a list of decoupled `VECTOR::System` objects (`CameraSystem`, `ShootingSystem`, `BulletPhysicsSystem`, etc.) and invokes them. 
   - **ECS Iteration**: Systems query the **ECS Registry** using a zero-allocation lambda-based `View()` iterator. Bullet physics steps forward using the actual frame time, ensuring smooth rigid body simulation.
   - **Deferred State Changes**: Scene transitions (e.g. from UI buttons) are deferred to the end of the update loop to prevent use-after-free bugs.
   - **Rendering**: Calls `Render()`. The active `Scene` iterates over entities with `RenderComponent`, `MeshComponent`, and `TransformComponent` and delegates drawing to the `Renderer`. The Renderer also handles drawing the 2D orthographic UI on top of the 3D scene (caching text textures for performance).
4. **Shutdown**: On exit, the `SceneManager` is cleared before `SDL_Quit()` is invoked, ensuring all textures, meshes, and audio chunks are safely destroyed while SDL and OpenGL are still initialized.
