#include "Engine/Core/SceneManager.hpp"

namespace VECTOR {

    void SceneManager::PushScene(std::unique_ptr<Scene> scene) {
        if (scene) {
            scene->OnEnter();
            m_Scenes.push_back(std::move(scene));
        }
    }

    void SceneManager::PopScene() {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->OnExit();
            m_Scenes.pop_back();
        }
    }

    void SceneManager::ChangeScene(std::unique_ptr<Scene> scene) {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->OnExit();
            m_Scenes.pop_back();
        }

        if (scene) {
            scene->OnEnter();
            m_Scenes.push_back(std::move(scene));
        }
    }

    void SceneManager::Clear() {
        while (!m_Scenes.empty()) {
            m_Scenes.back()->OnExit();
            m_Scenes.pop_back();
        }
    }

    void SceneManager::Update(float deltaTime) {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->Update(deltaTime);
        }
    }

    void SceneManager::Render(Renderer* renderer) {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->Render(renderer);
        }
    }

} // namespace VECTOR
