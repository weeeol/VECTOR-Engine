#pragma once

#include "Engine/Core/Window.hpp"
#include "Engine/Graphics/GraphicsContext.hpp"
#include <SDL.h>
#include <memory>

namespace VECTOR {

    class SDLWindow : public Window {
    public:
        SDLWindow(const WindowProps& props);
        virtual ~SDLWindow();

        void OnUpdate() override;

        inline uint32_t GetWidth() const override { return m_Data.Width; }
        inline uint32_t GetHeight() const override { return m_Data.Height; }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        inline void* GetNativeWindow() const override { return m_Window; }

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

    private:
        SDL_Window* m_Window;
        std::unique_ptr<GraphicsContext> m_Context;

        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
        };

        WindowData m_Data;
    };

} // namespace VECTOR
