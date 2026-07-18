# VECTOR Engine: 3D Physically Based Deferred FPS

**VECTOR** (Velocity Engine for C++ Texturing and Object Rendering) is a custom hardware-accelerated 3D C++ game engine built from scratch. Originally a 2D Pong Engine, it has been completely pivoted into a **3D Physically Based Deferred First-Person Shooter (FPS)** prototype supporting both **OpenGL 3.3 Core** and **Vulkan**, using Bullet Physics and SDL3!

## Features
- **Dual-Backend Hardware Rendering**: Supports both a modernized deferred OpenGL 3.3 pipeline (with G-Buffer and screen-space lighting) and a growing Vulkan backend.
- **Dynamic Sparse Set ECS**: A highly optimized Entity-Component System core framework with dynamic page scaling and **Entity ID Recycling**, maximizing CPU cache locality and preventing index exhaustion.
- **Physically Based Rendering (PBR)**: Upgraded from Blinn-Phong to a metallic-roughness workflow utilizing the **Cook-Torrance BRDF** (GGX normal distribution, Fresnel-Schlick, and Smith geometry shadow mapping).
- **Physical Volumetric Lighting**: Advanced fragment shaders that perform 3D Volumetric Raymarching directly against the shadow map to calculate real atmospheric scattering, rays, and fog.
- **Shadow Mapping**: Multi-pass rendering pipeline with a dedicated depth pre-pass and PCF (Percentage-Closer Filtering) soft shadows.
- **Interactive Dear ImGui Debugger**: A full-featured in-game developer overlay displaying dynamic system performance metrics, hardware specs, live camera FOV/speed/sensitivity adjustments, and real-time physics gravity control.
- **Asset Management & Shader Cache**: A centralized `ResourceManager` handling on-the-fly shader compilation, caching, and font loading. Core engine shaders are abstracted completely into the `assets/engine/shaders` library.
- **Physics Simulation**: Integrated Bullet3 for 3D rigid body dynamics, gravity, and continuous collision detection. Includes physics-based projectile shooting.
- **First-Person Camera**: Mouse-look and WASD movement systems that apply forces directly to the player's physical RigidBody.
- **2D UI Overlay**: Custom orthographic rendering layer built on top of the 3D pipeline for menus, crosshairs, and text.
- **Audio & BGM Support**: Robust audio manager supporting `SDL3_mixer` sound effects and background music with persistent volume configurations.

## Requirements

* C++17 or C++20 compatible compiler
* CMake 3.15+
* Vulkan SDK & OpenGL 3.3+ Compatible GPU
* GLEW and GLM
* SDL3, SDL3_ttf, SDL3_image, and SDL3_mixer development libraries
* Bullet Physics (Bullet3)
* Dear ImGui (configured with sdl3 and opengl3 backends)

## Controls
- **Movement**: `W`, `A`, `S`, `D`
- **Look**: Mouse Movement (relative mode)
- **Shoot**: Left Mouse Click
- **Toggle Debugger Overlay**: `F3`
- **Pause/In-Game Menu**: `ESC`

## Building

This engine uses a **universal CMake configuration**. 

### Option 1: Vcpkg (Universal / MSVC / Cross-Platform)
If you are using MSVC or a standard CMake environment, it is highly recommended to use [vcpkg](https://vcpkg.io/):
```bash
vcpkg install sdl3 sdl3-ttf sdl3-image sdl3-mixer bullet3 glew glm imgui[sdl3-binding,opengl3-binding]
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

### Option 2: MSYS2 / MinGW (Windows)
If using MSYS2 (UCRT64), install the dependencies:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL3 mingw-w64-ucrt-x86_64-SDL3_ttf mingw-w64-ucrt-x86_64-SDL3_image mingw-w64-ucrt-x86_64-SDL3_mixer mingw-w64-ucrt-x86_64-bullet mingw-w64-ucrt-x86_64-glew mingw-w64-ucrt-x86_64-glm
```

Then configure and build via CMake:
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```
