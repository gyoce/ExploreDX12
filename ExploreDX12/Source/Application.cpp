#include "Application.h"

#include "Graphics/DirectX12.h"
#include "Scenes/BoxAppScene.h"

Application::Application(HINSTANCE hInstance)
    : mInstanceHandle(hInstance)
{
	Logs::AddConsole();
	mScenes.push_back(std::make_unique<BoxAppScene>());
	mIsInit = mWindowManager.Initialize(hInstance) && DirectX12::Initialize(hInstance);

	for (size_t i = 0; i < mScenes.size() && mIsInit; i++)
		mIsInit &= mScenes[i]->Initialize();

	if (mIsInit)
	{
		mCurrentScene = mScenes[0].get();
		OnWindowResize();
	}
}

Application::~Application()
{
	mScenes.clear();
	DirectX12::Dispose();
}

int Application::Run()
{
    if (!mIsInit)
        return -1;

	while (mWindowManager.IsRunning())
	{
		mWindowManager.PollEvents();
		Update();
		Draw();
	}

    return 0;
}

void Application::OnWindowResize()
{
	DirectX12::OnWindowResize();
	if (mCurrentScene)
		mCurrentScene->OnWindowResize();
}

void Application::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (mCurrentScene)
		mCurrentScene->OnMouseDown(btnState, x, y);
}

void Application::OnMouseUp(WPARAM btnState, int x, int y)
{
	if (mCurrentScene)
		mCurrentScene->OnMouseUp(btnState, x, y);
}

void Application::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (mCurrentScene)
		mCurrentScene->OnMouseMove(btnState, x, y);
}

void Application::Update()
{
	if (mCurrentScene)
		mCurrentScene->Update();
}

void Application::Draw()
{
	if (mCurrentScene)
		mCurrentScene->Draw();
}