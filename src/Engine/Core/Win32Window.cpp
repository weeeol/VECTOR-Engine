#include "Engine/Core/Win32Window.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace VECTOR {

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CLOSE:
                PostQuitMessage(0);
                return 0;
            case WM_DESTROY:
                return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    Win32Window::Win32Window(const WindowProps& props) {
        Init(props);
    }

    Win32Window::~Win32Window() {
        Shutdown();
    }

    void Win32Window::Init(const WindowProps& props) {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        VECTOR_LOG_INFO("Creating Win32 window " + props.Title + " (" + std::to_string(props.Width) + ", " + std::to_string(props.Height) + ")");

        m_Instance = GetModuleHandle(nullptr);

        const char* CLASS_NAME = "VECTOR_ENGINE_WINDOW";

        WNDCLASS wc = {};
        wc.lpfnWndProc   = WindowProc;
        wc.hInstance     = m_Instance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        m_Window = CreateWindowEx(
            0,
            CLASS_NAME,
            m_Data.Title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, m_Data.Width, m_Data.Height,
            nullptr,
            nullptr,
            m_Instance,
            nullptr
        );

        if (!m_Window) {
            VECTOR_LOG_ERROR("Failed to create Win32 window.");
            return;
        }

        ShowWindow(m_Window, SW_SHOW);

        if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12) {
            m_Context = std::make_unique<DirectX12Context>(m_Window, m_Data.Width, m_Data.Height);
            m_Context->Init();
        }

        SetVSync(true);
    }

    void Win32Window::Shutdown() {
        if (m_Window) {
            DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void Win32Window::OnUpdate() {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                // Handle application quit here
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (m_Context) {
            m_Context->SwapBuffers();
        }
    }

    void Win32Window::SetVSync(bool enabled) {
        m_Data.VSync = enabled;
        // DirectX Swapchain VSync is handled in SwapBuffers (Present), this sets state
    }

    bool Win32Window::IsVSync() const {
        return m_Data.VSync;
    }

} // namespace VECTOR
