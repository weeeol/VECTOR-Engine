#pragma once

#include "Engine/Core/Scene.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Engine/UI/UISystem.hpp"
#include <memory>
#include <functional>
#include <string>
#include <glm/glm.hpp>
namespace VECTOR {
    class InputManager;
}

namespace Game {

    class MainMenuScene : public VECTOR::Scene {
    public:
        MainMenuScene(int width, int height, VECTOR::InputManager* inputManager);
        ~MainMenuScene() override = default;

        void OnEnter() override;
        void OnResize(int width, int height) override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

        enum class MainMenuState {
            Main,
            Settings
        };

    private:
        void CreateUI();
        void CreateMainMenuUI();
        void CreateSettingsMenuUI();
        void ClearUI();

        VECTOR::Entity CreateButtonEntity(int x, int y, int width, int height, const std::string& text, 
                                          const glm::vec4& normalColor, const glm::vec4& hoverColor, 
                                          std::function<void()> onClick);
        VECTOR::Entity CreateSliderEntity(int x, int y, int width, int height, float initialValue, std::function<void(float)> onChange);

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;
        VECTOR::Registry m_Registry;
        std::unique_ptr<VECTOR::UISystem> m_UISystem;
        
        MainMenuState m_State = MainMenuState::Main;
        bool m_NeedsUIRefresh = false;
        AIDifficulty m_SelectedDifficulty;
    };

} // namespace Game
