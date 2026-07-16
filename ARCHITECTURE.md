# VECTOR Engine Architecture

VECTOR (Velocity Engine for C++ Texturing and Object Rendering) is built with a highly decoupled, data-oriented architectural philosophy.

## High-Level Systems Overview

The engine separates low-level system integrations (rendering APIs, audio drivers, physics engines, OS windows) from high-level gameplay logic using a unified set of abstract interfaces and an Entity-Component System (ECS).

### 1. The Core Application Loop
`Application` is the central singleton that orchestrates initialization and the game loop. It performs:
- **Input Processing**: Interrogates the OS window for events.
- **Game Update**: Advances physics and ECS systems (`deltaTime`).
- **Rendering**: Dispatches the visual state to the rendering backend.

### 2. Dual Graphics Backend (Renderer API)
The rendering system abstracts away API-specific code behind interfaces (`Texture2D`, `Shader`, `Buffer`, `Framebuffer`, `RendererAPI`).
- **OpenGL 3.3 Core Profile**: Implements the graphics interfaces using `glew` and legacy GL state machines. Highly portable across Linux, Mac, and Windows (MinGW).
- **DirectX 12**: A modern low-level native Windows implementation utilizing `d3d12.h` and DXGI. Implements advanced explicit memory management, resource barriers, descriptor heaps (CBV/SRV/UAV), and command lists.
- **The Global `Renderer`**: Issues high-level draw calls (e.g., `DrawMesh`, `DrawUIText`). It holds no backend-specific code, instead calling virtual functions on the backend interfaces.

### 3. Dual Audio Backend
The `AudioManager` abstracts sound playback.
- **SDL2_mixer**: A lightweight, cross-platform audio mixing library.
- **XAudio2**: A low-level Windows-native API capable of 3D spatial audio and low latency. Uses RIFF/WAVE chunk parsing to load raw audio streams into source voices.

### 4. Entity Component System (ECS)
The engine utilizes a custom `Registry` to manage data-oriented entities.
- **Entities**: Lightweight IDs.
- **Components**: Plain-Old-Data (POD) structs storing state (e.g., `TransformComponent`, `RigidBodyComponent`, `CameraComponent`).
- **Systems**: Stateless logic blocks (e.g., `CameraSystem`, `ShootingSystem`) that iterate over specific sets of components in the `Registry`.
This structure prevents the "Diamond Problem" of Deep Object-Oriented inheritance hierarchies.

### 5. Physics Engine
Physics is delegated to **Bullet3**. 
- `BulletPhysicsSystem` steps the internal physics world. 
- The ECS seamlessly integrates Bullet objects: when an entity has a `RigidBodyComponent`, the physics system maps the Bullet `btRigidBody` transform directly into the entity's `TransformComponent`, syncing the physics and rendering pipelines automatically.

### 6. Windowing and Input
- `Window` acts as an abstract OS window.
- `Win32Window` handles raw Windows messages (DirectX mode).
- `SDLWindow` handles SDL events (OpenGL mode).
- Both backends feed generic inputs (Keys, Mouse Deltas) into a unified `InputManager` that normalizes OS-level scancodes and tracks state changes.

## Directory Structure
- `src/Engine/`: Low-level systems and interfaces.
  - `Core/`: Application, Logging, Windows, Asset Caching.
  - `ECS/`: Registry and common Components.
  - `Graphics/`: The Renderer, Shaders, Textures, and both OpenGL/DX12 backends.
  - `Audio/`: Sound Manager and SDL/XAudio2 implementations.
  - `Input/`: Keycodes and input tracking.
  - `Physics/`: Bullet integration.
- `src/Game/`: High-level gameplay code.
  - `Scenes/`: Game states (MainMenu, Gameplay).
  - `Systems/`: Game-specific logic (Shooting, Cameras).
- `assets/`: Runtime shaders, fonts, models, and sound files.
