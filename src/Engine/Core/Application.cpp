#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
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

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
            VECTOR_LOG_ERROR(std::string("SDL could not initialize! SDL_Error: ") + SDL_GetError());
            return false;
        }

        // Initialize Window and Renderer
        m_Renderer = std::make_unique<Renderer>();
        if (!m_Renderer->Initialize(m_Title, m_Width, m_Height)) {
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

        Uint32 previousTime = SDL_GetTicks();
        float accumulator = 0.0f;
        Uint32 frameCount = 0;
        Uint32 fpsTimer = SDL_GetTicks();

        while (m_IsRunning) {
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = currentTime - previousTime;
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

            if (currentTime - fpsTimer >= 1000) {
                m_CurrentFPS = frameCount;
                frameCount = 0;
                fpsTimer = currentTime;
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
