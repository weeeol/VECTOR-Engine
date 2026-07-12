#include "MainMenuScene.hpp"
#include "GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace Game {

    MainMenuScene::MainMenuScene(int width, int height, VECTOR::InputManager* inputManager)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), 
          m_SelectedDifficulty(AIDifficulty::Medium), m_WasEnterPressed(false)
    {
    }

    void MainMenuScene::OnEnter() {
        m_WasEnterPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_RETURN);
    }

    void MainMenuScene::Update(float deltaTime) {
        if (m_InputManager->IsKeyPressed(SDL_SCANCODE_1)) {
            m_SelectedDifficulty = AIDifficulty::Easy;
        } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_2)) {
            m_SelectedDifficulty = AIDifficulty::Medium;
        } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_3)) {
            m_SelectedDifficulty = AIDifficulty::Hard;
        }

        bool isEnterPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_RETURN);
        if (isEnterPressed && !m_WasEnterPressed) {
            // Push the Gameplay Scene and pop the MainMenu scene
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
            return;
        }

        m_WasEnterPressed = isEnterPressed;
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->DrawText("PONG", m_Width / 2 - 50, m_Height / 2 - 150, 255, 255, 255, 48);

        std::string easyPrefix = (m_SelectedDifficulty == AIDifficulty::Easy) ? "> " : "  ";
        std::string medPrefix  = (m_SelectedDifficulty == AIDifficulty::Medium) ? "> " : "  ";
        std::string hardPrefix = (m_SelectedDifficulty == AIDifficulty::Hard) ? "> " : "  ";

        renderer->DrawText(easyPrefix + "Press 1 for EASY AI", m_Width / 2 - 170, m_Height / 2 - 40, 0, 255, 0, 24);
        renderer->DrawText(medPrefix + "Press 2 for MEDIUM AI", m_Width / 2 - 170, m_Height / 2, 255, 255, 0, 24);
        renderer->DrawText(hardPrefix + "Press 3 for HARD AI", m_Width / 2 - 170, m_Height / 2 + 40, 255, 0, 0, 24);
        
        renderer->DrawText("Press ENTER to Start", m_Width / 2 - 150, m_Height / 2 + 120, 255, 255, 255, 24);
    }

} // namespace Game
