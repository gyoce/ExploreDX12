# Initialisation

[Page d'accueil](README.md)

Sommaire : 
- [Création du *device*](#création-du-device)
- [Création de la barrière et des tailles de descripteurs](#création-de-la-barrière-et-des-tailles-de-descripteurs)
- [Vérification du support du "4X MSAA"](#vérification-du-support-du-4x-msaa)
- [Création de la file et liste de commande](#création-de-la-file-et-liste-de-commande)
- [Création de la *Swap Chain*](#création-de-la-swap-chain)
- [Création des tas de descripteurs](#création-des-tas-de-descripteurs)
- [Création de la vue de rendu](#création-de-la-vue-de-rendu)
- [Création du tampon de profondeur/stencil et la vue](#création-du-tampon-de-profondeurstencil-et-la-vue)
- [Définition du *viewport*](#définition-du-viewport)
- [Définition du *scissor rect*](#définition-du-scissor-rect)


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
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
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

## Vérification du support du 4X MSAA
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

## Création des tas de descripteurs
[Explications](Overview.md#ressources-et-descripteurs)

On a besoin de créer les tas de descripteurs pour stocker les descripteurs/vues que notre application va utiliser. Un tas de descripteur est représenté par l'interface `ID3D12DescriptorHeap` et est créé avec la méthode `ID3D12Device::CreateDescriptorHeap`. Ici on va avoir besoin de `SwapChainBufferCount` (= 2) *RTV* pour décrire les ressources de tampon et 1 *DSV* pour décrire le *depth/stencil buffer*. 
```cpp
Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
rtvHeapDesc.NumDescriptors = SwapChainBufferCount; // = 2
rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
rtvHeapDesc.NodeMask = 0; 
ThrowIfFailed(
    md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()))
);

D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
dsvHeapDesc.NumDescriptors = 1;
dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
dsvHeapDesc.NodeMask = 0;
ThrowIfFailed(
    md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()))
);
```
Dans notre application on a besoin de savoir quel est le tampon actuel, on peut le faire simplement avec `int mCurrBackBuffer = 0;` qui sera l'index du tampon. 
Après avoir créé les tas de descripteurs, on doit pouvoir accéder aux descripteurs qu'ils contiennent. L'application reférence les descripteurs par leur *handle*. On peut obtenir le *handle* du premier descripteur du tas de descripteur avec la méthode `ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart`. On peut donc avoir deux fonctions qui nous permettent d'obtenir les *handles* qui nous intéressent :
```cpp
D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const
{
    // Ici on utilise un constructeur de CD3DX12 pour *offset* vers le RTV du tampon actuel
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(), 
        mCurrBackBuffer, // Index à offset
        mRtvDescriptorSize // taille en octet du descripteur
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mDsvHeap->GetCPUDescriptorHandleForHeapStart()
    );
}
```

## Création de la vue de rendu
Pour rappel on ne *bind* pas une ressource pour une étape de la pipeline de rendu, à la plce on doit créer une vue de ressource (descripteur) vers la ressource et on *bind* la vue vers l'étape de la pipeline de rendu. Donc, pour *bind* le *back buffer* vers l'étape de rendu, on doit d'abord créer une vue de rendu vers le *back buffer*. La première étape est de récupérer les ressources du tampon qui sont stockées dans la *Swap Chain* : 
```cpp
HRESULT IDXGISwapChain::GetBuffer(
    UINT Buffer, 
    REFIID riid, 
    void **ppSurface
);
```
1. ***Buffer*** : est l'index du tampon dans la *Swap Chain* (0 pour le premier tampon, 1 pour le second, etc.).
2. ***riid*** : est le *COM ID* de l'interface `ID3D12Resource` que l'on veut récupérer.
3. ***ppSurface*** : est un pointeur de sortie vers la ressource tampon récupérée.
ATTENTION : l'appel vers `GetBuffer` augmente le nombre de références du *back buffer*, donc on doit *release* quand on a fini de l'utiliser (c'est automatiquement fait si on utilise `ComPtr`).

Pour créer le *RTV* on utilise : 
```cpp
HRESULT ID3D12Device::CreateRenderTargetView(
    ID3D12Resource *pResource, 
    const D3D12_RENDER_TARGET_VIEW_DESC *pDesc, 
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor
);
```
1. ***pResource*** : spécifie la ressource qui va être utilisée comme cible de rendu, par exemple le *back buffer*.
2. ***pDesc*** : est un pointeur vers la structure `D3D12_RENDER_TARGET_VIEW_DESC` qui décrit le type de donnée des éléments dans la ressource.
3. ***DestDescriptor*** : est le *handle* du descripteur qui stockera la RTV créée.

Voici un exemple de création de *RTV* pour chaque tampon dans la *Swap Chain* :
```cpp
Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffers[SwapChainBufferCount];

CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
for (UINT i = 0; i < SwapChainBufferCount; i++)
{
    ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffers[i])));
    md3dDevice->CreateRenderTargetView(mSwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
    rtvHeapHandle.Offset(1, mRtvDescriptorSize);
}
```

## Création du tampon de profondeur/stencil et la vue
On doit maintenant créer le tampon de profondeur/stencil qui va être utilisé pour le rendu. Une texture est un type de ressource GPU donc on peut la créer en remplissant une structure `D3D12_RESOURCE_DESC` qui décrit la texture :
```cpp
typedef struct D3D12_RESOURCE_DESC 
{
    D3D12_RESOURCE_DIMENSION Dimension;
    UINT Alignment;
    UINT64 Width;
    UINT Height;
    UINT DepthOrArraySize;
    UINT MipLevels;
    DXGI_FORMAT Format;
    D3D12_SAMPLE_DESC SampleDesc;
    D3D12_TEXTURE_LAYOUT Layout;
    D3D12_RESOURCE_MISC_FLAGS MiscFlags;
} D3D12_RESOURCE_DESC;

// Avec l'énumération D3D12_RESOURCE_DIMENSION
typedef enum D3D12_RESOURCE_DIMENSION
{
    D3D12_RESOURCE_DIMENSION_UNKNOWN = 0,
    D3D12_RESOURCE_DIMENSION_BUFFER = 1,
    D3D12_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D12_RESOURCE_DIMENSION_TEXTURE3D = 4
} D3D12_RESOURCE_DIMENSION;
```
1. ***Dimension*** : spécifie la dimension de la ressource cf l'énumération `D3D12_RESOURCE_DIMENSION`.
2. ***Width*** : est la largeur de la texture en texels. Pour les tampons de ressources, c'est la taille en octets dans le tampon.
3. ***Height*** : est la hauteur de la texture en texels.
4. ***DepthOrArraySize*** : est la profondeur de la texture en texels ou la taille du tableau de textures (pour les textures 1D et 2D). A savoir qu'on ne peut pas avoir de tableau de textures 3D.
5. ***MipLevels*** : est le nombre de niveaux de mipmap dans la texture. Pour créer le tampon de profondeur/stencil, on met 1.
6. ***Format*** : est le format de la texture, donc un membre de l'énumération `DXGI_FORMAT`.
7. ***SampleDesc*** : est la description de l'échantillonnage.
8. ***Layout*** : est la disposition des données dans la texture, donc un membre de l'énumération `D3D12_TEXTURE_LAYOUT`.
9. ***MiscFlags*** : est un ensemble de flags divers qui spécifient des options supplémentaires pour la ressource, ici on utilisera `D3D12_RESOURCE_MISC_DEPTH_STENCIL`.

Les ressourcs GPU vivent dans des tas, qui sont essentiellement des blocs de mémoire GPU avec certaines propriétés. On peut créer et attribue une ressource à un tas particulier avec la méthode : 
```cpp
HRESULT ID3D12Device::CreateCommittedResource(
    const D3D12_HEAP_PROPERTIES *pHeapProperties, 
    D3D12_HEAP_MISC_FLAGS HeapMiscFlags, 
    const D3D12_RESOURCE_DESC *pResourceDesc, 
    D3D12_RESOURCE_USAGE InitialResourceState,
    const D3D12_CLEAR_VALUE *pOptimizedClearValue, 
    REFIID riidResource, 
    void **ppvResource
);

// Avec la structure D3D12_HEAP_PROPERTIES
typedef struct D3D12_HEAP_PROPERTIES
{
    D3D12_HEAP_TYPE Type;
    D3D12_CPU_PAGE_PROPERTIES CPUPageProperties;
    D3D12_MEMORY_POOL MemoryPoolPreference;
    UINT CreationNodeMask;
    UINT VisibleNodeMask;
} D3D12_HEAP_PROPERTIES;
```
1. ***pHeapProperties*** : spécifie les propriétés du tas dans lequel la ressource sera allouée. Pour le moment, la principale propriété que l'on va utiliser est le type de tas qui est un membre de l'énumération `D3D12_HEAP_TYPE`.
2. ***HeapMiscFlags*** : spécifie les flags additionnels de tas, généralement on met `D3D12_HEAP_MISC_NONE`.
3. ***pResourceDesc*** : est un pointeur vers la structure `D3D12_RESOURCE_DESC` que l'on a rempli.
4. ***InitialResourceState*** : spécifie l'état initial de la ressource, pour le tampon de profondeur/stencil on met `D3D12_RESOURCE_USAGE_INITIAL` et plus tard on mettre `D3D12_RESOURCE_USAGE_DEPTH` comme ça, il pourra être utilisé dans la pipeline.
5. ***pOptimizedClearValue*** : est un pointeur vers une structure `D3D12_CLEAR_VALUE` qui décrit une valeur optimisée pour le nettoyage de la ressource. 
6. ***riidResource*** : est le *COM ID* de l'interface `ID3D12Resource` que l'on veut créer.
7. ***ppvResource*** : est un pointeur de sortie vers la ressource créée.

Avant d'utiliser le tampon de profondeur/stencil, on doit d'abord créer une vue de profondeur/stencil qui sera lié à la pipeline de rendu. On peut la créer de cette manière :  
```cpp
Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

D3D12_RESOURCE_DESC depthStencilDesc;
depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
depthStencilDesc.Alignment = 0;
depthStencilDesc.Width = mClientWidth;
depthStencilDesc.Height = mClientHeight;
depthStencilDesc.DepthOrArraySize = 1;
depthStencilDesc.MipLevels = 1;
depthStencilDesc.Format = mDepthStencilFormat;
depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

D3D12_CLEAR_VALUE optClear;
optClear.Format = mDepthStencilFormat;
optClear.DepthStencil.Depth = 1.0f;
optClear.DepthStencil.Stencil = 0;
ThrowIfFailed(
    md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
    )
);

md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
```
- Ici on se sert du constructeur `CD3DX12_HEAP_PROPERTIES` qui nous permet de créer une instance de la structure `D3D12_HEAP_PROPERTIES`.
- Le second paramètre de `CreateDepthStencilView` est un pointeur vers la structure `D3D12_DEPTH_STENCIL_VIEW_DESC`.

## Définition du *viewport*
Habituellement on rend la scène 3D en entier dans le *back buffer* avec la taille du tampon qui correspond à la taille de l'écran ou de la fenêtre. Mais on peut aussi définir un *viewport* qui est une zone de rendu dans le tampon. Le *viewport* est défini par la structure : 
```cpp
typedef struct D3D12_VIEWPORT 
{
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
} D3D12_VIEWPORT;
```
On peut voir que les 4 premiers paramètres définissent le rectangle de rendu dans le tampon. On rappelle que les valeurs de profondeurs sont normalisées entre 0.0 et 1.0, les paramètres `MinDepth` et `MaxDepth` permettent de transformer l'intervalle de profondeur de [0, 1] vers [MinDepth, MaxDepth] mais habituellement on met `MinDepth` à 0.0 et `MaxDepth` à 1.0 pour que les valeurs ne soient pas modifiées. Pour définir le *viewport* on peut faire : 
```cpp
D3D12_VIEWPORT vp;
vp.TopLeftX = 0.0f;
vp.TopLeftY = 0.0f;
vp.Width = static_cast<FLOAT>(mClientWidth);
vp.Height = static_cast<FLOAT>(mClientHeight);
vp.MinDepth = 0.0f;
vp.MaxDepth = 1.0f;
mCommandList->RSSetViewports(1, &vp);
```

## Définition du *scissor rect*
On peut définir une zone de découpe (*scissor rect*) qui permet de ne pas rendre les pixels en dehors de cette zone. La zone de découpe est définie par la structure :
```cpp
typedef struct D3D12_RECT 
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} D3D12_RECT;
```
On peut alors définir le *scissor rect* de la manière suivante :
```cpp
D3D12_RECT mScissorRect = {0, 0, mClientWidth / 2, mClientHeight / 2};
mCommandList->RSSetScissorRects(1, &mScissorRect);
```