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
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Graphics/Shader.cpp
    src/Engine/Graphics/Mesh.cpp
    src/Engine/Graphics/Texture2D.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Audio/AudioManager.cpp
    src/Engine/Events/EventBus.cpp
    src/Engine/Physics/BulletPhysicsSystem.cpp
    src/Engine/UI/UIButton.cpp
    src/Engine/UI/UIManager.cpp
    src/Engine/UI/UISlider.cpp
)

add_library(VECTOR STATIC ${VECTOR_SOURCES})
target_include_directories(VECTOR PUBLIC src ${VECTOR_SDL_INCLUDES} ${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIRS})
target_link_directories(VECTOR PUBLIC ${BULLET_ROOT_DIR}/${BULLET_LIBRARY_DIRS})
target_link_libraries(VECTOR PUBLIC ${VECTOR_SDL_LIBS} OpenGL::GL GLEW::GLEW glm::glm ${BULLET_LIBRARIES})

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
