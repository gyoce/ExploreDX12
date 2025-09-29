#pragma once

#include "Application.h"
#include "Graphics/UploadBuffer.h"
#include "Graphics/MeshGeometry.h"

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = DirectXMathUtils::Identity4x4();
};

class BoxApp : public Application
{
public:
    BoxApp(HINSTANCE hInstance);

    void OnWindowResize() override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;

protected:
    void Update() override;
    void Draw() override;

private:
    bool Initialize();

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = nullptr;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 mView = DirectXMathUtils::Identity4x4();
    XMFLOAT4X4 mProj = DirectXMathUtils::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;
};