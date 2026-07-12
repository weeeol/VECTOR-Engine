#pragma once

#include <string>
#include <memory>
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    /**
     * @class Application
     * @brief The core engine loop and initialization manager.
     * 
     * Handles initializing systems, running the main game loop with a fixed
     * timestep for physics updates and a variable timestep for rendering,
     * and graceful shutdown.
     */
    class Application {
    public:
        static Application& Get();

        Application(const std::string& title, int width, int height);
        virtual ~Application();

        // Run the main game loop
        void Run();

        // Gracefully shut down the application
        void Quit();

        float GetFPS() const { return m_CurrentFPS; }

    private:
        // Initialize subsystems (SDL, Renderer, Window, etc.)
        bool Initialize();
        // Cleanup resources
        void Shutdown();

        // Internal loop methods
        void ProcessInput();
    protected:
        virtual void OnInit() {}
        virtual void Update(float deltaTime);
        virtual void Render();

        std::string m_Title;
        int m_Width;
        int m_Height;
        bool m_IsRunning;
        float m_CurrentFPS;

        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<InputManager> m_InputManager;
    };

} // namespace VECTOR
