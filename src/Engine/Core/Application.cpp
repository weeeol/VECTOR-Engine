#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/SceneManager.hpp"
#include <SDL3/SDL.h>

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

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == false) {
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

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForSDLRenderer(m_Renderer->GetSDLWindow(), m_Renderer->GetSDLRenderer());
        ImGui_ImplSDLRenderer3_Init(m_Renderer->GetSDLRenderer());

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

            // Start ImGui frame
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Fixed time-step update loop
            while (accumulator >= targetFrameTime) {
                Update(targetFrameTime / 1000.0f); // Convert back to seconds for logic
                if (m_InputManager) {
                    m_InputManager->ClearJustPressed(); // Consume single-frame inputs
                }
                accumulator -= targetFrameTime;
            }

            Render();

            // Render ImGui
            OnImGuiRender();
            SceneManager::Get().OnImGuiRender();
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer->GetSDLRenderer());

            // Present the frame
            if (m_Renderer) {
                m_Renderer->Present();
            }
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

    void Application::Shutdown() {
        // Shutdown ImGui
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        SceneManager::Get().Clear();
        
        if (m_Renderer) {
            m_Renderer->Shutdown();
        }
        
        AudioManager::Get().Shutdown();
        ResourceManager::Get().Shutdown();
        
        SDL_Quit();
    }

    void Application::ProcessInput() {
        if (m_InputManager) {
            m_InputManager->PrepareForEvents();
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                Quit();
            }
            // ImGui wants to capture mouse/keyboard?
            ImGuiIO& io = ImGui::GetIO();
            bool blockInput = false;
            // For now, let's just pass events. In a full engine you'd check io.WantCaptureMouse.
            if (m_InputManager) {
                m_InputManager->ProcessEvent(event);
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
