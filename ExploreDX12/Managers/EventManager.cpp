#include "EventManager.h"

#include "Utils/Logs.h"

EventManager* EventManager::sInstance = nullptr;

EventManager::EventManager()
{
    if (sInstance)
    {
        Logs::Error("Only one EventManager can exist.");
        throw std::runtime_error("EventManager already exists.");
    }
    sInstance = this;
}

void EventManager::PollEvents()
{
    MSG msg{ 0 };

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT EventManager::HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        mCloseAppAsked = true;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool EventManager::CloseAppAsked()
{
    return sInstance->mCloseAppAsked;
}

EventManager* EventManager::Instance()
{
    return sInstance;
}
