#pragma once

#include "Graphics/D3DUtils.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

namespace DirectX12
{
    inline Microsoft::WRL::ComPtr<IDXGIFactory4> DxgiFactory;
    inline Microsoft::WRL::ComPtr<ID3D12Device> D3DDevice;
    inline Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    inline UINT64 CurrentFence = 0;

    inline UINT RtvDescriptorSize = 0;
    inline UINT DsvDescriptorSize = 0;
    inline UINT CbvSrvUavDescriptorSize = 0;

    inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
    inline Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandListAllocator;
    inline Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
    
    inline Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
    inline static const int SwapChainBufferCount = 2;
    inline DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    inline int CurrentBackBufferIndex = 0;
    
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RtvHeap;
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DsvHeap;
    
    inline Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainBuffer[SwapChainBufferCount];
    inline Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencilBuffer;
    inline DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    
    inline D3D12_VIEWPORT ScreenViewport;
    inline D3D12_RECT ScissorRect;

    bool Initialize(HINSTANCE hInstance);
    void Dispose();
    void OnWindowResize();
    void FlushCommandQueue();

    ID3D12Resource* CurrentBackBuffer();
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView();

} // namespace DirectX12