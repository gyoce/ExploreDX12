#include "WindowManager.h"

#include "Utils/Logs.h"
#include "Managers/EventManager.h"

WindowManager* WindowManager::sInstance = nullptr;

WindowManager::WindowManager()
{
    if (sInstance)
    {
        Logs::Error("Only one WindowManager can exist.");
        throw std::runtime_error("WindowManager already exists.");
    }
    sInstance = this;
}

HWND WindowManager::GetWindowHandle()
{
    return sInstance->mMainWindow;
}

bool WindowManager::Initialize(HINSTANCE hInstance)
{
    mhInstance = hInstance;

    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowManagerProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhInstance;
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

    mMainWindow = CreateWindow(L"MainWnd", L"ExploreDX12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhInstance, 0);
    if (mMainWindow == nullptr)
    {
        Logs::Error("CreateWindow failed.");
        return false;
    }

    ShowWindow(mMainWindow, SW_SHOW);
    UpdateWindow(mMainWindow);

    return true;
}

LRESULT WindowManager::WindowManagerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return EventManager::Instance()->HandleEvent(hwnd, msg, wParam, lParam);
}
