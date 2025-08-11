#include "Game.h"

#include "Utils/Logs.h"
#include "DirectX12.h"

Game::Game(HINSTANCE hInstance)
{
    Logs::AddConsole();

    mWindowManager = std::make_unique<WindowManager>();
    mEventManager = std::make_unique<EventManager>();

    mIsInit = mWindowManager->Initialize(hInstance) && DirectX12::Initialize();
}

int Game::Run()
{
    if (!mIsInit)
        return 1;

    while (!EventManager::CloseAppAsked())
    {
        mEventManager->PollEvents();
    }

    return 0;
}