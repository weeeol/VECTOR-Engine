#include "Engine/Core/Win32Window.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Input/InputManager.hpp"
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

    static KeyCode MapWin32Key(WPARAM wParam) {
        switch (wParam) {
            case VK_SPACE: return KeyCode::Space;
            case VK_ESCAPE: return KeyCode::Escape;
            case VK_RETURN: return KeyCode::Enter;
            case VK_TAB: return KeyCode::Tab;
            case VK_RIGHT: return KeyCode::Right;
            case VK_LEFT: return KeyCode::Left;
            case VK_DOWN: return KeyCode::Down;
            case VK_UP: return KeyCode::Up;
            case VK_F1: return KeyCode::F1;
            case VK_F2: return KeyCode::F2;
            case VK_F3: return KeyCode::F3;
            default:
                if (wParam >= 'A' && wParam <= 'Z') return (KeyCode)wParam;
                if (wParam >= '0' && wParam <= '9') return (KeyCode)wParam;
                return (KeyCode)0;
        }
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
        if (m_Context) {
            m_Context->SwapBuffers();
        }
    }

    void Win32Window::ProcessEvents(class InputManager* inputManager) {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_ShouldClose = true;
            } else if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
                if (inputManager) {
                    bool isPressed = (msg.message == WM_KEYDOWN);
                    KeyCode key = MapWin32Key(msg.wParam);
                    if ((int)key != 0) {
                        inputManager->SetKeyState(key, isPressed);
                    }
                }
            } else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP) {
                if (inputManager) inputManager->SetMouseButtonState(MouseButton::Left, msg.message == WM_LBUTTONDOWN);
            } else if (msg.message == WM_RBUTTONDOWN || msg.message == WM_RBUTTONUP) {
                if (inputManager) inputManager->SetMouseButtonState(MouseButton::Right, msg.message == WM_RBUTTONDOWN);
            } else if (msg.message == WM_MBUTTONDOWN || msg.message == WM_MBUTTONUP) {
                if (inputManager) inputManager->SetMouseButtonState(MouseButton::Middle, msg.message == WM_MBUTTONDOWN);
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (inputManager && GetActiveWindow() == m_Window) {
            POINT p;
            GetCursorPos(&p);
            
            if (inputManager->IsRelativeMouseMode()) {
                RECT rect;
                GetWindowRect(m_Window, &rect);
                int centerX = rect.left + (rect.right - rect.left) / 2;
                int centerY = rect.top + (rect.bottom - rect.top) / 2;
                
                inputManager->SetMouseDelta(p.x - centerX, p.y - centerY);
                SetCursorPos(centerX, centerY);
            } else {
                ScreenToClient(m_Window, &p);
                static int lastX = p.x;
                static int lastY = p.y;
                
                inputManager->SetMouseDelta(p.x - lastX, p.y - lastY);
                inputManager->SetMousePosition(p.x, p.y);
                
                lastX = p.x;
                lastY = p.y;
            }
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
