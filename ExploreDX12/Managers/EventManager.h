#pragma once

#include <windows.h>
#include <memory>

class EventManager
{
public:
    EventManager();

    static bool CloseAppAsked();

    static EventManager* Instance();

    using UP = std::unique_ptr<EventManager>;

private:
    void PollEvents();
    LRESULT HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    bool mCloseAppAsked = false;

    static EventManager* sInstance;

    friend class Game;
    friend class WindowManager;
};
