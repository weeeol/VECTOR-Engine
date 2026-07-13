# VECTOR Engine: FPS Prototype

**VECTOR** (Velocity Engine for C++ Texturing and Object Rendering) is a custom hardware-accelerated 3D C++ game engine built from scratch. Originally a 2D Pong Engine, it has been completely pivoted into a **3D First-Person Shooter (FPS)** prototype using OpenGL and Bullet Physics!

## Features
- **Data-Oriented ECS**: A custom Entity-Component System core framework, maximizing cache locality and decoupling logic from data.
- **3D Graphics Pipeline**: Fully abstracted modular OpenGL 3.3 Core Profile pipeline featuring custom `Shader`, `Mesh`, and `Texture2D` architecture.
- **Physics Simulation**: Integrated Bullet3 for 3D rigid body dynamics, gravity, and continuous collision detection. Includes physics-based projectile shooting.
- **First-Person Camera**: Mouse-look and WASD movement systems that apply forces directly to the player's physical RigidBody.
- **OpenGL 2D UI Overlay**: Custom orthographic rendering layer built on top of the 3D pipeline for menus, crosshairs, and text using `SDL2_ttf`.
- **Audio & BGM Support**: Robust audio manager supporting `SDL2_mixer` sound effects and endless background music.
- **AI Entities**: Simple enemy AI system that tracks the player in 3D space.

## Requirements

* C++17 or C++20 compatible compiler
* CMake 3.10+
* OpenGL 3.3+ Compatible GPU
* GLEW and GLM
* SDL2, SDL2_ttf, SDL2_image, and SDL2_mixer development libraries
* Bullet Physics (Bullet3)

## Controls
- **Movement**: `W`, `A`, `S`, `D`
- **Look**: Mouse Movement
- **Shoot**: Left Mouse Click
- **Pause**: `ESC`

## Building

This engine uses a **universal CMake configuration**. 

### Option 1: Vcpkg (Universal / MSVC / Cross-Platform)
If you are using MSVC or a standard CMake environment, it is highly recommended to use [vcpkg](https://vcpkg.io/):
```bash
vcpkg install sdl2 sdl2-ttf sdl2-image sdl2-mixer bullet3 glew glm
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

### Option 2: MSYS2 / MinGW (Windows)
If using MSYS2 (UCRT64), install the dependencies:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_mixer mingw-w64-ucrt-x86_64-bullet mingw-w64-ucrt-x86_64-glew mingw-w64-ucrt-x86_64-glm
```

Then configure and build via CMake:
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```
