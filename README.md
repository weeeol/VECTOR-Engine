# VECTOR Engine and Pong

**VECTOR** (Velocity Engine for C++ Texturing and Object Rendering) is a custom hardware-accelerated 2D C++ game engine built from scratch using SDL3. It is compiled as a standalone static library (`libVECTOR.a`) and includes a fully functional Pong game that demonstrates the engine's capabilities.

## Features
- **Post-Processing & Juice**: Engine support for render targets enables dynamic CRT scanline overlays. Gameplay features impact screen shake, particle explosions, and velocity-based ball squish & stretch.
- **Save System & Settings**: Lightweight persistent data storage (`save.dat`) to track Player and AI High Scores, Audio Volume preferences, Window Mode (Fullscreen/Borderless), and Resolution across sessions.
- **Developer Debug Tools**: Full integration of **Dear ImGui**, allowing developers to create custom floating debug panels (press `F3` to toggle the Engine Status & FPS window).
- **Game Modes**: Play in standard "5 Point Mode" or "Endless Mode".
- **Dynamic AI Opponent**: Play against a computer-controlled AI with 3 difficulty levels!
- **Data-Oriented ECS**: A custom Entity-Component System core framework, maximizing cache locality and decoupling logic from data.
- **Audio & BGM Support**: Robust audio manager supporting `SDL_mixer` sound effects and endless `.mp3`/`.wav`/`.ogg` background music.
- **Sprite Animations**: Core `Animator` subsystem for slicing and animating spritesheets.
- **Physics Engine**: Integrated Box2D v3 for rigid body simulation, collision events, and continuous collision detection (CCD).
- **Fixed Time-Step**: Physics run at a constant rate regardless of frame rate.
- **High Definition & Polish**: Includes a multi-stage Splash Screen upon boot, slick glowing neon UIs, and dynamic window resizing.
- **Game States**: Includes Splash, Main Menu, Playing, and Paused menus with interactive mouse-driven UI.

## Requirements

* C++17 or C++20 compatible compiler
* CMake 3.10+
* SDL3, SDL3_ttf, SDL3_image, and SDL3_mixer development libraries
* Box2D (v3.0+)
* *Note: Dear ImGui is automatically fetched by CMake at build time.*

## Controls
- **Player 1 (Left)**: `W` (Up) and `S` (Down)
- **Player 2 (Right)**: Controlled by AI
- **Pause**: `P` or `ESC`
- **Toggle Debug UI**: `F3`
- **UI Interaction**: Use the `Mouse` to click buttons (Select Game Mode, Select Difficulty, Adjust Settings).

## Building

This engine uses a **universal CMake configuration**. It first attempts to find SDL2 using standard `find_package` (ideal for `vcpkg` or macOS/Linux package managers), and gracefully falls back to `pkg-config` (ideal for MSYS2 on Windows).

### Option 1: Vcpkg (Universal / MSVC / Cross-Platform)
If you are using MSVC or a standard CMake environment, it is highly recommended to use [vcpkg](https://vcpkg.io/):
```bash
vcpkg install sdl3 sdl3-ttf sdl3-image sdl3-mixer box2d
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

### Option 2: MSYS2 / MinGW (Windows)
If using MSYS2 (UCRT64), install the dependencies:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL3 mingw-w64-ucrt-x86_64-SDL3_ttf mingw-w64-ucrt-x86_64-SDL3_image mingw-w64-ucrt-x86_64-SDL3_mixer mingw-w64-ucrt-x86_64-box2d
```

Then configure and build via CMake:
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```
