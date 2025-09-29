#pragma once

#include "Graphics/DirectXUtils.h"
#include "Graphics/UploadBuffer.h"
#include "Graphics/Light.h"
#include "Graphics/Material.h"

struct ObjectConstants
{
    XMFLOAT4X4 World = DirectXMathUtils::Identity4x4();
};

struct PassConstants
{
    XMFLOAT4X4 View = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 InvView = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 Proj = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 InvProj = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 ViewProj = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 InvViewProj = DirectXMathUtils::Identity4x4();
    XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
    Light Lights[MaxLights];
};

struct Vertex
{
    Vertex() = default;
    Vertex(XMFLOAT3 pos, XMFLOAT3 normal)
        : Pos(pos), Normal(normal) { }

    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
};

// Permet de stocker les ressources nécessaires au CPU pour construire les listes de commande pour une frame.
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT waveVertexCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandListAllocator.GetAddressOf())));
        PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
        MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
        WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, waveVertexCount, false);
    }
    ~FrameResource() = default;
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;

    // On ne peut pas réinitialiser l'allocateur tant que le GPU n'a pas fini de traiter les commandes. Donc chaque frame a son propre allocateur.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandListAllocator;

    // On ne peut pas mettre à jour un constant buffer tant que le GPu n'a pas fini de traiter les commandes qui le référencent. Donc chaque frame a son propre cbuffer.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    std::unique_ptr<UploadBuffer<Vertex>> WavesVB = nullptr;

    // Valeur de la barrière pour marquer les commandes jusqu'à ce point. Cela nous permet de vérifier si ces ressources de frame sont toujours utilisées par le GPU.
    UINT64 Fence = 0;
};