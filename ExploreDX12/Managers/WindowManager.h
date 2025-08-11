#pragma once

#include <windows.h>
#include <memory>

class WindowManager
{
public:
    WindowManager();

    static HWND GetWindowHandle();

    using UP = std::unique_ptr<WindowManager>;

private:
    bool Initialize(HINSTANCE hInstance);

    static LRESULT CALLBACK WindowManagerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HINSTANCE mhInstance = nullptr;
    HWND mMainWindow = nullptr;

    static WindowManager* sInstance;

    friend class Game;
};

