#pragma once

#include "Engine/Core/Scene.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Engine/UI/UISystem.hpp"
#include <memory>

namespace VECTOR {
    class InputManager;
}

namespace Game {

    class MainMenuScene : public VECTOR::Scene {
    public:
        MainMenuScene(int width, int height, VECTOR::InputManager* inputManager);
        ~MainMenuScene() override = default;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void CreateUI();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;
        VECTOR::Registry m_Registry;
        std::unique_ptr<VECTOR::UISystem> m_UISystem;
        
        AIDifficulty m_SelectedDifficulty;
    };

} // namespace Game
