#pragma once

#include "Application.h"
#include "Graphics/MeshGeometry.h"
#include "Graphics/GeometryGenerator.h"
#include "FrameResource.h"

constexpr int gNumberOfFrameResources = 3;

// Structure légère qui stocke les paramètres pour dessiner une forme. 
struct RenderItem
{
    RenderItem() = default;

    // Matrice monde de l'objet qui décrit l'espace local de l'objet par rapport à l'espace monde.
    // Elle définit la position, l'orientation et l'échelle de l'objet dans le monde.
    XMFLOAT4X4 World = MathUtils::Identity4x4();

    // Indicateur de modification indiquant que les données de l'objet ont changé et qu'on doit mettre à jour le constant buffer.
    // Parce qu'on a un objet cbuffer pour chaque FrameResource, on doit appliquer la mise à jour à chaque FrameResource. 
    // Donc, quand on modifie les données de l'objet, on doit définir NumFramesDirty = gNumFrameResources pour que chaque ressource de frame reçoive la mise à jour.
    int NumFramesDirty = gNumberOfFrameResources;

    // Indice dans le buffer constant GPU correspondant à l'ObjectCB pour ce render item.
    UINT ObjCBIndex = -1;

    // Geometry associé à ce render item. Note : plusieurs render items peuvent partager la même géométrie.
    MeshGeometry* Geo = nullptr;

    // Type de primitive.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // Paramètres de la fonction DrawIndexedInstanced.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class ShapesApp : public Application
{
public:
    ShapesApp(HINSTANCE hInstance);
    ~ShapesApp();

    void OnWindowResize() override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;
    void OnKeyDown(WPARAM wParam) override;

protected:
    void Update() override;
    void Draw() override;

private:
    void UpdateCamera();
    void UpdateObjectCBs();
    void UpdateMainPassCB();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

    bool Initialize();

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildRenderItems();
    void BuildFrameResources();
    void BuildDescriptorHeaps();
    void BuildConstantBufferViews();
    void BuildPSOs();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrentFrameResource = nullptr;
    int mCurrentFrameResourceIndex = 0;
    PassConstants mMainPassCB;

    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<RenderItem*> mOpaqueRitems;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

    UINT mPassCbvOffset = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 mView = MathUtils::Identity4x4();
    XMFLOAT4X4 mProj = MathUtils::Identity4x4();
    float mTheta = 1.5f * XM_PI;
    float mPhi = 0.2f * XM_PI;
    float mRadius = 15.0f;
    bool mIsWireframe = false;
    POINT mLastMousePos;
};