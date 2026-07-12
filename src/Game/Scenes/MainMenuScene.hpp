#pragma once

#include "Engine/Core/Scene.hpp"
#include "Game/Entities/AIPaddle.hpp"
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
        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;
        
        AIDifficulty m_SelectedDifficulty;
        bool m_WasEnterPressed;
    };

} // namespace Game
