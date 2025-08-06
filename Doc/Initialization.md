# Initialisation

[Page d'accueil](Main.md)

Sommaire : 
- [Création du *device*](#création-du-device)
- [Création de la barrière et des tailles de descripteurs](#création-de-la-barrière-et-des-tailles-de-descripteurs)
- [Vérification du support du "4X MSAA"](##vérification-du-support-du-4x-msaa)
- [Création de la file et liste de commande](#création-de-la-file-et-liste-de-commande)

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
[Explications](Overview.md#ressources-et-descripteurs)

On peut maintenant créer la barrière qui permet de faire une synchronisation CPU/GPU.
```cpp
Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
UINT mRtvDescriptorSize, mDsvDescriptorSize, mCbvDescriptorSize;

ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
mCbvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
```

## Vérification du support du "4X MSAA"
On peut vérifier le support de la fonctionnalité *4X MSAA* car elle donne une meilleure qualité d'image sans pour autant être trop gourmande en ressources. Mais on doit vérifier le niveau de qualité pris en charge avec : 
```cpp
Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
UINT m4xMsaaQuality;

D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
msQualityLevels.Format = mBackBufferFormat;
msQualityLevels.SampleCount = 4;
msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
msQualityLevels.NumQualityLevels = 0;
ThrowIfFailed(
    md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels))
);
m4xMsaaQuality = msQualityLevels.NumQualityLevels;
assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
```

## Création de la file et liste de commande
[Explications](Overview.md#file-et-liste-de-commande)

On peut créer ces deux éléments par le processus suivant : 
```cpp
Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
ComPtr<ID3D12CommandQueue> mCommandQueue;
ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
ComPtr<ID3D12GraphicsCommandList> mCommandList;

D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
ThrowIfFailed(
    md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue))
);
ThrowIfFailed(
    md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf()))
);
ThrowIfFailed(
    md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf()))
);
mCommandList->Close();
```

## Création de la *Swap Chain*
[Explications](Overview.md#swap-chain)

Pour créer la *Swap Chain*, on doit d'abord remplir une instance de la structure `DXGI_SWAP_CHAIN_DESC` qui décrit les caractéristiques de la *Swap Chain*. Cette structure est définie comme suit : 
```cpp
typedef struct DXGI_SWAP_CHAIN_DESC 
{
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage; 
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    DXGI_SWAP_EFFECT SwapEffect;
    UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

// Avec la structure DXGI_MODE_DESC
typedef struct DXGI_MODE_DESC 
{
    UINT Width; // Largeur de résolution du tampon
    UINT Height; // Hauteur de résolution du tampon
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format; // Format du tampon
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering; // Progressive vs interlaced
    DXGI_MODE_SCALING Scaling; // Comment l'image est étirée sur l'écran
} DXGI_MODE_DESC;

// Avec la structure DXGI_SAMPLE_DESC
typedef struct DXGI_SAMPLE_DESC
{
    UINT Count; // Nombre d'échantillons pour le multisampling
    UINT Quality; // Qualité du multisampling
} DXGI_SAMPLE_DESC;
```
1. ***BufferDesc*** : décrit les propriétés du *back buffer* que l'on veut créer. 
2. ***SampleDesc*** : décrit le *multisampling* et le niveau de qualité. Par exemple pour un échantillonnage simple on peut mettre `Count` à 1 et `Quality` à 0.
3. ***BufferUsage*** : indique comment le tampon sera utilisé, par exemple `DXGI_USAGE_RENDER_TARGET_OUTPUT` comme on va rendre vers le *back buffer*.
4. ***BufferCount*** : indique le nombre de tampons dans la *Swap Chain*, par exemple 2 pour un *double buffering*.
5. ***OutputWindow*** : est le *handle* vers la fenêtre de rendu.
6. ***Windowed*** : *true* pour un rendu fenêtré, *false* pour un rendu en plein écran.
7. ***SwapEffect*** : indique l'effet de swap, par exemple `DXGI_SWAP_EFFECT_FLIP_DISCARD` pour un swap simple.
8. ***Flags*** : indique les options supplémentaires, par exemple `DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH` pour permettre de changer le mode d'affichage.

On peut ensuite créer la *Swap Chain* avec la méthode
```cpp
HRESULT IDXGIFactory::CreateSwapChain(
    IUnknown *pDevice, 
    DXGI_SWAP_CHAIN_DESC *pDesc, 
    IDXGISwapChain **ppSwapChain
);
```
1. ***pDevice*** : est le pointeur vers le `ID3D12CommandQueue`
2. ***pDesc*** : est un pointeur vers la structure `DXGI_SWAP_CHAIN_DESC` que l'on a rempli.
3. ***ppSwapChain*** : est un pointeur de sortie vers la *Swap Chain* créée.

Voici un exemple de création de la *Swap Chain* :
```cpp
Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
int mClientWidth = 800;
int mClientHeight = 600;
DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
bool m4xMsaaState;
UINT m4xMsaaQuality;
static const int SwapChainBufferCount = 2;
HWND mhMainWnd;

DXGI_SWAP_CHAIN_DESC sd;
sd.BufferDesc.Width = mClientWidth;
sd.BufferDesc.Height = mClientHeight;
sd.BufferDesc.RefreshRate.Numerator = 60;
sd.BufferDesc.RefreshRate.Denominator = 1;
sd.BufferDesc.Format = mBackBufferFormat;
sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
sd.BufferCount = SwapChainBufferCount;
sd.OutputWindow = mhMainWnd;
sd.Windowed = true;
sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

ThrowIfFailed(
    mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf())
);
```