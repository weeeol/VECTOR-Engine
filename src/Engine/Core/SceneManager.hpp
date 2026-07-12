#pragma once

#include "Engine/Core/Scene.hpp"
#include <memory>
#include <vector>

namespace VECTOR {

    class Renderer;

    class SceneManager {
    public:
        static SceneManager& Get() {
            static SceneManager instance;
            return instance;
        }

        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        // Push a new scene onto the stack
        void PushScene(std::unique_ptr<Scene> scene);

        // Pop the top scene from the stack
        void PopScene();

        // Change the current scene (pops current, pushes new)
        void ChangeScene(std::unique_ptr<Scene> scene);

        // Clear all scenes (useful for shutdown before SDL_Quit)
        void Clear();

        // Update the active scene
        void Update(float deltaTime);

        // Render the active scene
        void Render(Renderer* renderer);

    private:
        SceneManager() = default;
        ~SceneManager() = default;

        std::vector<std::unique_ptr<Scene>> m_Scenes;
    };

} // namespace VECTOR
