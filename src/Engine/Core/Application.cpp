#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include <SDL.h>

namespace VECTOR {

    Application::Application(const std::string& title, int width, int height)
        : m_Title(title), m_Width(width), m_Height(height), m_IsRunning(false), m_CurrentFPS(0.0f)
    {
    }

    Application::~Application() {
        Shutdown();
    }

    bool Application::Initialize() {
        Logger::Init();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
            VECTOR_LOG_ERROR(std::string("SDL could not initialize! SDL_Error: ") + SDL_GetError());
            SDL_assert(false && "SDL failed to initialize");
            return false;
        }

        ResourceManager::Get().Initialize();
        
        if (!AudioManager::Get().Initialize()) {
            return false;
        }

        // Initialize Window and Renderer
        m_Renderer = std::make_unique<Renderer>();
        if (!m_Renderer->Initialize(m_Title, m_Width, m_Height)) {
            SDL_assert(false && "Renderer failed to initialize");
            return false;
        }

        // Initialize InputManager
        m_InputManager = std::make_unique<InputManager>();

        m_IsRunning = true;
        return true;
    }

    void Application::Run() {
        if (!Initialize()) {
            return;
        }

        // Setup fixed time step variables
        const float targetFPS = 60.0f;
        const float targetFrameTime = 1000.0f / targetFPS; // In milliseconds

        Uint64 previousTime = SDL_GetPerformanceCounter();
        float accumulator = 0.0f;
        Uint32 frameCount = 0;
        Uint32 fpsTimer = SDL_GetTicks();

        while (m_IsRunning) {
            Uint64 currentTime = SDL_GetPerformanceCounter();
            // Convert to milliseconds
            float deltaTime = (float)((currentTime - previousTime) * 1000.0 / SDL_GetPerformanceFrequency());
            previousTime = currentTime;

            // Prevent spiral of death if deltaTime is too large
            if (deltaTime > 250.0f) {
                deltaTime = 250.0f;
            }

            accumulator += deltaTime;

            ProcessInput();

            // Fixed time-step update loop
            while (accumulator >= targetFrameTime) {
                Update(targetFrameTime / 1000.0f); // Convert back to seconds for logic
                accumulator -= targetFrameTime;
            }

            Render();
            frameCount++;

            Uint32 currentTicks = SDL_GetTicks();
            if (currentTicks - fpsTimer >= 1000) {
                m_CurrentFPS = frameCount;
                frameCount = 0;
                fpsTimer = currentTicks;
            }
        }
    }

    void Application::Quit() {
        m_IsRunning = false;
    }

    void Application::Shutdown() {
        if (m_Renderer) {
            m_Renderer->Shutdown();
        }
        
        AudioManager::Get().Shutdown();
        ResourceManager::Get().Shutdown();
        
        SDL_Quit();
    }

    void Application::ProcessInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
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
