#include "Game.h"

#include "Utils/d3dUtil.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int)
{
    try
    {
        Game game{ hInstance };
        return game.Run();
    } 
    catch (const DxException& exception)
    {
        MessageBox(nullptr, exception.what().c_str(), L"HR Failed", MB_OK);
        return -1;
    }
}