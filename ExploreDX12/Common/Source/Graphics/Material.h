#pragma once

#include "Graphics/DirectX12.h"

struct MaterialConstants
{
    MaterialConstants() = default;
    MaterialConstants(XMFLOAT4 diffuseAlbedo, XMFLOAT3 fresnelR0, float roughness)
        : DiffuseAlbedo(diffuseAlbedo), FresnelR0(fresnelR0), Roughness(roughness) { }

    XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;
    XMFLOAT4X4 MatTransform = DirectXMathUtils::Identity4x4();
};

struct Material
{
    Material() = default;
    Material(std::string name, int matCBIndex, XMFLOAT4 diffuseAlbedo, XMFLOAT3 fresnelR0, float roughness)
        : Name(name), MatCBIndex(matCBIndex), DiffuseAlbedo(diffuseAlbedo), FresnelR0(fresnelR0), Roughness(roughness) { }

    // Nom unique du matériau pour la recherche.
    std::string Name;

    // Indice dans le buffer constant correspondant à ce matériau.
    int MatCBIndex = -1;

    // Index dans le tas SRV pour la texture diffuse.
    int DiffuseSrvHeapIndex = -1;

    // Flag indiquant si le matériau a changé et qu'on doit mettre à jour le buffer constant. 
    // Parce qu'on a un buffer constant de matériau pour chaque FrameResource, on doit appliquer la mise à jour à chaque FrameResource.
    // Donc, quand on modifie un matériau, on doit définir NumFramesDirty = gNumFrameResources pour que chaque FrameResource reçoive la mise à jour.
    int NumFramesDirty = DirectX12::NumberOfFrameResources;

    // Données du buffer constant de matériau utilisées pour le shading.
    XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;
    XMFLOAT4X4 MatTransform = DirectXMathUtils::Identity4x4();
};