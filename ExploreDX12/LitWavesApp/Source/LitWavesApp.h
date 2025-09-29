#pragma once

#include "Application.h"
#include "Graphics/MeshGeometry.h"
#include "FrameResource.h"
#include "Waves.h"

// Structure légère qui stocke les paramètres pour dessiner une forme. 
struct RenderItem
{
    RenderItem() = default;
    RenderItem(XMFLOAT4X4 world, int objCBIndex, Material* mat, MeshGeometry* geo, D3D12_PRIMITIVE_TOPOLOGY primitiveType, UINT indexCount, UINT startIndexLocation, int baseVertexLocation)
        : World(world), ObjCBIndex(objCBIndex), Mat(mat), Geo(geo), PrimitiveType(primitiveType), IndexCount(indexCount), StartIndexLocation(startIndexLocation), BaseVertexLocation(baseVertexLocation) {  }

    // Matrice monde de l'objet qui décrit l'espace local de l'objet par rapport à l'espace monde.
    // Elle définit la position, l'orientation et l'échelle de l'objet dans le monde.
    XMFLOAT4X4 World = DirectXMathUtils::Identity4x4();

    XMFLOAT4X4 TexTransform = DirectXMathUtils::Identity4x4();

    // Indicateur de modification indiquant que les données de l'objet ont changé et qu'on doit mettre à jour le constant buffer.
    // Parce qu'on a un objet cbuffer pour chaque FrameResource, on doit appliquer la mise à jour à chaque FrameResource. 
    // Donc, quand on modifie les données de l'objet, on doit définir NumFramesDirty = gNumFrameResources pour que chaque ressource de frame reçoive la mise à jour.
    int NumFramesDirty = DirectX12::NumberOfFrameResources;

    // Indice dans le buffer constant GPU correspondant à l'ObjectCB pour ce render item.
    UINT ObjCBIndex = -1;

    Material* Mat = nullptr;

    // Geometry associé à ce render item. Note : plusieurs render items peuvent partager la même géométrie.
    MeshGeometry* Geo = nullptr;

    // Type de primitive.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // Paramètres de la fonction DrawIndexedInstanced.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class LitWavesApp : public Application
{
public:
    LitWavesApp(HINSTANCE hInstance);
    ~LitWavesApp();

    void OnWindowResize() override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;

protected:
    void Update() override;
    void Draw() override;

private:
    bool Initialize();

    void UpdateCamera();
    void UpdateObjectCBs();
    void UpdateMainPassCB();
    void UpdateMaterialCBs();
    void UpdateWaves();
    void UpdateKeyboardInput();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList);

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometryBuffers();
    void BuildMaterials();
    void BuildRenderItems();
    void BuildFrameResources();
    void BuildPSOs();

    static float GetHillsHeight(float x, float z);
    static XMFLOAT3 GetHillsNormal(float x, float z);

    std::unique_ptr<Waves> mWaves = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrentFrameResource = nullptr;
    int mCurrentFrameResourceIndex = 0;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

    RenderItem* mWavesRenderitem = nullptr;
    PassConstants mMainPassCB;

    XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 mView = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 mProj = DirectXMathUtils::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV2 - 0.1f;
    float mRadius = 50.0f;

    float mSunTheta = 1.25f * XM_PI;
    float mSunPhi = XM_PIDIV4;

    POINT mLastMousePos;
};