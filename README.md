# VECTOR Engine and Pong

**VECTOR** (Velocity Engine for C++ Texturing and Object Rendering) is a custom hardware-accelerated 2D C++ game engine built from scratch using SDL2. It is compiled as a standalone static library (`libVECTOR.a`) and includes a fully functional Pong game that demonstrates the engine's capabilities.

## Features
- **Data-Oriented ECS**: A custom Entity-Component System core framework, maximizing cache locality and decoupling logic from data.
- **Audio & BGM Support**: Robust audio manager supporting `SDL_mixer` sound effects and endless `.mp3`/`.wav`/`.ogg` background music.
- **Sprite Animations**: Core `Animator` subsystem for slicing and animating spritesheets.
- **Physics Engine**: Integrated Box2D v3 for rigid body simulation, collision events, and continuous collision detection (CCD).
- **Fixed Time-Step**: Physics run at a constant rate regardless of frame rate.
- **High Definition**: 1280x720 window resolution.
- **Game States**: Includes Start, Playing, and Paused menus.
- **Dynamic AI Opponent**: Play against a computer-controlled AI with 3 difficulty levels!

## Requirements

* C++17 or C++20 compatible compiler
* CMake 3.10+
* SDL2, SDL2_ttf, SDL2_image, and SDL2_mixer development libraries
* Box2D (v3.0+)

## Controls
- **Player 1 (Left)**: `W` (Up) and `S` (Down)
- **Player 2 (Right)**: Controlled by AI
- **Pause**: `P` or `ESC`
- **Start / Select Difficulty**: `1` (Easy), `2` (Medium), `3` (Hard), then `ENTER` to start.

## Building

This engine uses a **universal CMake configuration**. It first attempts to find SDL2 using standard `find_package` (ideal for `vcpkg` or macOS/Linux package managers), and gracefully falls back to `pkg-config` (ideal for MSYS2 on Windows).

### Option 1: Vcpkg (Universal / MSVC / Cross-Platform)
If you are using MSVC or a standard CMake environment, it is highly recommended to use [vcpkg](https://vcpkg.io/):
```bash
vcpkg install sdl2 sdl2-ttf sdl2-image sdl2-mixer box2d
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

### Option 2: MSYS2 / MinGW (Windows)
If using MSYS2 (UCRT64), install the dependencies:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_mixer mingw-w64-ucrt-x86_64-box2d
```

Then configure and build via CMake:
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```
