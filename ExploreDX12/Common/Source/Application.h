#pragma once

#include "Graphics/D3DUtils.h"
#include "Graphics/DirectX12.h"
#include "Managers/WindowManager.h"
#include "Managers/TimeManager.h"

class Application
{
public:
    Application(HINSTANCE hInstance);
    virtual ~Application();

    int Run();

    virtual void OnWindowResize();
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
    virtual void Update() {}
    virtual void Draw() {}

    bool mIsInit = false;
    HINSTANCE mInstanceHandle = nullptr;
    WindowManager mWindowManager { this };
    TimeManager mTimeManager{};
    float timer = 0.0f;
};