cmake_minimum_required(VERSION 3.10)
project(PongGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

set(VECTOR_SDL_LIBS 
    SDL3::SDL3 
    SDL3_ttf::SDL3_ttf 
    $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
    $<IF:$<TARGET_EXISTS:SDL3_mixer::SDL3_mixer>,SDL3_mixer::SDL3_mixer,SDL3_mixer::SDL3_mixer-static>
)
set(VECTOR_SDL_INCLUDES "")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
set(VECTOR_SOURCES
    src/Engine/Core/Application.cpp
    src/Engine/Core/Logger.cpp
    src/Engine/Core/ResourceManager.cpp
    src/Engine/Core/SceneManager.cpp
    src/Engine/Graphics/RendererAPI.cpp
    src/Engine/Graphics/Renderer.cpp
    src/Engine/Graphics/Shader.cpp
    src/Engine/Graphics/Mesh.cpp
    src/Engine/Graphics/Texture2D.cpp
    src/Engine/Graphics/Frustum.cpp
    src/Engine/Graphics/Material.cpp
    src/Engine/Graphics/RenderQueue.cpp
    src/Engine/Graphics/UniformBufferObject.cpp
    src/Engine/Graphics/OpenGL/OpenGLTexture2D.cpp
    src/Engine/Graphics/OpenGL/OpenGLShader.cpp
    src/Engine/Graphics/OpenGL/OpenGLMesh.cpp
    src/Engine/Graphics/OpenGL/OpenGLUniformBuffer.cpp
    src/Engine/Graphics/OpenGL/OpenGLRenderer.cpp
    src/Engine/Input/InputManager.cpp
    src/Engine/Audio/AudioManager.cpp
    src/Engine/Events/EventBus.cpp
    src/Engine/Physics/BulletPhysicsSystem.cpp
    src/Engine/UI/UISystem.cpp
)

add_library(VECTOR STATIC ${VECTOR_SOURCES})
target_include_directories(VECTOR PUBLIC include src ${VECTOR_SDL_INCLUDES} ${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIRS})
target_link_directories(VECTOR PUBLIC ${BULLET_ROOT_DIR}/${BULLET_LIBRARY_DIRS})
target_link_libraries(VECTOR PUBLIC ${VECTOR_SDL_LIBS} OpenGL::GL GLEW::GLEW glm::glm ${BULLET_LIBRARIES} imgui::imgui)

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
