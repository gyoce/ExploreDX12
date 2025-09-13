#include "Application.h"

#include <string>

Application::Application(HINSTANCE hInstance)
    : mInstanceHandle(hInstance)
{
	mIsInit = mWindowManager.Initialize(hInstance) && DirectX12::Initialize(hInstance);
}

Application::~Application()
{
	DirectX12::Dispose();
}

int Application::Run()
{
    if (!mIsInit)
        return -1;

	OnWindowResize();
	WindowManager::FocusActiveWindow();

	while (mWindowManager.IsRunning())
	{
		mWindowManager.PollEvents();
		mTimeManager.Tick();
		Update();
		Draw();

		timer += TimeManager::GetDeltaTime();
		if (timer > 1.0f)
		{
			timer = 0.0f;
			WindowManager::SetWindowTitle(L"ExploreDX12 | fps : " + std::to_wstring(static_cast<int>(1.0f / TimeManager::GetDeltaTime())));
		}
	}

    return 0;
}

void Application::OnWindowResize()
{
	DirectX12::OnWindowResize();
}