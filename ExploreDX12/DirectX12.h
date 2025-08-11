#pragma once

#include "Utils/d3dUtil.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

namespace DirectX12
{
    inline Microsoft::WRL::ComPtr<IDXGIFactory4> DxgiFactory;
    inline Microsoft::WRL::ComPtr<ID3D12Device> D3DDevice;
    inline Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    inline UINT RtvDescriptorSize, DsvDescriptorSize, CbvDescriptorSize;
    inline const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    inline UINT MsaaQuality;
    inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
    inline Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
    inline Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
    inline Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
    inline const int SwapChainBufferCount = 2;
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RtvHeap;
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DsvHeap;
    inline Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffers[SwapChainBufferCount];
    inline Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencilBuffer;
    inline const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    bool Initialize();

    //D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView()
    //{
    //    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
    //        RtvHeap->GetCPUDescriptorHandleForHeapStart(),
    //        CurrBackBuffer,
    //        RtvDescriptorSize
    //    );
    //}

    //D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView()
    //{
    //    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
    //        DsvHeap->GetCPUDescriptorHandleForHeapStart()
    //    );
    //}
} // namespace DirectX12