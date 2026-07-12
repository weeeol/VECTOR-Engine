# VECTOR Engine and Pong

**VECTOR** (Velocity Engine for C++ Texturing and Object Rendering) is a custom hardware-accelerated 2D C++ game engine built from scratch using SDL2. It is compiled as a standalone static library (`libVECTOR.a`) and includes a fully functional Pong game that demonstrates the engine's capabilities.

## Features
- **Strict Architecture**: Complete separation between the `VECTOR` engine library and the `Game` components.
- **Object System**: Formalized `VECTOR::GameObject` base class for game entities.
- **Fixed Time-Step**: Physics run at a constant rate regardless of frame rate.
- **High Definition**: 1280x720 window resolution.
- **Game States**: Includes Start, Playing, and Paused menus.
- **Text & Texture Rendering**: Uses `SDL2_ttf` for fonts and `SDL2_image` for loading sprites.
- **Dynamic AI Opponent**: Play against a computer-controlled AI with 3 difficulty levels!

## Requirements

* C++17 or C++20 compatible compiler
* CMake 3.10+
* SDL2, SDL2_ttf, and SDL2_image development libraries

## Controls
- **Player 1 (Left)**: `W` (Up) and `S` (Down)
- **Player 2 (Right)**: Controlled by AI
- **Pause**: `P` or `ESC`
- **Start / Select Difficulty**: `1` (Easy), `2` (Medium), `3` (Hard), then `ENTER` to start.

## Building

If using MSYS2 (UCRT64), ensure you have the dependencies installed:
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
pacman -S mingw-w64-ucrt-x86_64-SDL2_image
```

Then configure and build via CMake using PowerShell:
```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```
