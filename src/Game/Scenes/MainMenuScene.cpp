#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/UI/UIButton.hpp"
#include "Engine/UI/UISlider.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Game/Core/SaveManager.hpp"
#include <cmath>

namespace Game {

    // Resolution presets
    struct ResolutionPreset {
        int width;
        int height;
        const char* label;
    };

    static const ResolutionPreset s_Resolutions[] = {
        { 1280, 720,  "1280 x 720" },
        { 1366, 768,  "1366 x 768" },
        { 1600, 900,  "1600 x 900" },
        { 1920, 1080, "1920 x 1080" }
    };
    static const int s_ResolutionCount = sizeof(s_Resolutions) / sizeof(s_Resolutions[0]);

    MainMenuScene::MainMenuScene(int width, int height, VECTOR::InputManager* inputManager)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), 
          m_ParticleEmitter(300),
          m_State(MenuState::SelectingMode),
          m_SelectedMode(GameMode::FivePoint),
          m_SelectedDifficulty(AIDifficulty::Medium)
    {
        // Load persisted settings
        float volume = 0.5f;
        bool borderless = false;
        int resW = 1280, resH = 720;
        SaveManager::LoadSettings(volume, borderless, resW, resH);

        m_IsBorderless = borderless;

        // Find matching resolution index
        m_ResolutionIndex = 0;
        for (int i = 0; i < s_ResolutionCount; ++i) {
            if (s_Resolutions[i].width == resW && s_Resolutions[i].height == resH) {
                m_ResolutionIndex = i;
                break;
            }
        }

        // Apply saved settings on construction
        m_Width = resW;
        m_Height = resH;
        VECTOR::AudioManager::Get().SetMusicVolume(volume);
        VECTOR::Application::Get().SetDimensions(resW, resH);
        VECTOR::Application::Get().GetRenderer()->SetResolution(resW, resH);
        if (borderless) {
            VECTOR::Application::Get().GetRenderer()->SetBorderless(true);
        }
    }

    void MainMenuScene::OnEnter() {
        VECTOR_LOG_INFO("MainMenuScene::OnEnter() started");
        VECTOR::AudioManager::Get().PlayMusic("assets/bgm.wav");
        CreateModeUI();
        VECTOR_LOG_INFO("MainMenuScene::OnEnter() finished");
    }

    void MainMenuScene::CreateModeUI() {
        m_UIManager.Clear();
        m_State = MenuState::SelectingMode;

        int btnWidth = 300;
        int btnHeight = 50;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 + 10;

        auto fivePointBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "5 Point Mode", [this]() {
            m_SelectedMode = GameMode::FivePoint;
            CreateDifficultyUI();
        });
        fivePointBtn->SetColors({20, 100, 160, 255}, {30, 160, 220, 255}, {200, 240, 255, 255});

        auto noLimitBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 70, btnWidth, btnHeight, "No Limit Mode", [this]() {
            m_SelectedMode = GameMode::Endless;
            CreateDifficultyUI();
        });
        noLimitBtn->SetColors({120, 30, 140, 255}, {180, 60, 200, 255}, {240, 200, 255, 255});

        auto settingsBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 140, btnWidth, btnHeight, "Settings", [this]() {
            CreateSettingsUI();
        });
        settingsBtn->SetColors({50, 70, 90, 255}, {80, 110, 140, 255}, {220, 230, 240, 255});

        auto exitBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 210, btnWidth, btnHeight, "Exit", []() {
            VECTOR::Application::Get().Quit();
        });
        exitBtn->SetColors({80, 30, 30, 255}, {140, 40, 40, 255}, {255, 200, 200, 255});

        m_UIManager.AddElement(fivePointBtn);
        m_UIManager.AddElement(noLimitBtn);
        m_UIManager.AddElement(settingsBtn);
        m_UIManager.AddElement(exitBtn);
    }

    void MainMenuScene::CreateDifficultyUI() {
        m_UIManager.Clear();
        m_State = MenuState::SelectingDifficulty;

        int btnWidth = 300;
        int btnHeight = 50;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 10;

        auto easyBtn = std::make_shared<VECTOR::UIButton>(startX, startY, btnWidth, btnHeight, "Easy AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Easy;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        easyBtn->SetColors({20, 120, 50, 255}, {40, 190, 80, 255}, {200, 255, 210, 255});

        auto medBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 70, btnWidth, btnHeight, "Medium AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Medium;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        medBtn->SetColors({140, 120, 20, 255}, {210, 180, 40, 255}, {255, 245, 200, 255});

        auto hardBtn = std::make_shared<VECTOR::UIButton>(startX, startY + 140, btnWidth, btnHeight, "Hard AI", [this]() {
            m_SelectedDifficulty = AIDifficulty::Hard;
            auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_SelectedDifficulty, m_SelectedMode);
            VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
        });
        hardBtn->SetColors({140, 30, 30, 255}, {210, 50, 50, 255}, {255, 210, 200, 255});

        m_UIManager.AddElement(easyBtn);
        m_UIManager.AddElement(medBtn);
        m_UIManager.AddElement(hardBtn);
    }

    void MainMenuScene::CreateSettingsUI() {
        m_UIManager.Clear();
        m_State = MenuState::Settings;

        int btnWidth = 300;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int baseY = m_Height / 2 - 80;

        // --- Volume slider ---
        float currentVolume = VECTOR::AudioManager::Get().GetMusicVolume();
        auto volumeSlider = std::make_shared<VECTOR::UISlider>(startX, baseY, btnWidth, 20, currentVolume, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        });
        m_UIManager.AddElement(volumeSlider);

        // --- Window Mode toggle ---
        std::string modeLabel = m_IsBorderless ? "Mode: Borderless" : "Mode: Windowed";
        auto windowModeBtn = std::make_shared<VECTOR::UIButton>(startX, baseY + 50, btnWidth, btnHeight, modeLabel, [this]() {
            m_IsBorderless = !m_IsBorderless;
            VECTOR::Application::Get().GetRenderer()->SetBorderless(m_IsBorderless);
            // Save and refresh UI
            float vol = VECTOR::AudioManager::Get().GetMusicVolume();
            SaveManager::SaveSettings(vol, m_IsBorderless, s_Resolutions[m_ResolutionIndex].width, s_Resolutions[m_ResolutionIndex].height);
            CreateSettingsUI(); // Refresh to update button label
        });
        if (m_IsBorderless) {
            windowModeBtn->SetColors({30, 100, 120, 255}, {50, 160, 180, 255}, {200, 240, 255, 255});
        } else {
            windowModeBtn->SetColors({50, 70, 90, 255}, {80, 110, 140, 255}, {220, 230, 240, 255});
        }
        m_UIManager.AddElement(windowModeBtn);

        // --- Resolution buttons ---
        for (int i = 0; i < s_ResolutionCount; ++i) {
            int resW = s_Resolutions[i].width;
            int resH = s_Resolutions[i].height;
            std::string label = s_Resolutions[i].label;
            if (i == m_ResolutionIndex) {
                label = "> " + label + " <";
            }

            auto resBtn = std::make_shared<VECTOR::UIButton>(startX, baseY + 110 + i * 50, btnWidth, btnHeight, label, [this, i, resW, resH]() {
                m_ResolutionIndex = i;
                // Switch to windowed mode if borderless, so resolution is visible
                if (m_IsBorderless) {
                    m_IsBorderless = false;
                    VECTOR::Application::Get().GetRenderer()->SetBorderless(false);
                }
                // Update internal game dimensions
                m_Width = resW;
                m_Height = resH;
                VECTOR::Application::Get().SetDimensions(resW, resH);
                VECTOR::Application::Get().GetRenderer()->SetResolution(resW, resH);
                float vol = VECTOR::AudioManager::Get().GetMusicVolume();
                SaveManager::SaveSettings(vol, m_IsBorderless, resW, resH);
                CreateSettingsUI(); // Refresh to update selection markers
            });

            if (i == m_ResolutionIndex) {
                resBtn->SetColors({20, 100, 160, 255}, {30, 160, 220, 255}, {200, 240, 255, 255});
            } else {
                resBtn->SetColors({40, 40, 55, 255}, {60, 60, 80, 255}, {180, 180, 200, 255});
            }
            m_UIManager.AddElement(resBtn);
        }

        // --- Back button ---
        auto backBtn = std::make_shared<VECTOR::UIButton>(startX, baseY + 110 + s_ResolutionCount * 50 + 20, btnWidth, btnHeight, "Back", [this]() {
            // Save settings before leaving
            float vol = VECTOR::AudioManager::Get().GetMusicVolume();
            SaveManager::SaveSettings(vol, m_IsBorderless, s_Resolutions[m_ResolutionIndex].width, s_Resolutions[m_ResolutionIndex].height);
            CreateModeUI();
        });
        backBtn->SetColors({60, 40, 40, 255}, {120, 60, 60, 255}, {255, 220, 220, 255});
        m_UIManager.AddElement(backBtn);
    }

    void MainMenuScene::Update(float deltaTime) {
        m_TitleTimer += deltaTime;
        m_GridOffset += 30.0f * deltaTime;
        if (m_GridOffset > 60.0f) m_GridOffset -= 60.0f;

        // Edge particles — emit along the borders for a glow effect
        if (rand() % 100 < 40) {
            int side = rand() % 4;
            float px = 0.0f, py = 0.0f;
            if (side == 0)      { px = (float)(rand() % m_Width); py = 5.0f; }      // Top
            else if (side == 1) { px = (float)(rand() % m_Width); py = (float)(m_Height - 5); } // Bottom
            else if (side == 2) { px = 5.0f; py = (float)(rand() % m_Height); }      // Left
            else                { px = (float)(m_Width - 5); py = (float)(rand() % m_Height); } // Right
            
            int colorPick = rand() % 3;
            Uint8 r = 80, g = 120, b = 255;
            if (colorPick == 1) { r = 180; g = 60; b = 220; }
            else if (colorPick == 2) { r = 60; g = 200; b = 200; }
            m_ParticleEmitter.Emit(px, py, 1, r, g, b, 25.0f, 2.5f);
        }

        // Ambient floating particles near center
        if (rand() % 100 < 15) {
            float px = (float)(m_Width / 4 + rand() % (m_Width / 2));
            float py = (float)(m_Height / 4 + rand() % (m_Height / 2));
            m_ParticleEmitter.Emit(px, py, 1, 100, 100, 180, 15.0f, 3.0f);
        }

        m_ParticleEmitter.Update(deltaTime);
        m_UIManager.Update(m_InputManager, deltaTime);

        // ESC to go back (with debounce)
        bool isEscPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        if (isEscPressed && !m_WasEscPressed) {
            if (m_State == MenuState::SelectingDifficulty || m_State == MenuState::Settings) {
                if (m_State == MenuState::Settings) {
                    // Save settings when leaving
                    float vol = VECTOR::AudioManager::Get().GetMusicVolume();
                    SaveManager::SaveSettings(vol, m_IsBorderless, s_Resolutions[m_ResolutionIndex].width, s_Resolutions[m_ResolutionIndex].height);
                }
                CreateModeUI();
            }
        }
        m_WasEscPressed = isEscPressed;
    }

    void MainMenuScene::Render(VECTOR::Renderer* renderer) {
        // --- Scrolling grid background ---
        for (int x = (int)m_GridOffset; x < m_Width; x += 60) {
            renderer->DrawRect(x, 0, 1, m_Height, 15, 15, 40, 60);
        }
        for (int y = (int)m_GridOffset; y < m_Height; y += 60) {
            renderer->DrawRect(0, y, m_Width, 1, 15, 15, 40, 60);
        }

        // --- Particles behind UI ---
        m_ParticleEmitter.Render(renderer);

        // --- Title with glow and bob ---
        float titleBob = std::sin(m_TitleTimer * 1.5f) * 8.0f;
        int titleX = m_Width / 2 - 80;
        int titleY = (int)(m_Height / 2 - 170 + titleBob);

        // Outer glow layers (drawn wider/taller, low alpha, cyan-tinted)
        renderer->DrawText("PONG", titleX - 4, titleY - 4, 0, 180, 255, 48);
        renderer->DrawText("PONG", titleX + 4, titleY + 4, 0, 180, 255, 48);
        renderer->DrawText("PONG", titleX - 3, titleY + 3, 0, 180, 255, 48);
        renderer->DrawText("PONG", titleX + 3, titleY - 3, 0, 180, 255, 48);
        // Main title
        renderer->DrawText("PONG", titleX, titleY, 255, 255, 255, 48);

        // --- Decorative line under title ---
        int lineY = titleY + 55;
        renderer->DrawRect(m_Width / 2 - 100, lineY, 200, 2, 0, 180, 255, 120);
        renderer->DrawRect(m_Width / 2 - 60, lineY + 4, 120, 1, 0, 180, 255, 60);

        // --- Sub-header ---
        if (m_State == MenuState::SelectingMode) {
            renderer->DrawText("Select Game Mode", m_Width / 2 - 110, m_Height / 2 - 40, 220, 220, 240, 24);
        } else if (m_State == MenuState::SelectingDifficulty) {
            renderer->DrawText("Select Difficulty", m_Width / 2 - 105, m_Height / 2 - 60, 220, 220, 240, 24);
        } else if (m_State == MenuState::Settings) {
            renderer->DrawText("Settings", m_Width / 2 - 55, m_Height / 2 - 140, 220, 220, 240, 24);
            // Volume label above slider
            renderer->DrawText("Volume", m_Width / 2 - 40, m_Height / 2 - 105, 180, 180, 200, 20);
        }

        // --- Accent rails alongside buttons ---
        int railX1 = m_Width / 2 - 170;
        int railX2 = m_Width / 2 + 168;
        int railTop, railHeight;
        if (m_State == MenuState::SelectingMode) {
            railTop = m_Height / 2 + 5;
            railHeight = 270;
        } else if (m_State == MenuState::SelectingDifficulty) {
            railTop = m_Height / 2 - 15;
            railHeight = 200;
        } else {
            // Settings: from volume slider to back button
            railTop = m_Height / 2 - 85;
            railHeight = 110 + s_ResolutionCount * 50 + 65;
        }
        renderer->DrawRect(railX1, railTop, 2, railHeight, 0, 180, 255, 80);
        renderer->DrawRect(railX2, railTop, 2, railHeight, 0, 180, 255, 80);
        // Rail caps
        renderer->DrawRect(railX1 - 4, railTop, 10, 2, 0, 180, 255, 100);
        renderer->DrawRect(railX1 - 4, railTop + railHeight - 2, 10, 2, 0, 180, 255, 100);
        renderer->DrawRect(railX2 - 4, railTop, 10, 2, 0, 180, 255, 100);
        renderer->DrawRect(railX2 - 4, railTop + railHeight - 2, 10, 2, 0, 180, 255, 100);

        // --- Buttons ---
        m_UIManager.Render(renderer);

        // --- Footer hint ---
        if (m_State == MenuState::SelectingDifficulty || m_State == MenuState::Settings) {
            renderer->DrawText("Press ESC to go back", m_Width / 2 - 120, m_Height - 50, 120, 120, 140, 20);
        }

        // --- Bottom credit/version ---
        renderer->DrawText("VECTOR Engine", m_Width / 2 - 75, m_Height - 25, 50, 50, 70, 16);
    }

} // namespace Game
