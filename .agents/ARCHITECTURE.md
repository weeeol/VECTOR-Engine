# VECTOR Engine Architecture

## Overview
VECTOR (Velocity Engine for C++ Texturing and Object Rendering) is a custom 2D game engine built on top of the SDL2 hardware abstraction layer and Box2D (v3). It provides a rigid, modular framework for building 2D games, featuring a high-resolution fixed-timestep physics loop, dynamic rendering, scene management, event messaging, and component-driven entities. The repository currently includes a fully playable implementation of Pong featuring dynamic AI and advanced rigid body collision physics.

## Folder Structure
```text
VECTOR-Engine/
├── assets/                 # Binary assets (fonts, sprites, audio)
├── build/                  # CMake build artifacts [IGNORED]
├── include/            # C++ Header files
│   ├── Engine/         # Game-Agnostic Core Engine Headers
│   │   ├── Audio/      
│   │   ├── Core/       
│   │   ├── ECS/        
│   │   ├── Events/     
│   │   ├── Graphics/   
│   │   ├── Input/      
│   │   ├── Math/       
│   │   ├── Physics/    
│   │   └── UI/         
│   └── Game/           # Game-Specific Headers
│       ├── Core/       
│       ├── Events/     
│       ├── Scenes/     
│       └── Systems/    
├── src/                # C++ Source files
│   ├── main.cpp        # Application Entry Point
│   ├── Engine/         # Engine Implementation
│   │   ├── Audio/      # SDL_mixer integration (AudioManager, BGM)
│   │   ├── Core/       # App Loop, High-Res Timers, Logger, SceneManager, ResourceManager
│   │   ├── ECS/        # Custom Entity-Component System (Registry, Components)
│   │   ├── Events/     # EventBus for decoupled messaging
│   │   ├── Graphics/   # Renderer, Texture caching, Fonts, ParticleSystem, Animator
│   │   ├── Input/      # Keyboard/Mouse state tracking
│   │   ├── Math/       # Vector math
│   │   ├── Physics/    # Box2D physics integration
│   │   └── UI/         # UIManager, UIElement, UIButton
│   └── Game/           # Game-Specific Implementation
│       ├── Core/       # PongGame application logic, GameComponents (Data)
│       ├── Events/     # Game-specific events (ScoreEvent, CollisionEvent)
│       ├── Scenes/     # MainMenuScene, GameplayScene
│       └── Systems/    # Modular ECS Systems (PlayerInput, AI, Physics, BallMechanics)
```

## Core Data Flow & Game Loop
1. **Entry**: The application starts in `src/main.cpp`, which instantiates `Game::PongGame` (derived from `VECTOR::Application`) and calls `Run()`.
2. **Initialization**: `PongGame::OnInit()` subscribes to engine events, starts the BGM, and pushes the initial `MainMenuScene` onto the `SceneManager` stack.
3. **The Loop**: `Application::Run()` manages a high-resolution fixed-timestep loop:
   - **Input**: Polls SDL events and updates the `InputManager` (keyboard state, mouse position, and clicks).
   - **Physics/Logic (Fixed Step)**: Calls `Update(deltaTime)` identically across hardware. The `SceneManager` routes this to the active `Scene`. The `GameplayScene` owns a list of decoupled `VECTOR::System` objects (`AISystem`, `PhysicsSystem`, etc.) and invokes them. Systems query the **ECS Registry** using a zero-allocation lambda-based `View()` iterator.
   - **Deferred State Changes**: Scene transitions (e.g. from UI buttons) are deferred to the end of the update loop to prevent use-after-free bugs.
   - **Rendering (Variable Step)**: Calls `Render()`. The active `Scene` iterates over entities with `RenderComponent` and `TransformComponent` and delegates drawing to the `Renderer`.
4. **Shutdown**: On exit, the `SceneManager` is cleared before `SDL_Quit()` is invoked, ensuring all textures and audio chunks are safely destroyed while SDL is still initialized.
