#pragma once

#include "Graphics/D3DUtils.h"
#include "Graphics/UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathUtils::Identity4x4();
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathUtils::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathUtils::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathUtils::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathUtils::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathUtils::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathUtils::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// Permet de stocker les ressources nécessaires au CPU pour construire les listes de commande pour une frame.
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));
        PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    }
    ~FrameResource() = default;
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;

    // On ne peut pas réinitialiser l'allocateur tant que le GPU n'a pas fini de traiter les commandes. Donc chaque frame a son propre allocateur.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // On ne peut pas mettre à jour un constant buffer tant que le GPu n'a pas fini de traiter les commandes qui le référencent. Donc chaque frame a son propre cbuffer.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Valeur de la barrière pour marquer les commandes jusqu'à ce point. Cela nous permet de vérifier si ces ressources de frame sont toujours utilisées par le GPU.
    UINT64 Fence = 0;
};