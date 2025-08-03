# Initialisation

[Page d'accueil](Main.md)

Sommaire : 
- [Création du *device*](#création-du-device)
- [Création de la barrière et des tailles de descripteurs](#création-de-la-barrière-et-des-tailles-de-descripteurs)

## Création du *device*
Pour initialiser DirectX12 on doit d’abord créer un `ID3D12Device`. Ce *device* représente un *display adapter* (un GPU par exemple). Pour créer ce device on peut le faire grâce à : 
```cpp
HRESULT WINAPI D3D12CreateDevice(
   IUnknown* pAdapter, 
   D3D_FEATURE_LEVEL MinimumFeatureLevel, 
   REFIID riid, 
   void** ppDevice
);
```
- ***pAdapter*** spécifie le *display adapter* à représenter, si on met *null* alors on utilise le *display adapter* principal.
- ***MinimumFeatureLevel*** spécifie le niveau de fonctionnalités minimal attendu (par exemple *D3D_FEATURE_LEVEL_11_0*).
- ***riid*** est le *COM ID* de l’interface *I3D12Device* que l’on veut créer.
- ***ppDevice*** permet de nous retourner le résultat du *device* créé.

Exemple :
```cpp
Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

// 1
#if defined(DEBUG) || defined(_DEBUG)
{
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    debugController->EnableDebugLayer();
}
#endif

// 2
ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));

// 3
if(FAILED(hardwareResult))
{
    ComPtr<IDXGIAdapter> pWarpAdapter;
    ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)));
}
```
1. On active d’abord le *debug layer* pour des builds de debug car on aura plus de messages de debug.
2. On créé le *device*
3. Si l’appel *fail* alors on se rabat vers un *WARP Device.*

## Création de la barrière et des tailles de descripteurs
On peut maintenant créer la barrière qui permet de faire une synchronisation CPU/GPU.
```cpp
Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
UINT mRtvDescriptorSize, mDsvDescriptorSize, mCbvDescriptorSize;

ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
mCbvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
```