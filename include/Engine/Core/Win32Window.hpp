#pragma once

#include "Engine/Core/Window.hpp"
#include "Engine/Graphics/GraphicsContext.hpp"
#include <memory>
#include <string>

// Forward declare Windows HWND to avoid including windows.h in header
struct HWND__;
typedef HWND__* HWND;
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;

namespace VECTOR {

    class Win32Window : public Window {
    public:
        Win32Window(const WindowProps& props);
        virtual ~Win32Window();

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
        HWND m_Window;
        HINSTANCE m_Instance;
        std::unique_ptr<GraphicsContext> m_Context;

        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
        };

        WindowData m_Data;
    };

} // namespace VECTOR
