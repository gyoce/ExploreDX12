#pragma once

#include <windows.h>
#include <string>

class Application;

class WindowManager
{
public:
    bool Initialize(HINSTANCE hInstance);
    void PollEvents();

    bool IsRunning() const { return mIsRunning; }

    static LRESULT CALLBACK HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HWND GetWindowHandle();
    static int GetWidth();
    static int GetHeight();
    static float AspectRatio();
    static void CaptureMouse();
    static void ReleaseCaptureMouse();
    static void SetWindowTitle(const std::wstring& title);
    static void FocusWindow();

private:
    WindowManager(Application* app);

    Application* mApp = nullptr;
    bool mIsRunning = true;
    bool mIsMaximized = false;
    bool mIsMinimized = false;
    bool mIsResizing = false;
    HWND mWindowHandle = nullptr;
    int mWidth = 1280;
    int mHeight = 720;

    static WindowManager* sInstance;

    friend class Application;
};

