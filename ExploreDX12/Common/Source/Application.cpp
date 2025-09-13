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
    WindowManager::FocusWindow();

	while (mWindowManager.IsRunning())
	{
		mWindowManager.PollEvents();
		mTimeManager.Tick();

		if (!mPause)
		{
            Update();
            Draw();
			ComputeFrameStats();
		}
		else
		{
			Sleep(100);
		}
	}

    return 0;
}

void Application::OnWindowResize()
{
    DirectX12::OnWindowResize();
}

void Application::OnApplicationPause()
{
	mPause = true;
	mTimeManager.Pause();
}

void Application::OnApplicationResume()
{
	mPause = false;
	mTimeManager.Resume();
}

void Application::ComputeFrameStats() 
{
	static int frameCount = 0;
	static float timeElapsed = 0.0f;

	frameCount++;

    if ((TimeManager::GetTotalTime() - timeElapsed) >= 1.0f)
    {
		float fps = static_cast<float>(frameCount);
		float mspf = 1000.0f / fps;
        WindowManager::SetWindowTitle(L"ExploreDX12 | fps : " + std::to_wstring(static_cast<int>(fps)) + L" | mspf : " + std::to_wstring(mspf));
		frameCount = 0;
		timeElapsed += 1.0f;
    }
}
