# Vue d'ensemble

[Page d'accueil](README.md)

Sommaire :
- [COM](#com)
- [Swap Chain](#swap-chain)
- [Ressources et descripteurs](#ressources-et-descripteurs)
- [File et liste de commande](#file-et-liste-de-commande)

## COM
*Component Object Model* est la technologie qui permet à DirectX d’être langage indépendant et d’être retro-compatible. Les objets COM sont comme des interfaces qui sont à peu près équivalentes à des classes C++ ou à des *shared_ptr*. 
```cpp
#include <wrl.h>

Microsoft::WRL::ComPtr<OBJECT> obj;

// Retourne un pointeur de l'objet sous-jacent.
obj->Get();

// Retourne l'adresse du pointeur de l'objet sous-jacent.
obj->GetAddressOf();

// Met à nullptr et décrémente le nombre de reference.
obj->Reset();
```

## *Swap Chain*
Pour éviter le *flickering* (scintillement) pour les animations, on *rend* une *frame* dans une texture hors écran qu'on appelle **back buffer**. Une fois que toute la scéne a été dessinée dans le tampon alors on peut l'afficher à l'écran. Pour implémenter cela, on utilise deux tampos (*buffer*), le **front buffer** et le **back buffer**. Le front buffer est celui qui est affiché à l'écran et le back buffer est celui qui est utilisé pour dessiner la prochaine frame. Après avoir dessiné dans le back buffer, on l'affiche à l'écran et on inverse les tampons. Ce processus est appelé *swap*. C'est ces deux *buffers* qui forment la **Swap Chain**. 

Elle est représentée par l'interface `IDXGISwapChain` qui stocke les tampons et fournit des méthodes comme `IDXGISwapChain::ResizeBuffers` et `IDXGISwapChain::Present`. 

## *Depth buffering*
Le tampon de profondeur (*depth buffer*) est une texture qui ne contient pas de données d'image mais qui stocke la profondeur de chaque pixel. Ses valeurs possibles sont entre 0.0 et 1.0, avec 0.0 étant le plus proche de la caméra et 1.0 étant le plus loin. Le tampon de profondeur est utilisé pour déterminer si un pixel doit être dessiné ou non. 

## Ressources et descripteurs
Pendant le rendu, le GPU va écrire vers des ressources et lire depuis des ressources. Avant de demander à dessiner, on doit d’abord lier les ressources à la pipeline de rendu. Cependant, les ressources GPU ne sont pas liées directement, on utilise des **descriptors** qui peut être vu comme des structures légères qui décrivent la ressource au GPU. Les descripteurs ont des *types* et ces types indiquent comment la ressource va être utilisée. Par exemple : 
- **CBV** / **SRV** / **UAV** désignent respectivement les *Constant Buffer View*, *Shader Resource View* et *Unordered Access View*.
- **Sampler** désigne un *sampler*/*échantillonneur* qui est utilisé pour échantillonner les textures.
- **RTV** désigne un *Render Target View* qui est utilisé pour les cibles de rendu.
- **DSV** désigne un *Depth Stencil View* qui est utilisé pour les tampons de profondeur/stencil.

Un *tas de descripteurs* est un tableau de descripteur : il s'agit de la mémoire qui stocke tous les descripteurs d'un type particulier. On a besoin d'un tas de descripteur pour chaque type de descripteur et on peut créer plusieurs tas de descripteurs pour un même type. On peut aussi avoir plusieurs descripteurs vers une même ressource. 

## File et liste de commande
Le GPU a une file de commande, le CPU peut soumettre des commandes à cette file via l'API Direct3D en utilisant des listes de commandes. Une fois qu'un ensemble de commandes est soumis à la file de commande, elles ne sont pas tout de suite exécutées, elles sont exécutées par le GPU quand il est prêt.

Le cas où la file est vide implique que le GPU n'a pas de travail à faire et le cas où la file est pleine implique que le CPU doit attendre que le GPU termine son travail avant de soumettre de nouvelles commandes. Ce sont deux situations que l'on veut éviter.

La file de commande (***Command Queue***) est représentée par l'interface `ID3D12CommandQueue`. On peut la créer en remplissant une structure `D3D12_COMMAND_QUEUE_DESC` qui décrit la file. On peut ensuite faire appel à la méthode `ID3D12Device::CreateCommandQueue` pour créer la file de commande.

Une des méthodes principales de la file de commande est `ExecuteCommandLists` qui permet d'ajouter des listes de commande à la file de commande : 
```cpp
void ID3D12CommandQueue::ExecuteCommandLists(
    // Nombre de listes de commande dans le tableau
    UINT Count, 
    // Pointeur vers le tableau de listes de commande
    ID3D12CommandList *const *ppCommandLists 
);
```

Par exemple une liste de commande pour le rendu est représentée par l'interface `ID3D12GraphicsCommandList` qui hérite de `ID3D12CommandList`. On peut ajouter des commandes comme :
```cpp
ComPtr<ID3D12GraphicsCommandList> mCommandList;

// Définir la vue de rendu
mCommandList->RSSetViewports(1, &mScreenViewport);
// Netoyer la cible de rendu
mCommandList->ClearRenderTargetView(mBackBufferView, DirectX::Colors::LightSteelBlue, 0, nullptr);
// Émettre un appel de dessin
mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
```

On peut créer une liste de commande avec : 
```cpp
HRESULT ID3D12Device::CreateCommandList(
    UINT nodeMask, 
    D3D12_COMMAND_LIST_TYPE type, 
    ID3D12CommandAllocator *pCommandAllocator, 
    ID3D12PipelineState *pInitialState, 
    REFIID riid, 
    void **ppCommandList
);
```
1. ***nodeMask*** : est défini à 0 pour les systèmes à un seul GPU, sinon il identifie le GPU physique sur lequel la liste de commande sera exécutée.
2. ***type*** : est le type de la liste de commande, par exemple `D3D12_COMMAND_LIST_TYPE_DIRECT` pour une liste de commande de rendu.
3. ***pCommandAllocator*** : est l'allocateur de commandes qui va stocker les commandes de la liste de commande. Attention, le type de l'allocateur de commandes doit correspondre au type de la liste de commande.
4. ***pInitialState*** : est l'état initial de la pipeline de rendu, on peut le laisser à `nullptr` si on n'en a pas besoin.
5. ***riid*** : est le *COM ID* de l'interface `ID3D12CommandList` que l'on veut créer.
6. ***ppCommandList*** : est un pointeur de sortie vers la liste de commande créée.

Quand on a fini de soumettre des commandes, on doit fermer la liste de commande avec `ID3D12GraphicsCommandList::Close`. La liste de commande doit être **fermée** avant de passer dans la méthode `ExecuteCommandLists` de la file de commande.

Associé à la liste de commande, on a un *allocateur de commandes* `ID3D12CommandAllocator`. Quand les commandes sont ajoutées à la liste de commande, elles sont stockées dans l'allocateur de commandes. Quand une liste de commande est exécutée, la file de commande va reférencée les commandes dans l'allocateur. On peut créer l'allocateur de commandes avec la méthode : 
```cpp
HRESULT ID3D12Device::CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE Type, 
    REFIID riid, 
    void **ppCommandAllocator
);
```
1. ***type*** : est le type de la liste de commande qui peut être associé à l'allocateur de commandes. Par exemple `D3D12_COMMAND_LIST_TYPE_DIRECT` pour une liste de commande de rendu (donc exécuté par le GPU) ou encore `D3D12_COMMAND_LIST_TYPE_BUNDLE`.
2. ***riid*** : est le *COM ID* de l'interface que l'on veut créer.
3. ***ppCommandAllocator*** : est un pointeur de sortie vers l'allocateur de commandes créé.

Après avoir faire appel à `ExecuteCommandLists`, on peut réutiliser la mémoire interne de la liste de commande en appelant la méthode : 
```cpp
HRESULT ID3D12CommandList::Reset(
    ID3D12CommandAllocator *pAllocator,
    ID3D12PipelineState *pInitialState
);
```

On peut faire la même avec l'allocateur de commandes : 
```cpp
HRESULT ID3D12CommandAllocator::Reset(void);
```
ATTENTION : **Comme la file d'attente de commandes peut faire référence à des données dans un allocateur, l'allocateur de commandes ne doit pas être réinitialisé tant qu'on est pas sur que le GPU a fini d'exécuter toutes les commandes**