#include "BoxApp.h"

#include <array>

BoxApp::BoxApp(HINSTANCE hInstance)
    : Application(hInstance)
{
    if (mIsInit)
        mIsInit &= Initialize();        
}

void BoxApp::OnWindowResize()
{
    Application::OnWindowResize();

    // Quand la fenêtre est resize, on doit mettre à jour l'aspect ratio et recalculer la matrice de projection.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathUtils::Pi, WindowManager::AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    Application::OnMouseDown(btnState, x, y);

    mLastMousePos.x = x;
    mLastMousePos.y = y;

    WindowManager::CaptureMouse();
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    Application::OnMouseUp(btnState, x, y);

    WindowManager::ReleaseCaptureMouse();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    Application::OnMouseMove(btnState, x, y);

    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        mTheta += dx;
        mPhi += dy;

        mPhi = MathUtils::Clamp(mPhi, 0.1f, MathUtils::Pi - 0.1f);
    } 
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        mRadius += dx - dy;

        mRadius = MathUtils::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::Update()
{
    Application::Update();

    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    // On met à jour le buffer constant avec la dernière matrice WorldViewProj 
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw()
{
    Application::Draw();

    // On réutilise la mémoire associée à l'enregistrement des commandes.
    ThrowIfFailed(DirectX12::CommandListAllocator->Reset());

    // Une liste de commande peut être reset après qu'elle a été ajouté à la file de commande avec via ExecuteCommandList, ce qui nous permet de réutiliser la mémoire de la liste de commande.
    ThrowIfFailed(DirectX12::CommandList->Reset(DirectX12::CommandListAllocator.Get(), mPSO.Get()));

    DirectX12::CommandList->RSSetViewports(1, &DirectX12::ScreenViewport);
    DirectX12::CommandList->RSSetScissorRects(1, &DirectX12::ScissorRect);

    // On indique une transition d'état sur l'utilisation de la ressource.
    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DirectX12::CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    DirectX12::CommandList->ResourceBarrier(1, &resourceBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE bbv = DirectX12::CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DirectX12::DepthStencilView();
    // On clear le back buffer et le depth buffer.
    DirectX12::CommandList->ClearRenderTargetView(bbv, Colors::LightSteelBlue, 0, nullptr);
    DirectX12::CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    // On précise les buffers sur lesquels on va dessiner.
    DirectX12::CommandList->OMSetRenderTargets(1, &bbv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    DirectX12::CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    DirectX12::CommandList->SetGraphicsRootSignature(mRootSignature.Get());

    D3D12_VERTEX_BUFFER_VIEW vbv = mBoxGeo->VertexBufferView();
    D3D12_INDEX_BUFFER_VIEW ibv = mBoxGeo->IndexBufferView();
    DirectX12::CommandList->IASetVertexBuffers(0, 1, &vbv);
    DirectX12::CommandList->IASetIndexBuffer(&ibv);
    DirectX12::CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DirectX12::CommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    DirectX12::CommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DirectX12::CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    DirectX12::CommandList->ResourceBarrier(1, &resourceBarrier);

    // Ici, on a fini d'enregistrer les commandes.
    ThrowIfFailed(DirectX12::CommandList->Close());
    // On ajoute la liste de commande à la file pour qu'elle s'exécute.
    ID3D12CommandList* cmdsLists[] = { DirectX12::CommandList.Get() };
    DirectX12::CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // On swap le back buffer et le front buffer.
    ThrowIfFailed(DirectX12::SwapChain->Present(0, 0));
    DirectX12::CurrentBackBufferIndex = (DirectX12::CurrentBackBufferIndex + 1) % DirectX12::SwapChainBufferCount;

    // On attend que les commandes soient finies. Cette manière de faire n'est pas efficace mais est faite par soucis de simplicité.
    DirectX12::FlushCommandQueue();
}

bool BoxApp::Initialize()
{
    DirectX12::ResetCommandList();

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    DirectX12::ExecuteCommandsAndWait();

    return true;
}

void BoxApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(DirectX12::D3DDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(DirectX12::D3DDevice.Get(), 1, true);

    UINT objCBByteSize = D3DUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objCBByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = D3DUtils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    DirectX12::D3DDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
        Logs::Error("Error while doing D3D12SerializeRootSignature : {}", (char*)errorBlob->GetBufferPointer());
    ThrowIfFailed(hr);

    ThrowIfFailed(DirectX12::D3DDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    mvsByteCode = D3DUtils::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = D3DUtils::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices = {
        Vertex { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   },
        Vertex { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
        Vertex { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
        Vertex { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
        Vertex { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)    },
        Vertex { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
        Vertex { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)    },
        Vertex { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) },
    };

    std::array<std::uint16_t, 36> indices = {
        0, 1, 2,   0, 2, 3,
        4, 6, 5,   4, 7, 6,
        4, 5, 1,   4, 1, 0,
        3, 2, 6,   3, 6, 7,
        1, 5, 6,   1, 6, 2,
        4, 0, 3,   4, 3, 7,
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mBoxGeo->VertexBufferGPU = D3DUtils::CreateDefaultBuffer(DirectX12::D3DDevice.Get(), DirectX12::CommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
    mBoxGeo->IndexBufferGPU = D3DUtils::CreateDefaultBuffer(DirectX12::D3DDevice.Get(), DirectX12::CommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(Vertex);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DirectX12::BackBufferFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = DirectX12::DepthStencilFormat;
    ThrowIfFailed(DirectX12::D3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}