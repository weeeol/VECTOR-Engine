#include "Game/Scenes/LoadingScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/ECS/UIComponents.hpp"
#include <thread>
#include <chrono>

namespace Game {

    LoadingScene::LoadingScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), m_Difficulty(aiDifficulty)
    {
        m_UIRegistry.RegisterComponent<VECTOR::UIRectComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UITextComponent>();
        
        m_UISystem = std::make_unique<VECTOR::UISystem>(m_InputManager);
    }

    LoadingScene::~LoadingScene() {
    }

    void LoadingScene::OnEnter() {
        CreateUI();
        m_LoadingStarted = false;
        m_LoadingProgress = 0.0f;
    }

    void LoadingScene::OnResize(int width, int height) {
        m_Width = width;
        m_Height = height;
        CreateUI();
    }

    void LoadingScene::CreateUI() {
        m_UIRegistry.Clear();

        // Background
        VECTOR::Entity bg = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(bg, VECTOR::UIRectComponent(0, 0, m_Width, m_Height, glm::vec4(0.05f, 0.05f, 0.05f, 1.0f)));

        // Loading Text
        VECTOR::Entity text = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(text, VECTOR::UITextComponent("LOADING...", 36, glm::vec4(1.0f)));
        auto& textC = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(text);
        textC.offsetX = (m_Width / 2) - 80;
        textC.offsetY = (m_Height / 2) - 60;

        // Progress Bar Background
        int barWidth = 400;
        int barHeight = 20;
        int startX = m_Width / 2 - barWidth / 2;
        int startY = m_Height / 2;

        VECTOR::Entity barBg = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(barBg, VECTOR::UIRectComponent(startX, startY, barWidth, barHeight, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)));

        // Progress Bar Fill
        m_ProgressBarEntity = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(m_ProgressBarEntity, VECTOR::UIRectComponent(startX, startY, 0, barHeight, glm::vec4(0.3f, 0.8f, 0.3f, 1.0f)));
    }

    void LoadingScene::Update(float deltaTime) {
        m_UISystem->Update(m_UIRegistry, deltaTime);

        if (!m_LoadingStarted) {
            m_LoadingStarted = true;
            
            // Dispatch the heavy loading tasks to the worker threads
            m_LoadingFuture = VECTOR::JobSystem::Get().Execute([this]() {
                auto playerModel = VECTOR::ResourceManager::Get().LoadModel("player", "assets/models/Walking.dae");
                m_LoadingProgress = 0.4f;
                
                VECTOR::ResourceManager::Get().LoadAnimation("walk", "assets/models/Walking.dae", playerModel);
                m_LoadingProgress = 0.7f;
                
                // Add tiny fake delays just to show the dynamic progress bar (for polish!)
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                
                m_LoadingProgress = 1.0f;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        } else {
            // Update UI progress bar width
            if (m_UIRegistry.HasComponent<VECTOR::UIRectComponent>(m_ProgressBarEntity)) {
                auto& rect = m_UIRegistry.GetComponent<VECTOR::UIRectComponent>(m_ProgressBarEntity);
                
                int maxBarWidth = 400;
                // Smooth interpolation towards target progress
                float currentWidth = static_cast<float>(rect.width);
                float targetWidth = m_LoadingProgress * maxBarWidth;
                
                currentWidth += (targetWidth - currentWidth) * 10.0f * deltaTime;
                rect.width = static_cast<int>(currentWidth);
            }

            // Check if future is ready
            if (m_LoadingFuture.valid() && m_LoadingFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                // Done loading! Transition to GameplayScene
                auto gameplayScene = std::make_unique<GameplayScene>(m_Width, m_Height, m_InputManager, m_Difficulty);
                VECTOR::SceneManager::Get().ChangeScene(std::move(gameplayScene));
            }
        }
    }

    void LoadingScene::Render(VECTOR::Renderer* renderer) {
        renderer->BeginUI();
        
        m_UIRegistry.View<VECTOR::UIRectComponent>([&](VECTOR::Entity entity) {
            auto& rect = m_UIRegistry.GetComponent<VECTOR::UIRectComponent>(entity);
            if (!rect.isVisible) return;
            
            renderer->DrawUIRect(rect.x, rect.y, rect.width, rect.height, rect.color);
            
            if (m_UIRegistry.HasComponent<VECTOR::UITextComponent>(entity)) {
                auto& text = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(entity);
                renderer->DrawUIText(text.text, rect.x + text.offsetX, rect.y + text.offsetY, text.color, text.fontSize);
            }
        });

        renderer->EndUI();
    }

} // namespace Game
