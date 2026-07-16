#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Audio/AudioManager.hpp"
namespace Game {

    MainMenuScene::MainMenuScene(int width, int height, VECTOR::InputManager* inputManager)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), 
          m_SelectedDifficulty(AIDifficulty::Medium)
    {
        m_Registry.RegisterComponent<VECTOR::UIRectComponent>();
        m_Registry.RegisterComponent<VECTOR::UITextComponent>();
        m_Registry.RegisterComponent<VECTOR::UIButtonComponent>();
        m_Registry.RegisterComponent<VECTOR::UISliderComponent>();
        
        m_UISystem = std::make_unique<VECTOR::UISystem>(m_InputManager);
    }

    void MainMenuScene::OnEnter() {
        CreateUI();
    }

    void MainMenuScene::CreateUI() {
        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        VECTOR::Entity playBtn = m_Registry.CreateEntity();
        m_Registry.AddComponent(playBtn, VECTOR::UIRectComponent(startX, startY, btnWidth, btnHeight, glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f)));
        m_Registry.AddComponent(playBtn, VECTOR::UITextComponent("Play", 24, glm::vec4(1.0f)));
        
        // Center text in button
        auto& textC = m_Registry.GetComponent<VECTOR::UITextComponent>(playBtn);
        textC.offsetX = (btnWidth / 2) - (std::string("Play").length() * 6);
        textC.offsetY = (btnHeight / 2) - 12;

        m_Registry.AddComponent(playBtn, VECTOR::UIButtonComponent(
            glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f),
            glm::vec4(100/255.0f, 200/255.0f, 100/255.0f, 1.0f),
            [this]() {
                m_SelectedDifficulty = AIDifficulty::Medium;
                auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
                VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
            }
        ));

        VECTOR::Entity volSlider = m_Registry.CreateEntity();
        m_Registry.AddComponent(volSlider, VECTOR::UIRectComponent(startX, startY + 180, btnWidth, 20, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f)));
        m_Registry.AddComponent(volSlider, VECTOR::UISliderComponent(0.5f, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        }));
    }

    void MainMenuScene::Update(float deltaTime) {
        m_UISystem->Update(m_Registry, deltaTime);
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->BeginUI();
        
        // Static Text
        renderer->DrawUIText("FPS", m_Width / 2 - 40, m_Height / 2 - 150, glm::vec4(1.0f), 48);
        renderer->DrawUIText("Volume", m_Width / 2 - 40, m_Height / 2 + 130, glm::vec4(200/255.0f, 200/255.0f, 200/255.0f, 1.0f), 18);

        // UI Components
        m_Registry.View<VECTOR::UIRectComponent>([&](VECTOR::Entity entity) {
            auto& rect = m_Registry.GetComponent<VECTOR::UIRectComponent>(entity);
            if (!rect.isVisible) return;
            
            // Sliders require custom rendering for the track/knob
            if (m_Registry.HasComponent<VECTOR::UISliderComponent>(entity)) {
                auto& slider = m_Registry.GetComponent<VECTOR::UISliderComponent>(entity);
                renderer->DrawUIRect(rect.x, rect.y, rect.width, rect.height, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f));
                int fillWidth = (int)(rect.width * slider.value);
                renderer->DrawUIRect(rect.x, rect.y, fillWidth, rect.height, glm::vec4(50/255.0f, 200/255.0f, 50/255.0f, 1.0f));
                
                int knobWidth = 10;
                int knobHeight = rect.height + 10;
                int knobX = rect.x + fillWidth - (knobWidth / 2);
                int knobY = rect.y - 5;
                renderer->DrawUIRect(knobX, knobY, knobWidth, knobHeight, glm::vec4(1.0f));
            } else {
                renderer->DrawUIRect(rect.x, rect.y, rect.width, rect.height, rect.color);
            }
            
            if (m_Registry.HasComponent<VECTOR::UITextComponent>(entity)) {
                auto& text = m_Registry.GetComponent<VECTOR::UITextComponent>(entity);
                renderer->DrawUIText(text.text, rect.x + text.offsetX, rect.y + text.offsetY, text.color, text.fontSize);
            }
        });

        renderer->EndUI();
    }

} // namespace Game

