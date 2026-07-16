#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/SceneManager.hpp"
#include <chrono>

namespace VECTOR {

    static Application* s_Instance = nullptr;

    Application& Application::Get() {
        return *s_Instance;
    }

    Application::Application(const std::string& title, int width, int height)
        : m_Title(title), m_Width(width), m_Height(height), m_IsRunning(false), m_CurrentFPS(0.0f)
    {
        s_Instance = this;
    }

    Application::~Application() {
        Shutdown();
    }

    bool Application::Initialize() {
        Logger::Init();

        ResourceManager::Get().Initialize();
        
        if (!AudioManager::Get().Initialize()) {
            return false;
        }

        // Initialize Window and Renderer
        m_Renderer = std::make_unique<Renderer>();
        if (!m_Renderer->Initialize(m_Title, m_Width, m_Height)) {
            std::string error = "Renderer failed to initialize";
            VECTOR_LOG_ERROR(error);
            return false;
        }

        // Initialize InputManager
        m_InputManager = std::make_unique<InputManager>();

        m_IsRunning = true;
        
        OnInit();
        
        return true;
    }

    void Application::Run() {
        if (!Initialize()) {
            return;
        }

        auto previousTime = std::chrono::high_resolution_clock::now();
        float accumulator = 0.0f;
        uint32_t frameCount = 0;
        
        auto fpsTimer = std::chrono::high_resolution_clock::now();

        while (m_IsRunning) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::milli>(currentTime - previousTime).count();
            previousTime = currentTime;

            // Prevent spiral of death if deltaTime is too large
            if (deltaTime > 250.0f) {
                deltaTime = 250.0f;
            }

            accumulator += deltaTime;

            ProcessInput();

            // Allow Bullet and logic to use actual frame time.
            Update(deltaTime / 1000.0f);

            Render();
            frameCount++;

            auto currentFpsTime = std::chrono::high_resolution_clock::now();
            float elapsedSinceLastFPS = std::chrono::duration<float, std::milli>(currentFpsTime - fpsTimer).count();
            if (elapsedSinceLastFPS >= 1000.0f) {
                m_CurrentFPS = frameCount;
                frameCount = 0;
                fpsTimer = currentFpsTime;
            }
        }
    }

    void Application::Quit() {
        m_IsRunning = false;
    }

    void Application::Shutdown() {
        SceneManager::Get().Clear();
        
        if (m_Renderer) {
            m_Renderer->Shutdown();
        }
        
        AudioManager::Get().Shutdown();
        ResourceManager::Get().Shutdown();
    }

    void Application::ProcessInput() {
        if (m_Renderer && m_Renderer->GetWindow()) {
            m_Renderer->GetWindow()->ProcessEvents(m_InputManager.get());
            if (m_Renderer->GetWindow()->ShouldClose()) {
                Quit();
            }
        }
        
        if (m_InputManager) {
            m_InputManager->Update();
        }
    }

    void Application::Update(float deltaTime) {
        // Game Logic update goes here
    }

    void Application::Render() {
        // Rendering goes here
    }

} // namespace VECTOR
