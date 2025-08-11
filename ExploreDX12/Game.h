#pragma once

#include "Managers/WindowManager.h"
#include "Managers/EventManager.h"

class Game
{
public:
    Game(HINSTANCE hInstance);

    int Run();

private:
    bool mIsInit = false;
    WindowManager::UP mWindowManager = nullptr;
    EventManager::UP mEventManager = nullptr;
};
