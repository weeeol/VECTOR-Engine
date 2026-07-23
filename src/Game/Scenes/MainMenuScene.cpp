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
        if (m_State == MainMenuState::Main) {
            CreateMainMenuUI();
        } else if (m_State == MainMenuState::Settings) {
            CreateSettingsMenuUI();
        } else if (m_State == MainMenuState::Loading) {
            CreateLoadingUI();
        }
    }

    VECTOR::Entity MainMenuScene::CreateButtonEntity(int x, int y, int width, int height, const std::string& text, 
                                      const glm::vec4& normalColor, const glm::vec4& hoverColor, 
                                      std::function<void()> onClick) {
        VECTOR::Entity btn = m_Registry.CreateEntity();
        m_Registry.AddComponent(btn, VECTOR::UIRectComponent(x, y, width, height, normalColor));
        m_Registry.AddComponent(btn, VECTOR::UITextComponent(text, 24, glm::vec4(1.0f)));
        auto& textC = m_Registry.GetComponent<VECTOR::UITextComponent>(btn);
        textC.offsetX = (width / 2) - (text.length() * 6);
        textC.offsetY = (height / 2) - 12;
        m_Registry.AddComponent(btn, VECTOR::UIButtonComponent(normalColor, hoverColor, onClick));
        return btn;
    }

    VECTOR::Entity MainMenuScene::CreateSliderEntity(int x, int y, int width, int height, float initialValue, std::function<void(float)> onChange) {
        VECTOR::Entity slider = m_Registry.CreateEntity();
        m_Registry.AddComponent(slider, VECTOR::UIRectComponent(x, y, width, height, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f)));
        m_Registry.AddComponent(slider, VECTOR::UISliderComponent(initialValue, onChange));
        return slider;
    }

    void MainMenuScene::CreateMainMenuUI() {
        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        CreateButtonEntity(startX, startY, btnWidth, btnHeight, "Play", 
            glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f), 
            glm::vec4(100/255.0f, 200/255.0f, 100/255.0f, 1.0f),
            [this]() {
                m_State = MainMenuState::Loading;
                m_NeedsUIRefresh = true;
                m_LoadingFrames = 2; // Wait two frames so the UI can draw "Loading..."
            }
        );

        CreateButtonEntity(startX, startY + 60, btnWidth, btnHeight, "Settings", 
            glm::vec4(50/255.0f, 50/255.0f, 100/255.0f, 1.0f), 
            glm::vec4(100/255.0f, 100/255.0f, 200/255.0f, 1.0f),
            [this]() {
                m_State = MainMenuState::Settings;
                m_NeedsUIRefresh = true;
            }
        );

        CreateButtonEntity(startX, startY + 120, btnWidth, btnHeight, "Exit", 
            glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f), 
            glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            []() {
                VECTOR::Application::Get().Quit();
            }
        );

        CreateSliderEntity(startX, startY + 200, btnWidth, 20, VECTOR::AudioManager::Get().GetMusicVolume(), [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        });
    }

    void MainMenuScene::CreateSettingsMenuUI() {
        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        CreateButtonEntity(startX, startY, btnWidth, btnHeight, "1280x720", 
            glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            []() { VECTOR::Application::Get().SetResolution(1280, 720); }
        );

        CreateButtonEntity(startX, startY + 60, btnWidth, btnHeight, "1920x1080", 
            glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            []() { VECTOR::Application::Get().SetResolution(1920, 1080); }
        );

        CreateButtonEntity(startX - 110, startY + 120, btnWidth, btnHeight, "Windowed", 
            glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            []() { VECTOR::Application::Get().SetFullscreen(false, false); }
        );

        CreateButtonEntity(startX + 110, startY + 120, btnWidth, btnHeight, "Borderless", 
            glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            []() { VECTOR::Application::Get().SetFullscreen(true, true); }
        );

        CreateButtonEntity(startX, startY + 180, btnWidth, btnHeight, "Back", 
            glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
            [this]() {
                m_State = MainMenuState::Main;
                m_NeedsUIRefresh = true;
            }
        );
    }

    void MainMenuScene::CreateLoadingUI() {
        // Nothing here, we just draw the text directly in Render!
    }

    void MainMenuScene::Update(float deltaTime) {
        if (m_State == MainMenuState::Loading) {
            if (m_LoadingFrames > 0) {
                m_LoadingFrames--;
            } else {
                // Actually start the game
                m_SelectedDifficulty = AIDifficulty::Medium;
                auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty);
                VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
                return;
            }
        }

        m_UISystem->Update(m_Registry, deltaTime);
        if (m_NeedsUIRefresh) {
            CreateUI();
            m_NeedsUIRefresh = false;
        }
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        renderer->Clear(0, 0, 0, 255);
        renderer->BeginUI();
        
        if (m_State == MainMenuState::Main) {
            renderer->DrawUIText("PONG", m_Width / 2 - 60, m_Height / 2 - 150, glm::vec4(1.0f), 48);
            renderer->DrawUIText("Volume", m_Width / 2 - 40, m_Height / 2 + 155, glm::vec4(200/255.0f, 200/255.0f, 200/255.0f, 1.0f), 18);
        } else if (m_State == MainMenuState::Settings) {
            renderer->DrawUIText("Settings", m_Width / 2 - 80, m_Height / 2 - 150, glm::vec4(1.0f), 48);
        } else if (m_State == MainMenuState::Loading) {
            renderer->DrawUIText("Loading...", m_Width / 2 - 80, m_Height / 2 - 20, glm::vec4(1.0f), 48);
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
