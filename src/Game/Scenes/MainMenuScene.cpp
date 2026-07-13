#include "MainMenuScene.hpp"
#include "GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/UI/UIButton.hpp"

namespace Game {

    MainMenuScene::MainMenuScene(int width, int height, VECTOR::InputManager* inputManager)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), 
          m_SelectedDifficulty(AIDifficulty::Medium)
    {
    }

    void MainMenuScene::OnEnter() {
        CreateUI();
    }

    void MainMenuScene::CreateUI() {
        m_UIManager.Clear();

        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        auto easyBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "Easy AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Easy;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        easyBtn->SetColors({50, 100, 50, 255}, {100, 200, 100, 255}, {255, 255, 255, 255});

        auto medBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 60, btnWidth, btnHeight, "Medium AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Medium;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        medBtn->SetColors({100, 100, 50, 255}, {200, 200, 100, 255}, {255, 255, 255, 255});

        auto hardBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 120, btnWidth, btnHeight, "Hard AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Hard;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        hardBtn->SetColors({100, 50, 50, 255}, {200, 100, 100, 255}, {255, 255, 255, 255});

        m_UIManager.AddElement(easyBtn);
        m_UIManager.AddElement(medBtn);
        m_UIManager.AddElement(hardBtn);
    }

    void MainMenuScene::Update(float deltaTime) {
        m_UIManager.Update(m_InputManager, deltaTime);
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->DrawText("FPS", m_Width / 2 - 40, m_Height / 2 - 150, 255, 255, 255, 48);
        renderer->DrawText("Select Difficulty", m_Width / 2 - 100, m_Height / 2 - 70, 200, 200, 200, 24);

        m_UIManager.Render(renderer);
    }

} // namespace Game
