#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Engine/UI/UISystem.hpp"
#include <future>
#include <memory>

#include "Game/Scenes/GameplayScene.hpp" // For AIDifficulty

namespace VECTOR {
    class InputManager;
}

namespace Game {

    class LoadingScene : public VECTOR::Scene {
    public:
        LoadingScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty);
        ~LoadingScene() override;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;
        void OnResize(int width, int height) override;

    private:
        void CreateUI();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_UIRegistry;
        std::unique_ptr<VECTOR::UISystem> m_UISystem;

        std::future<void> m_LoadingFuture;
        bool m_LoadingStarted = false;
        
        VECTOR::Entity m_ProgressBarEntity;
        float m_LoadingProgress = 0.0f;
        
        AIDifficulty m_Difficulty;
    };

} // namespace Game
