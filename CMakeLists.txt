cmake_minimum_required(VERSION 3.10)
project(PongGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 components.
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

# ==============================================================================
# VECTOR ENGINE LIBRARY
# ==============================================================================
set(VECTOR_SOURCES
    src/Engine/Application.cpp
    src/Engine/Renderer.cpp
    src/Engine/InputManager.cpp
    src/Engine/Texture.cpp
)

add_library(VECTOR STATIC ${VECTOR_SOURCES})
target_include_directories(VECTOR PUBLIC src)
target_link_libraries(VECTOR PUBLIC SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image::SDL2_image)

# ==============================================================================
# PONG GAME EXECUTABLE
# ==============================================================================
set(GAME_SOURCES
    src/Game/PongGame.cpp
    src/Game/Paddle.cpp
    src/Game/AgentPaddle.cpp
    src/Game/Ball.cpp
)

add_executable(Pong src/main.cpp ${GAME_SOURCES})

# Link the Pong game to the VECTOR engine
target_link_libraries(Pong PRIVATE VECTOR)

# Copy assets folder to the build directory so the executable can find the font and images
add_custom_command(TARGET Pong POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Pong>/assets"
)
