cmake_minimum_required(VERSION 3.10)
project(PongGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 QUIET)
find_package(SDL3_ttf QUIET)
find_package(SDL3_image QUIET)
find_package(SDL3_mixer QUIET)

if(SDL3_FOUND AND SDL3_ttf_FOUND AND SDL3_image_FOUND AND SDL3_mixer_FOUND)
    message(STATUS "Found SDL3 via find_package (vcpkg or system config)")
    set(VECTOR_SDL_LIBS SDL3::SDL3 SDL3_ttf::SDL3_ttf $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static> $<IF:$<TARGET_EXISTS:SDL3_mixer::SDL3_mixer>,SDL3_mixer::SDL3_mixer,SDL3_mixer::SDL3_mixer-static>)
    set(VECTOR_SDL_INCLUDES "")
else()
    message(STATUS "Falling back to PkgConfig for SDL3 resolution (MSYS2/MinGW)")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3_PKG REQUIRED sdl3 sdl3-ttf sdl3-image sdl3-mixer)
    set(VECTOR_SDL_LIBS ${SDL3_PKG_LIBRARIES})
    set(VECTOR_SDL_INCLUDES ${SDL3_PKG_INCLUDE_DIRS})
    link_directories(${SDL3_PKG_LIBRARY_DIRS})
endif()

find_package(box2d CONFIG REQUIRED)

# ==============================================================================
# DEAR IMGUI
# ==============================================================================
include(FetchContent)
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/heads/docking.zip
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

set(VECTOR_SOURCES
    src/Engine/Core/Application.cpp
    src/Engine/Core/Logger.cpp
    src/Engine/Core/ResourceManager.cpp
    src/Engine/Core/SceneManager.cpp
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Graphics/Texture.cpp
    src/Engine/Graphics/Animator.cpp
    src/Engine/Graphics/ParticleSystem.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Audio/AudioManager.cpp
    src/Engine/Events/EventBus.cpp
    src/Engine/Physics/Box2DPhysicsSystem.cpp
    src/Engine/UI/UIButton.cpp
    src/Engine/UI/UIManager.cpp
    src/Engine/UI/UISlider.cpp
    ${IMGUI_SOURCES}
)

add_library(VECTOR STATIC ${VECTOR_SOURCES})
target_include_directories(VECTOR PUBLIC include ${VECTOR_SDL_INCLUDES} ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(VECTOR PUBLIC ${VECTOR_SDL_LIBS} box2d::box2d)

# ==============================================================================
# PONG GAME EXECUTABLE
# ==============================================================================
set(GAME_SOURCES
    src/Game/Core/PongGame.cpp
    src/Game/Core/SaveManager.cpp
    src/Game/Scenes/SplashScene.cpp
    src/Game/Scenes/MainMenuScene.cpp
    src/Game/Scenes/GameplayScene.cpp
    src/Game/Systems/GameSystems.cpp
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
