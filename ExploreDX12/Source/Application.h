#pragma once

#include "Graphics/D3DUtils.h"
#include "Managers/WindowManager.h"
#include "Managers/TimeManager.h"
#include "Scenes/Scene.h"

class Application
{
public:
    Application(HINSTANCE hInstance);
    ~Application();

    int Run();

    void OnWindowResize();
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);

private:
    void Update();
    void Draw();

    bool mIsInit = false;
    HINSTANCE mInstanceHandle = nullptr;
    WindowManager mWindowManager { this };
    TimeManager mTimeManager{};
    std::vector<Scene::UP> mScenes{};
    Scene* mCurrentScene = nullptr;
    float timer = 0.0f;
};