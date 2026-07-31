<div align="center">
  
  <h1 align="center">VECTOR Engine & Pong </h1>
  <p align="center">
    <strong>Velocity Engine for C++ Texturing and Object Rendering</strong>
  </p>
  
  <p align="center">
    <a href="https://github.com/libsdl-org/SDL"><img src="https://img.shields.io/badge/SDL3-Awesome-blue?style=for-the-badge&logo=c%2B%2B" alt="SDL3"></a>
    <a href="https://www.vulkan.org/"><img src="https://img.shields.io/badge/Vulkan-Primary-red?style=for-the-badge&logo=vulkan" alt="Vulkan"></a>
    <a href="https://github.com/bulletphysics/bullet3"><img src="https://img.shields.io/badge/Bullet3D-Physics-green?style=for-the-badge&logo=c%2B%2B" alt="Bullet3D"></a>
    <a href="https://cmake.org/"><img src="https://img.shields.io/badge/CMake-3.10+-red?style=for-the-badge&logo=cmake" alt="CMake"></a>
    <img src="https://img.shields.io/badge/C++-17%2F20-blueviolet?style=for-the-badge&logo=c%2B%2B" alt="C++17/20">
  </p>
</div>

<br/>

## 🚀 Overview

**VECTOR** is a custom hardware-accelerated 3D C++ game engine built from scratch. It utilizes **SDL3** for windowing and input, with **Vulkan** as its primary rendering backend (OpenGL is supported but deprecated). It is compiled as a standalone static library (`libVECTOR.a`/`VECTOR.lib`) and includes a fully functional 3D Pong game that demonstrates the engine's incredible capabilities.

---

## ✨ Features

- 📺 **Advanced Rendering (Vulkan)**: Features a modern Vulkan rendering backend with support for Shadow Mapping, Screen Space Ambient Occlusion (SSAO), Cubemaps/Skyboxes, and Post-Processing.
- 🎬 **3D Models & Skeletal Animation**: Integrated with **Assimp** for loading 3D meshes, materials, and skeletal animations.
- 💥 **3D Physics Engine**: Integrated **Bullet Physics** for 3D rigid body simulation, collision events, and continuous collision detection.
- 🧵 **Multithreading**: Custom `JobSystem` for parallel task execution and maximizing CPU utilization.
- 💾 **Save System & Settings**: Lightweight persistent data storage (`save.dat`) using JSON to track settings and high scores.
- 🛠️ **Developer Debug Tools**: Full integration of **Dear ImGui**, allowing developers to create custom floating debug panels (press `F3` to toggle the Engine Status & FPS window).
- 🕹️ **Game Modes**: Play in standard or endless modes.
- 🤖 **Dynamic AI Opponent**: Play against a computer-controlled AI.
- 🧬 **Data-Oriented ECS**: A custom Entity-Component System core framework, maximizing cache locality and decoupling logic from data.
- 🎵 **Audio & BGM Support**: Robust audio manager supporting `SDL_mixer` sound effects and endless `.mp3`/`.wav`/`.ogg` background music.
- 💎 **High Definition & Polish**: Includes neon UIs, dynamic window resizing, and smooth state transitions.
- 🚥 **Game States**: Includes Splash, Main Menu, Playing, Loading, and Paused menus with interactive mouse-driven UI.

---

## 🛠️ Requirements

Ensure you have the following installed before building:

* 🧰 **C++17 or C++20** compatible compiler
* 🏗️ **CMake 3.10+**
* 🎮 **SDL3, SDL3_ttf, SDL3_image, and SDL3_mixer** development libraries
* 🌋 **Vulkan SDK** & **VulkanMemoryAllocator**
* 📦 **Bullet Physics**, **GLM**, **Assimp**, **nlohmann_json**, **GLEW**, **OpenGL** (for legacy support)
* *Note: Dear ImGui is automatically fetched by CMake at build time.*

---

## 🎮 Controls

| Action | Key |
| :--- | :--- |
| **Player 1** | `W` (Up) and `S` (Down) |
| **Player 2** | *Controlled by AI* 🤖 |
| **Pause** | `P` or `ESC` |
| **Toggle Debug UI** | `F3` |
| **UI Interaction** | `Mouse` 🖱️ (Select Game Mode, Select Difficulty, Adjust Settings) |

---

## 🏗️ Building

This engine uses a **universal CMake configuration**. It is highly recommended to use a package manager like `vcpkg` to resolve all the 3D and media dependencies.

### 🌐 Option 1: Vcpkg (Universal / MSVC / Cross-Platform)
Using [vcpkg](https://vcpkg.io/):

```bash
vcpkg install sdl3 sdl3-ttf sdl3-image sdl3-mixer bullet3 glm assimp nlohmann-json vulkan-memory-allocator glew opengl
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

### 🪟 Option 2: MSYS2 / MinGW (Windows)
If using MSYS2 (UCRT64), install the dependencies (example):

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL3 mingw-w64-ucrt-x86_64-SDL3_ttf mingw-w64-ucrt-x86_64-SDL3_image mingw-w64-ucrt-x86_64-SDL3_mixer mingw-w64-ucrt-x86_64-bullet mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-assimp mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-vulkan-headers mingw-w64-ucrt-x86_64-vulkan-loader mingw-w64-ucrt-x86_64-vulkan-memory-allocator mingw-w64-ucrt-x86_64-glew
```

Then configure and build via CMake:

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" ..
cmake --build .
```

<div align="center">
  <br/>
  Made with ❤️ by the VECTOR Team.
</div>
