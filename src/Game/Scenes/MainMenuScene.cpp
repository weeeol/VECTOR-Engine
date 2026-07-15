#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/UI/UIButton.hpp"
#include "Engine/Core/Logger.hpp"

namespace Game {

    MainMenuScene::MainMenuScene(int width, int height, VECTOR::InputManager* inputManager)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), 
          m_ParticleEmitter(300),
          m_State(MenuState::SelectingMode),
          m_SelectedMode(GameMode::FivePoint),
          m_SelectedDifficulty(AIDifficulty::Medium)
    {
    }

    void MainMenuScene::OnEnter() {
        VECTOR_LOG_INFO("MainMenuScene::OnEnter() started");
        CreateModeUI();
        VECTOR_LOG_INFO("MainMenuScene::OnEnter() finished");
    }

    void MainMenuScene::CreateModeUI() {
        m_UIManager.Clear();
        m_State = MenuState::SelectingMode;

        int btnWidth = 250;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        auto fivePointBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "5 Point Mode", [this]() {
            m_SelectedMode = GameMode::FivePoint;
            CreateDifficultyUI();
        });
        fivePointBtn->SetColors({50, 50, 150, 255}, {100, 100, 200, 255}, {255, 255, 255, 255});

        auto noLimitBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 60, btnWidth, btnHeight, "No Limit Mode", [this]() {
            m_SelectedMode = GameMode::Endless;
            CreateDifficultyUI();
        });
        noLimitBtn->SetColors({150, 50, 150, 255}, {200, 100, 200, 255}, {255, 255, 255, 255});

        m_UIManager.AddElement(fivePointBtn);
        m_UIManager.AddElement(noLimitBtn);
    }

    void MainMenuScene::CreateDifficultyUI() {
        m_UIManager.Clear();
        m_State = MenuState::SelectingDifficulty;

        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        auto easyBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "Easy AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Easy;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        easyBtn->SetColors({50, 100, 50, 255}, {100, 200, 100, 255}, {255, 255, 255, 255});

        auto medBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 60, btnWidth, btnHeight, "Medium AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Medium;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        medBtn->SetColors({100, 100, 50, 255}, {200, 200, 100, 255}, {255, 255, 255, 255});

        auto hardBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 120, btnWidth, btnHeight, "Hard AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Hard;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        hardBtn->SetColors({100, 50, 50, 255}, {200, 100, 100, 255}, {255, 255, 255, 255});

        m_UIManager.AddElement(easyBtn);
        m_UIManager.AddElement(medBtn);
        m_UIManager.AddElement(hardBtn);
    }

    void MainMenuScene::Update(float deltaTime) {
        if (rand() % 100 < 30) { 
            m_ParticleEmitter.Emit(rand() % m_Width, rand() % m_Height, 1, 150, 150, 255, 30.0f, 3.0f);
        }
        m_ParticleEmitter.Update(deltaTime);
        m_UIManager.Update(m_InputManager, deltaTime);
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        m_ParticleEmitter.Render(renderer);

        renderer->DrawText("PONG", m_Width / 2 - 50, m_Height / 2 - 150, 255, 255, 255, 48);
        
        if (m_State == MenuState::SelectingMode) {
            renderer->DrawText("Select Game Mode", m_Width / 2 - 100, m_Height / 2 - 70, 200, 200, 200, 24);
        } else {
            renderer->DrawText("Select Difficulty", m_Width / 2 - 90, m_Height / 2 - 70, 200, 200, 200, 24);
        }

        m_UIManager.Render(renderer);
    }

} // namespace Game
