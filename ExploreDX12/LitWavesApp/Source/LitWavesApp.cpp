#include "LitWavesApp.h"

#include "Graphics/GeometryGenerator.h"

LitWavesApp::LitWavesApp(HINSTANCE hInstance)
    : Application(hInstance)
{
    if (mIsInit)
        mIsInit &= Initialize();
}

LitWavesApp::~LitWavesApp()
{
    if (DirectX12::D3DDevice != nullptr)
        DirectX12::FlushCommandQueue();
}

void LitWavesApp::OnWindowResize()
{
    Application::OnWindowResize();

    // Quand la fenêtre est resize, on doit mettre à jour l'aspect ratio et recalculer la matrice de projection.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * DirectXMathUtils::Pi, WindowManager::AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void LitWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    Application::OnMouseDown(btnState, x, y);

    mLastMousePos.x = x;
    mLastMousePos.y = y;
    WindowManager::CaptureMouse();
}

void LitWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    Application::OnMouseUp(btnState, x, y);

    WindowManager::ReleaseCaptureMouse();
}

void LitWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    Application::OnMouseMove(btnState, x, y);

    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
        mTheta += dx;
        mPhi += dy;
        mPhi = DirectXMathUtils::Clamp(mPhi, 0.1f, DirectXMathUtils::Pi - 0.1f);
    } 
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);
        mRadius += dx - dy;
        mRadius = DirectXMathUtils::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void LitWavesApp::Update()
{
    Application::Update();

    UpdateCamera();
    UpdateKeyboardInput();

    // On boucle circulairement sur les frame resources.
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % DirectX12::NumberOfFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

    // Est-ce que le GPU a fini de traiter les commandes de la frame resource courante ? Si ce n'est pas le cas, on attend que le GPU ait fini de traiter les commandes jusqu'à cette barrière.
    if (mCurrentFrameResource->Fence != 0 && DirectX12::Fence->GetCompletedValue() < mCurrentFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(DirectX12::Fence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCBs();
    UpdateMainPassCB();
    UpdateMaterialCBs();
    UpdateWaves();
}

void LitWavesApp::Draw()
{
    Application::Draw();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrentFrameResource->CommandListAllocator;
    ThrowIfFailed(cmdListAlloc->Reset());
    ThrowIfFailed(DirectX12::CommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    DirectX12::CommandList->RSSetViewports(1, &DirectX12::ScreenViewport);
    DirectX12::CommandList->RSSetScissorRects(1, &DirectX12::ScissorRect);

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DirectX12::CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    DirectX12::CommandList->ResourceBarrier(1, &resourceBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE cbbv = DirectX12::CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DirectX12::DepthStencilView();
    DirectX12::CommandList->ClearRenderTargetView(cbbv, Colors::LightSteelBlue, 0, nullptr);
    DirectX12::CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    DirectX12::CommandList->OMSetRenderTargets(1, &cbbv, true, &dsv);

    DirectX12::CommandList->SetGraphicsRootSignature(mRootSignature.Get());

    ID3D12Resource* passCB = mCurrentFrameResource->PassCB->Resource();
    DirectX12::CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(DirectX12::CommandList.Get());

    resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DirectX12::CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    DirectX12::CommandList->ResourceBarrier(1, &resourceBarrier);

    ThrowIfFailed(DirectX12::CommandList->Close());

    ID3D12CommandList* cmdsLists[] = { DirectX12::CommandList.Get() };
    DirectX12::CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(DirectX12::SwapChain->Present(0, 0));
    DirectX12::CurrentBackBufferIndex = (DirectX12::CurrentBackBufferIndex + 1) % DirectX12::SwapChainBufferCount;

    mCurrentFrameResource->Fence = ++DirectX12::CurrentFence;

    DirectX12::CommandQueue->Signal(DirectX12::Fence.Get(), DirectX12::CurrentFence);
}

bool LitWavesApp::Initialize()
{
    DirectX12::ResetCommandList();

    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildLandGeometry();
    BuildWavesGeometryBuffers();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    DirectX12::ExecuteCommandsAndWait();

    return true;
}

void LitWavesApp::UpdateCamera()
{
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // Construction de la matrice de vue.
    XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
}

void LitWavesApp::UpdateObjectCBs()
{
    UploadBuffer<ObjectConstants>* currentObjectCB = mCurrentFrameResource->ObjectCB.get();
    for (const std::unique_ptr<RenderItem>& e : mAllRenderItems)
    {
        // On ne met à jour les données du cbuffer que si les constantes ont changé.
        // Attention : cela doit être suivi pour chaque frame ressource.
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            currentObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Le prochain FrameResource doit aussi être mis à jour.
            e->NumFramesDirty--;
        }
    }
}

void LitWavesApp::UpdateMainPassCB()
{
    const int clientWidth = WindowManager::GetWidth();
    const int clientHeight = WindowManager::GetHeight();

    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    XMVECTOR viewDeterminant = XMMatrixDeterminant(view);
    XMVECTOR projDeterminant = XMMatrixDeterminant(proj);
    XMVECTOR viewProjDeterminant = XMMatrixDeterminant(viewProj);

    XMMATRIX invView = XMMatrixInverse(&viewDeterminant, view);
    XMMATRIX invProj = XMMatrixInverse(&projDeterminant, proj);
    XMMATRIX invViewProj = XMMatrixInverse(&viewProjDeterminant, viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.RenderTargetSize = XMFLOAT2(static_cast<float>(clientWidth), static_cast<float>(clientHeight));
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / clientWidth, 1.0f / clientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    //mMainPassCB.TotalTime = gameTimer.TotalTime();
    mMainPassCB.DeltaTime = TimeManager::GetDeltaTime();
    mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

    XMVECTOR lightDir = -DirectXMathUtils::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };

    UploadBuffer<PassConstants>* currentPassCB = mCurrentFrameResource->PassCB.get();
    currentPassCB->CopyData(0, mMainPassCB);
}

void LitWavesApp::UpdateMaterialCBs()
{
    UploadBuffer<MaterialConstants>* currentMaterialCB = mCurrentFrameResource->MaterialCB.get();
    for (const auto& [_, mat] : mMaterials)
    {
        // On ne met à jour les données du cbuffer que si les constantes ont changé.
        // Attention : cela doit être suivi pour chaque frame ressource.
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
            MaterialConstants matConstants(mat->DiffuseAlbedo, mat->FresnelR0, mat->Roughness);
            currentMaterialCB->CopyData(mat->MatCBIndex, matConstants);
            mat->NumFramesDirty--;
        }
    }
}

void LitWavesApp::UpdateWaves()
{
    static float tBase = 0.0f;

    if ((TimeManager::GetTotalTime() - tBase) > 0.25f)
    {
        tBase += 0.25f;
        int i = DirectXMathUtils::Rand(4, mWaves->RowCount() - 5);
        int j = DirectXMathUtils::Rand(4, mWaves->ColumnCount() - 5);
        float magnitude = DirectXMathUtils::Randf(0.2f, 0.5f);
        mWaves->Disturb(i, j, magnitude);
    }

    mWaves->Update();

    UploadBuffer<Vertex>* currentWavesVB = mCurrentFrameResource->WavesVB.get();
    for (int i = 0; i < mWaves->VertexCount(); i++)
    {
        Vertex v(mWaves->Position(i), mWaves->Normal(i));
        currentWavesVB->CopyData(i, v);
    }

    mWavesRenderitem->Geo->VertexBufferGPU = currentWavesVB->Resource();
}

void LitWavesApp::UpdateKeyboardInput()
{
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        mSunTheta -= 1.0f * TimeManager::GetDeltaTime();
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        mSunTheta += 1.0f * TimeManager::GetDeltaTime();
    if (GetAsyncKeyState(VK_UP) & 0x8000)
        mSunPhi -= 1.0f * TimeManager::GetDeltaTime();
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        mSunPhi += 1.0f * TimeManager::GetDeltaTime();

    mSunPhi = DirectXMathUtils::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}

void LitWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList)
{
    UINT objCBByteSize = DirectXUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = DirectXUtils::CalcConstantBufferByteSize(sizeof(MaterialConstants));

    ID3D12Resource* objectCB = mCurrentFrameResource->ObjectCB->Resource();
    ID3D12Resource* matCB = mCurrentFrameResource->MaterialCB->Resource();

    for (const std::unique_ptr<RenderItem>& ri : mAllRenderItems)
    {
        D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
        D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();

        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void LitWavesApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];
    
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
        Logs::Error("Error while doing D3D12SerializeRootSignature : {}", (char*)errorBlob->GetBufferPointer());
    ThrowIfFailed(hr);

    ThrowIfFailed(DirectX12::D3DDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void LitWavesApp::BuildShadersAndInputLayout()
{
    mShaders["standardVS"] = DirectXUtils::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = DirectXUtils::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void LitWavesApp::BuildLandGeometry()
{
    GeometryGenerator::MeshData grid = GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);

    std::vector<Vertex> vertices(grid.Vertices.size());
    for (size_t i = 0; i < grid.Vertices.size(); i++)
    {
        XMFLOAT3& pos = grid.Vertices[i].Position;
        vertices[i].Pos = pos;
        vertices[i].Pos.y = GetHillsHeight(pos.x, pos.z);
        vertices[i].Normal = GetHillsNormal(pos.x, pos.z);
    }

    const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "landGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = DirectXUtils::CreateDefaultBuffer(DirectX12::D3DDevice.Get(), DirectX12::CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
    geo->IndexBufferGPU = DirectXUtils::CreateDefaultBuffer(DirectX12::D3DDevice.Get(), DirectX12::CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;
    geo->DrawArgs["grid"] = SubmeshGeometry(static_cast<UINT>(indices.size()), 0, 0);;
    mGeometries["landGeo"] = std::move(geo);
}

void LitWavesApp::BuildWavesGeometryBuffers()
{
    // 3 indices par face
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount());
    assert(mWaves->VertexCount() < 0x0000ffff);

    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for (int i = 0; i < m - 1; i++)
    {
        for (int j = 0; j < n - 1; j++)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; // prochain quad
        }
    }

    UINT vbByteSize = static_cast<UINT>(mWaves->VertexCount() * sizeof(Vertex));
    UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
    
    geo->IndexBufferGPU = DirectXUtils::CreateDefaultBuffer(DirectX12::D3DDevice.Get(), DirectX12::CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;
    geo->DrawArgs["grid"] = SubmeshGeometry(static_cast<UINT>(indices.size()), 0, 0);
    mGeometries["waterGeo"] = std::move(geo);
}

void LitWavesApp::BuildMaterials()
{
    mMaterials["grass"] = std::make_unique<Material>("grass", 0, XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.125f);
    mMaterials["water"] = std::make_unique<Material>("water", 1, XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.0f);
}

void LitWavesApp::BuildRenderItems()
{
    MeshGeometry* waterGeo = mGeometries["waterGeo"].get();
    std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>(DirectXMathUtils::Identity4x4(), 0, mMaterials["water"].get(), waterGeo, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, waterGeo->DrawArgs["grid"].IndexCount, waterGeo->DrawArgs["grid"].StartIndexLocation, waterGeo->DrawArgs["grid"].BaseVertexLocation);

    MeshGeometry* landGeo = mGeometries["landGeo"].get();
    std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>(DirectXMathUtils::Identity4x4(), 1, mMaterials["grass"].get(), landGeo, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, landGeo->DrawArgs["grid"].IndexCount, landGeo->DrawArgs["grid"].StartIndexLocation, landGeo->DrawArgs["grid"].BaseVertexLocation);

    mWavesRenderitem = wavesRenderItem.get();
    mAllRenderItems.push_back(std::move(wavesRenderItem));
    mAllRenderItems.push_back(std::move(gridRenderItem));
}

void LitWavesApp::BuildFrameResources()
{
    for (int i = 0; i < DirectX12::NumberOfFrameResources; i++)
        mFrameResources.push_back(std::make_unique<FrameResource>(DirectX12::D3DDevice.Get(), 1, static_cast<UINT>(mAllRenderItems.size()), static_cast<UINT>(mMaterials.size()), mWaves->VertexCount()));
}

void LitWavesApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { mInputLayout.data(), static_cast<UINT>(mInputLayout.size()) };
    opaquePsoDesc.pRootSignature = mRootSignature.Get();
    opaquePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize() };
    opaquePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize() };
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = DirectX12::BackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = 1;
    opaquePsoDesc.SampleDesc.Quality = 0;
    opaquePsoDesc.DSVFormat = DirectX12::DepthStencilFormat;
    ThrowIfFailed(DirectX12::D3DDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

float LitWavesApp::GetHillsHeight(float x, float z)
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 LitWavesApp::GetHillsNormal(float x, float z)
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
        1.0f,
        -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z)
    );

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);
    return n;
}
