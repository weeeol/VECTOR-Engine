#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Application.hpp"
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

    void MainMenuScene::ClearUI() {
        m_Registry.Clear();
    }

    void MainMenuScene::OnResize(int width, int height) {
        m_Width = width;
        m_Height = height;
        CreateUI();
    }

    void MainMenuScene::CreateUI() {
        ClearUI();
        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        if (m_State == MainMenuState::Main) {
            VECTOR::Entity playBtn = m_Registry.CreateEntity();
            m_Registry.AddComponent(playBtn, VECTOR::UIRectComponent(startX, startY, btnWidth, btnHeight, glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(playBtn, VECTOR::UITextComponent("Play", 24, glm::vec4(1.0f)));
            
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

            VECTOR::Entity settingsBtn = m_Registry.CreateEntity();
            m_Registry.AddComponent(settingsBtn, VECTOR::UIRectComponent(startX, startY + 60, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 100/255.0f, 1.0f)));
            m_Registry.AddComponent(settingsBtn, VECTOR::UITextComponent("Settings", 24, glm::vec4(1.0f)));
            auto& sText = m_Registry.GetComponent<VECTOR::UITextComponent>(settingsBtn);
            sText.offsetX = (btnWidth / 2) - (std::string("Settings").length() * 6);
            sText.offsetY = (btnHeight / 2) - 12;

            m_Registry.AddComponent(settingsBtn, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 100/255.0f, 1.0f),
                glm::vec4(100/255.0f, 100/255.0f, 200/255.0f, 1.0f),
                [this]() {
                    m_State = MainMenuState::Settings;
                    m_NeedsUIRefresh = true;
                }
            ));

            VECTOR::Entity exitBtn = m_Registry.CreateEntity();
            m_Registry.AddComponent(exitBtn, VECTOR::UIRectComponent(startX, startY + 120, btnWidth, btnHeight, glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(exitBtn, VECTOR::UITextComponent("Exit", 24, glm::vec4(1.0f)));
            auto& eText = m_Registry.GetComponent<VECTOR::UITextComponent>(exitBtn);
            eText.offsetX = (btnWidth / 2) - (std::string("Exit").length() * 6);
            eText.offsetY = (btnHeight / 2) - 12;

            m_Registry.AddComponent(exitBtn, VECTOR::UIButtonComponent(
                glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f),
                glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() {
                    VECTOR::Application::Get().Quit();
                }
            ));

            VECTOR::Entity volSlider = m_Registry.CreateEntity();
            m_Registry.AddComponent(volSlider, VECTOR::UIRectComponent(startX, startY + 200, btnWidth, 20, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f)));
            m_Registry.AddComponent(volSlider, VECTOR::UISliderComponent(VECTOR::AudioManager::Get().GetMusicVolume(), [](float val) {
                VECTOR::AudioManager::Get().SetMusicVolume(val);
            }));
        } else if (m_State == MainMenuState::Settings) {
            // Settings UI
            VECTOR::Entity res1 = m_Registry.CreateEntity();
            m_Registry.AddComponent(res1, VECTOR::UIRectComponent(startX, startY, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(res1, VECTOR::UITextComponent("1280x720", 24, glm::vec4(1.0f)));
            auto& r1Text = m_Registry.GetComponent<VECTOR::UITextComponent>(res1);
            r1Text.offsetX = (btnWidth / 2) - (std::string("1280x720").length() * 6);
            r1Text.offsetY = (btnHeight / 2) - 12;
            m_Registry.AddComponent(res1, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetResolution(1280, 720); }
            ));

            VECTOR::Entity res2 = m_Registry.CreateEntity();
            m_Registry.AddComponent(res2, VECTOR::UIRectComponent(startX, startY + 60, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(res2, VECTOR::UITextComponent("1920x1080", 24, glm::vec4(1.0f)));
            auto& r2Text = m_Registry.GetComponent<VECTOR::UITextComponent>(res2);
            r2Text.offsetX = (btnWidth / 2) - (std::string("1920x1080").length() * 6);
            r2Text.offsetY = (btnHeight / 2) - 12;
            m_Registry.AddComponent(res2, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetResolution(1920, 1080); }
            ));

            VECTOR::Entity fsWin = m_Registry.CreateEntity();
            m_Registry.AddComponent(fsWin, VECTOR::UIRectComponent(startX - 110, startY + 120, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(fsWin, VECTOR::UITextComponent("Windowed", 24, glm::vec4(1.0f)));
            auto& fswText = m_Registry.GetComponent<VECTOR::UITextComponent>(fsWin);
            fswText.offsetX = (btnWidth / 2) - (std::string("Windowed").length() * 6);
            fswText.offsetY = (btnHeight / 2) - 12;
            m_Registry.AddComponent(fsWin, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetFullscreen(false, false); }
            ));

            VECTOR::Entity fsB = m_Registry.CreateEntity();
            m_Registry.AddComponent(fsB, VECTOR::UIRectComponent(startX + 110, startY + 120, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(fsB, VECTOR::UITextComponent("Borderless", 24, glm::vec4(1.0f)));
            auto& fsbText = m_Registry.GetComponent<VECTOR::UITextComponent>(fsB);
            fsbText.offsetX = (btnWidth / 2) - (std::string("Borderless").length() * 6);
            fsbText.offsetY = (btnHeight / 2) - 12;
            m_Registry.AddComponent(fsB, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetFullscreen(true, true); }
            ));

            VECTOR::Entity backBtn = m_Registry.CreateEntity();
            m_Registry.AddComponent(backBtn, VECTOR::UIRectComponent(startX, startY + 180, btnWidth, btnHeight, glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_Registry.AddComponent(backBtn, VECTOR::UITextComponent("Back", 24, glm::vec4(1.0f)));
            auto& bText = m_Registry.GetComponent<VECTOR::UITextComponent>(backBtn);
            bText.offsetX = (btnWidth / 2) - (std::string("Back").length() * 6);
            bText.offsetY = (btnHeight / 2) - 12;
            m_Registry.AddComponent(backBtn, VECTOR::UIButtonComponent(
                glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                [this]() {
                    m_State = MainMenuState::Main;
                    m_NeedsUIRefresh = true;
                }
            ));
        }
    }

    void MainMenuScene::Update(float deltaTime) {
        m_UISystem->Update(m_Registry, deltaTime);
        if (m_NeedsUIRefresh) {
            CreateUI();
            m_NeedsUIRefresh = false;
        }
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->BeginUI();
        
        if (m_State == MainMenuState::Main) {
            renderer->DrawUIText("PONG", m_Width / 2 - 60, m_Height / 2 - 150, glm::vec4(1.0f), 48);
            renderer->DrawUIText("Volume", m_Width / 2 - 40, m_Height / 2 + 155, glm::vec4(200/255.0f, 200/255.0f, 200/255.0f, 1.0f), 18);
        } else {
            renderer->DrawUIText("Settings", m_Width / 2 - 80, m_Height / 2 - 150, glm::vec4(1.0f), 48);
        }

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
