#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/SceneManager.hpp"
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>

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

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            std::string error = std::string("SDL could not initialize! SDL_Error: ") + SDL_GetError();
            VECTOR_LOG_ERROR(error);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization Error", error.c_str(), nullptr);
            return false;
        }

        ResourceManager::Get().Initialize();
        
        if (!AudioManager::Get().Initialize()) {
            return false;
        }

        // Initialize Window and Renderer
        m_Renderer = Renderer::Create();
        if (!m_Renderer->Initialize(m_Title, m_Width, m_Height)) {
            std::string error = "Renderer failed to initialize";
            VECTOR_LOG_ERROR(error);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization Error", error.c_str(), nullptr);
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

        // Setup fixed time step variables
        const float targetFPS = 60.0f;
        const float targetFrameTime = 1000.0f / targetFPS; // In milliseconds

        Uint64 previousTime = SDL_GetPerformanceCounter();
        float accumulator = 0.0f;
        Uint32 frameCount = 0;
        Uint64 fpsTimer = SDL_GetTicks();

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

            // Allow Bullet and logic to use actual frame time.
            // Bullet handles fixed time steps internally and provides interpolated transforms.
            Update(deltaTime / 1000.0f);

            Render();
            frameCount++;

            Uint64 currentTicks = SDL_GetTicks();
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

    void Application::SetResolution(int width, int height) {
        m_Width = width;
        m_Height = height;
        if (m_Renderer) {
            m_Renderer->SetResolution(width, height);
        }
        SceneManager::Get().OnResize(width, height);
    }

    void Application::SetFullscreen(bool fullscreen, bool borderless) {
        if (m_Renderer) {
            m_Renderer->SetFullscreen(fullscreen, borderless);
        }
    }

    void Application::Shutdown() {
        if (m_Renderer) {
            m_Renderer->WaitIdle();
        }

        SceneManager::Get().Clear();
        
        ResourceManager::Get().Shutdown();
        AudioManager::Get().Shutdown();

        if (m_Renderer) {
            m_Renderer->Shutdown();
        }
        
        SDL_Quit();
    }

    void Application::ProcessInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                Quit();
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                SetResolution(event.window.data1, event.window.data2);
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
