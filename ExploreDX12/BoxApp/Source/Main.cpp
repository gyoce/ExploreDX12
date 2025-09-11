#include "BoxApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        BoxApp app{ hInstance };
        return app.Run();
    } 
    catch (const DxException& exception)
    {
        MessageBox(nullptr, exception.what().c_str(), L"HR Failed", MB_OK);
        return -1;
    }
}