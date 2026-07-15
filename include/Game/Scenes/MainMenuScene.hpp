#pragma once

#include "Engine/Core/Scene.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include <memory>

namespace VECTOR {
    class InputManager;
}

namespace Game {

    enum class MenuState { SelectingMode, SelectingDifficulty };

    class MainMenuScene : public VECTOR::Scene {
    public:
        MainMenuScene(int width, int height, VECTOR::InputManager* inputManager);
        ~MainMenuScene() override = default;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void CreateModeUI();
        void CreateDifficultyUI();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;
        VECTOR::UIManager m_UIManager;
        VECTOR::ParticleEmitter m_ParticleEmitter;
        
        MenuState m_State;
        GameMode m_SelectedMode;
        AIDifficulty m_SelectedDifficulty;
    };

} // namespace Game
