#include "Managers/WindowManager.h"

#include "Application.h"
#include "Utils/Logs.h"

static LRESULT WindowManagerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return WindowManager::HandleEvent(hwnd, msg, wParam, lParam);
}

WindowManager* WindowManager::sInstance = nullptr;

WindowManager::WindowManager(Application* app)
    : mApp(app)
{
    sInstance = this;
}

bool WindowManager::Initialize(HINSTANCE hInstance)
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowManagerProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc))
    {
        Logs::Error("RegisterClass failed.");
        return false;
    }

    RECT R = { 0, 0, 1280, 720 };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mWindowHandle = CreateWindow(L"MainWnd", L"ExploreDX12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);
    if (mWindowHandle == nullptr)
    {
        Logs::Error("CreateWindow failed.");
        return false;
    }

    ShowWindow(mWindowHandle, SW_SHOW);
    UpdateWindow(mWindowHandle);

    return true;
}

void WindowManager::PollEvents()
{
    MSG msg = { 0 };
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowManager::HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        sInstance->mIsRunning = false;
        return 0;
    case WM_SIZE:
        sInstance->mWidth = LOWORD(lParam);
        sInstance->mHeight = HIWORD(lParam);
        if (wParam == SIZE_MINIMIZED)
        {
            sInstance->mIsMinimzed = true;
            sInstance->mIsMaximized = false;
        }
        else if (wParam == SIZE_MAXIMIZED)
        {
            sInstance->mIsMinimzed = false;
            sInstance->mIsMaximized = true;
            sInstance->mApp->OnWindowResize();
        }
        else if (wParam == SIZE_RESTORED)
        {
            if (sInstance->mIsMinimzed)
            {
                sInstance->mIsMinimzed = false;
                sInstance->mApp->OnWindowResize();
            }
            else if (sInstance->mIsMaximized)
            {
                sInstance->mIsMaximized = false;
                sInstance->mApp->OnWindowResize();
            }
            else if (sInstance->mIsResizing)
            {
                // Rien à faire
            }
            else
            {
                sInstance->mApp->OnWindowResize();
            }
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        sInstance->mIsResizing = true;
        return 0;
    case WM_EXITSIZEMOVE:
        sInstance->mIsResizing = false;
        sInstance->mApp->OnWindowResize();
        return 0;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        sInstance->mApp->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        sInstance->mApp->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        sInstance->mApp->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYDOWN:
        sInstance->mApp->OnKeyDown(wParam);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND WindowManager::GetWindowHandle()
{
    return sInstance->mWindowHandle;
}

int WindowManager::GetWidth()
{
    return sInstance->mWidth;
}

int WindowManager::GetHeight()
{
    return sInstance->mHeight;
}

float WindowManager::AspectRatio()
{
    return static_cast<float>(sInstance->mWidth) / sInstance->mHeight;
}

void WindowManager::CaptureMouse()
{
    SetCapture(sInstance->mWindowHandle);
}

void WindowManager::ReleaseCaptureMouse()
{
    ReleaseCapture();
}

void WindowManager::SetWindowTitle(const std::wstring& title)
{
    SetWindowText(sInstance->mWindowHandle, title.c_str());
}
