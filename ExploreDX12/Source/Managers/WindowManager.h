#pragma once

#include <windows.h>

class Application;

class WindowManager
{
public:
    WindowManager(Application* app);

    bool Initialize(HINSTANCE hInstance);
    void PollEvents();

    bool IsRunning() const { return mIsRunning; }

    LRESULT CALLBACK HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HWND GetWindowHandle();
    static int GetWidth();
    static int GetHeight();
    static float AspectRatio();
    static void CaptureMouse();
    static void ReleaseCaptureMouse();
    static WindowManager* GetInstance();

private:
    Application* mApp = nullptr;
    bool mIsRunning = true;
    HWND mWindowHandle = nullptr;
    int mWidth = 1280;
    int mHeight = 720;

    static WindowManager* sInstance;
};

