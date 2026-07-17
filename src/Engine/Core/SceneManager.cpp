#include "Engine/Core/SceneManager.hpp"

namespace VECTOR {

    void SceneManager::PushScene(std::unique_ptr<Scene> scene) {
        if (scene) {
            scene->OnEnter();
            m_Scenes.push_back(std::move(scene));
        }
    }

    void SceneManager::PopScene() {
        m_NeedsPop = true;
    }

    void SceneManager::ChangeScene(std::unique_ptr<Scene> scene) {
        m_NeedsPop = true;
        m_NextScene = std::move(scene);
    }

    void SceneManager::Clear() {
        while (!m_Scenes.empty()) {
            m_Scenes.back()->OnExit();
            m_Scenes.pop_back();
        }
    }

    void SceneManager::Update(float deltaTime) {
        if (m_NeedsPop) {
            if (!m_Scenes.empty()) {
                m_Scenes.back()->OnExit();
                m_Scenes.pop_back();
            }
            m_NeedsPop = false;
        }

        if (m_NextScene) {
            m_NextScene->OnEnter();
            m_Scenes.push_back(std::move(m_NextScene));
        }

        if (!m_Scenes.empty()) {
            m_Scenes.back()->Update(deltaTime);
        }
    }

    void SceneManager::Render(Renderer* renderer) {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->Render(renderer);
        }
    }

    void SceneManager::OnImGuiRender() {
        if (!m_Scenes.empty()) {
            m_Scenes.back()->OnImGuiRender();
        }
    }

} // namespace VECTOR
