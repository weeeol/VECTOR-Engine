cmake_minimum_required(VERSION 3.10)
project(PongGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal SDL2 Resolution: Try find_package first (e.g. for vcpkg/Mac/Linux native), fallback to PkgConfig (MSYS2)
find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)
find_package(SDL2_image QUIET)
find_package(SDL2_mixer QUIET)

if(SDL2_FOUND AND SDL2_ttf_FOUND AND SDL2_image_FOUND AND SDL2_mixer_FOUND)
    message(STATUS "Found SDL2 via find_package (vcpkg or system config)")
    set(VECTOR_SDL_LIBS SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer)
    set(VECTOR_SDL_INCLUDES "")
else()
    message(STATUS "Falling back to PkgConfig for SDL2 resolution (MSYS2/MinGW)")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PKG REQUIRED sdl2 SDL2_ttf SDL2_image SDL2_mixer)
    set(VECTOR_SDL_LIBS ${SDL2_PKG_LIBRARIES})
    set(VECTOR_SDL_INCLUDES ${SDL2_PKG_INCLUDE_DIRS})
    link_directories(${SDL2_PKG_LIBRARY_DIRS})
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
set(VECTOR_SOURCES
    src/Engine/Core/Application.cpp
    src/Engine/Core/Logger.cpp
    src/Engine/Core/ResourceManager.cpp
    src/Engine/Core/SceneManager.cpp
    src/Engine/Core/Window.cpp
    src/Engine/Core/SDLWindow.cpp
    src/Engine/Graphics/RendererAPI.cpp
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Graphics/Shader.cpp
    src/Engine/Graphics/Mesh.cpp
    src/Engine/Graphics/Texture2D.cpp
    src/Engine/Graphics/Buffer.cpp
    src/Engine/Graphics/VertexArray.cpp
    src/Engine/Graphics/Framebuffer.cpp
    src/Engine/Graphics/OpenGL/OpenGLContext.cpp
    src/Engine/Graphics/OpenGL/OpenGLRendererAPI.cpp
    src/Engine/Graphics/OpenGL/OpenGLShader.cpp
    src/Engine/Graphics/OpenGL/OpenGLTexture2D.cpp
    src/Engine/Graphics/OpenGL/OpenGLBuffer.cpp
    src/Engine/Graphics/OpenGL/OpenGLVertexArray.cpp
    src/Engine/Graphics/OpenGL/OpenGLFramebuffer.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Audio/AudioManager.cpp
    src/Engine/Audio/AudioAPI.cpp
    src/Engine/Audio/SDLMixerBackend.cpp
    src/Engine/Events/EventBus.cpp
    src/Engine/Physics/BulletPhysicsSystem.cpp
    src/Engine/UI/UISystem.cpp
)

option(VECTOR_USE_DIRECTX "Use DirectX 12 instead of OpenGL" OFF)

if(WIN32)
    list(APPEND VECTOR_SOURCES
        src/Engine/Core/Win32Window.cpp
        src/Engine/Audio/XAudio2Backend.cpp
        src/Engine/Graphics/DirectX/DirectX12Context.cpp
        src/Engine/Graphics/DirectX/DirectX12RendererAPI.cpp
        src/Engine/Graphics/DirectX/DirectX12Shader.cpp
        src/Engine/Graphics/DirectX/DirectX12Texture2D.cpp
        src/Engine/Graphics/DirectX/DirectX12Buffer.cpp
        src/Engine/Graphics/DirectX/DirectX12VertexArray.cpp
        src/Engine/Graphics/DirectX/DirectX12Framebuffer.cpp
    )
endif()

add_library(VECTOR STATIC ${VECTOR_SOURCES})
target_include_directories(VECTOR PUBLIC src ${VECTOR_SDL_INCLUDES} ${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIRS})
target_link_directories(VECTOR PUBLIC ${BULLET_ROOT_DIR}/${BULLET_LIBRARY_DIRS})
target_link_libraries(VECTOR PUBLIC ${VECTOR_SDL_LIBS} OpenGL::GL GLEW::GLEW glm::glm ${BULLET_LIBRARIES})

if(WIN32)
    target_link_libraries(VECTOR PUBLIC d3d12 dxgi d3dcompiler xaudio2_8 ole32)
    if(VECTOR_USE_DIRECTX)
        target_compile_definitions(VECTOR PUBLIC VECTOR_BUILD_DIRECTX)
    endif()
endif()

# ==============================================================================
# PONG GAME EXECUTABLE
# ==============================================================================
set(GAME_SOURCES
    src/Game/Core/PongGame.cpp
    src/Game/Scenes/MainMenuScene.cpp
    src/Game/Scenes/GameplayScene.cpp
    src/Game/Systems/GameSystems.cpp
    src/Game/Systems/CameraSystem.cpp
    src/Game/Systems/ShootingSystem.cpp
)

add_executable(Pong src/main.cpp ${GAME_SOURCES})

# Link the Pong game to the VECTOR engine
target_link_libraries(Pong PRIVATE VECTOR)

# Copy assets folder to the build directory unconditionally so it syncs new files (like bgm.wav)
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
)
add_dependencies(Pong CopyAssets)
