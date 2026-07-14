#include "MainMenuScene.hpp"
#include "GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/UI/UIButton.hpp"
#include "Engine/UI/UISlider.hpp"
#include "Engine/Audio/AudioManager.hpp"
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

        auto playBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "Play", [this]() {
            m_SelectedDifficulty = AIDifficulty::Medium;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        playBtn->SetColors({50, 100, 50, 255}, {100, 200, 100, 255}, {255, 255, 255, 255});

        m_UIManager.AddElement(playBtn);

        auto volSlider = std::make_shared<VECTOR::UISlider>(startX, startY + 180, btnWidth, 20, 0.5f, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        });
        m_UIManager.AddElement(volSlider);
    }

    void MainMenuScene::Update(float deltaTime) {
        m_UIManager.Update(m_InputManager, deltaTime);
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->DrawText("FPS", m_Width / 2 - 40, m_Height / 2 - 150, 255, 255, 255, 48);
        renderer->DrawText("Volume", m_Width / 2 - 40, m_Height / 2 + 130, 200, 200, 200, 18);

        m_UIManager.Render(renderer);
    }

} // namespace Game
