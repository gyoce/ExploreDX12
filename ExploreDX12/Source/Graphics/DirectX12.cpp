#include "Graphics/DirectX12.h"

#include "Managers/WindowManager.h"

namespace DirectX12
{
    void CreateDevice();
    void GetDescriptorsSize();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvAndDsvDescriptorHeaps();

    bool mIsInit = false;
}

bool DirectX12::Initialize(HINSTANCE hInstance)
{
    CreateDevice();
    GetDescriptorsSize();
    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    mIsInit = true;

    return true;
}

void DirectX12::Dispose()
{
    if (D3DDevice != nullptr)
        FlushCommandQueue();

    for (int i = 0; i < SwapChainBufferCount; i++)
        SwapChainBuffer[i].Reset();
    DepthStencilBuffer.Reset();

    RtvHeap.Reset();
    DsvHeap.Reset();

    SwapChain.Reset();
    CommandList.Reset();
    CommandListAllocator.Reset();
    CommandQueue.Reset();

    Fence.Reset();
    D3DDevice.Reset();
    DxgiFactory.Reset();

#if defined(DEBUG) || defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
    }
#endif
}

void DirectX12::CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG) 
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DxgiFactory)));
#else
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory)));
#endif

    HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&D3DDevice));

    if (FAILED(hardwareResult))
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&D3DDevice)));
    }

    ThrowIfFailed(D3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
}

void DirectX12::GetDescriptorsSize()
{
    RtvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DsvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CbvSrvUavDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DirectX12::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(D3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));
    ThrowIfFailed(D3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandListAllocator.GetAddressOf())));
    ThrowIfFailed(D3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandListAllocator.Get(), nullptr, IID_PPV_ARGS(CommandList.GetAddressOf())));
    CommandList->Close();
}

void DirectX12::CreateSwapChain()
{
    SwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = WindowManager::GetWidth();
    sd.BufferDesc.Height = WindowManager::GetHeight();
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = BackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = WindowManager::GetInstance()->GetWindowHandle();
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(DxgiFactory->CreateSwapChain(CommandQueue.Get(), &sd, SwapChain.GetAddressOf()));
}

void DirectX12::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(RtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(DsvHeap.GetAddressOf())));
}

void DirectX12::OnWindowResize()
{
    if (!mIsInit)
        return;

    const int width = WindowManager::GetWidth();
    const int height = WindowManager::GetHeight();

    FlushCommandQueue();

    ThrowIfFailed(CommandList->Reset(CommandListAllocator.Get(), nullptr));

    for (int i = 0; i < SwapChainBufferCount; i++)
        SwapChainBuffer[i].Reset();
    DepthStencilBuffer.Reset();

    ThrowIfFailed(SwapChain->ResizeBuffers(SwapChainBufferCount, width, height, BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    CurrentBackBufferIndex = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i])));
        D3DDevice->CreateRenderTargetView(SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, RtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(D3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(DepthStencilBuffer.GetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    D3DDevice->CreateDepthStencilView(DepthStencilBuffer.Get(), &dsvDesc, DsvHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    CommandList->ResourceBarrier(1, &resourceBarrier);

    ThrowIfFailed(CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

    ScreenViewport.TopLeftX = 0;
    ScreenViewport.TopLeftY = 0;
    ScreenViewport.Width = static_cast<float>(width);
    ScreenViewport.Height = static_cast<float>(height);
    ScreenViewport.MinDepth = 0.0f;
    ScreenViewport.MaxDepth = 1.0f;

    ScissorRect = { 0, 0, width, height };
}

void DirectX12::FlushCommandQueue()
{
    CurrentFence++;

    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), CurrentFence));

    if (Fence->GetCompletedValue() < CurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(Fence->SetEventOnCompletion(CurrentFence, eventHandle));

        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

ID3D12Resource* DirectX12::CurrentBackBuffer()
{
    return SwapChainBuffer[CurrentBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12::CurrentBackBufferView()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentBackBufferIndex, RtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12::DepthStencilView()
{
    return DsvHeap->GetCPUDescriptorHandleForHeapStart();
}
