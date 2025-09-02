# Vue d'ensemble

[Page d'accueil](README.md)

Sommaire :
- [COM](#com)
- [Swap Chain](#swap-chain)
- [Depth buffering](#depth-buffering)
- [Ressources et descripteurs](#ressources-et-descripteurs)
- [File et liste de commande](#file-et-liste-de-commande)
- [CPU/GPU Synchronisation](#cpugpu-synchronisation)
- [Resource Transitions](#resource-transitions)


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

## CPU/GPU Synchronisation 
Si on suppose qu'on a une ressource *R* qui stocke la position d'une primitive que l'on veut dessiner. De plus, supposons que le CPU mette à jour les données de *R* pour y stocker des positions *p1* et ensuite ajoute une commande de dessin *C* qui référence *R* à la file de commande dans l'intention de dessiner la primitive avec les positions *p1*. L'ajout de commande à la file de commande ne block pas le CPU donc le CPU continue d'exécuter le reste des instructions. Ce serait une erreur pour le CPU de continuer d'exécuter la suite et d'écraser les données de *R* avec des positions *p2* avant que le GPU n'ait exécuté la commande *C*.

Une solution possible à cette situation est de forcer le CPU à attendre que le GPU ait finit de traiter les commandes dans la queue jusqu'à un certain point de barrière. On appel ça *Flushing the command queue* à savoir vider la file d'attente de commande. On peut le faire grâce à une barrirère (*fence*) qui est représenté par l'interface `ID3D12Fence` et qui est utilisé pour synchroniser le GPU et le CPU. 

L'objet barrière conserve une valeur `UINT64` qui est juste un entier pour identifier un point de barrière dans le temps. On commence à 0 et à chaque fois que l'on veut marquer un nouveau point on incrémente cet entier.
```c++
UINT64 CurrentFence = 0;

void FlushCommandQueue()
{
    // Permet "d'avancer" la barrière pour marquer les commandes jusqu'à ce point.
    CurrentFence++;

    // Ajoute une instruction à la commande queue qui permet de définir un nouveau "marquage" de barrière.
    // Permet de dire au GPU de mettre à jour la valeur de la barrière à "CurrentFence".
    // Ne sera pas fait tant que le GPU n'aura pas fini de traiter toutes les commandes précédente à ce "Signal"
    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), CurrentFence));

    // Attendre jusqu'à ce que le GPU ait complété les commandes jusqu'à ce point de barrière.
    if (Fence->GetCompletedValue() < CurrentFence)
    {
        // Création d'un event handle.
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        // Permet d'associer l'événement quand le GPU a atteint la valeur de fence à ce handle.
        ThrowIfFailed(Fence->SetEventOnCompletion(CurrentFence, eventHandle));
        // Permet de bloquer complétement le thread CPU jusqu'à ce que l'event soit signalé
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}
```

## Resource Transitions
Pour implémenter des effets de rendu, il est commun pour le GPU d'écrire dans une ressource *R* à une étape et à une étape d'après de lire depuis cette ressource *R*. Cela poserait des problèmes de lire depuis une certaine ressource si le GPU n'a pas fini d'y écrire. Pour résoudre ce problème, *Direct3D* associe un état aux ressources. Les ressources sont dans un état par défaut à leur création et c'est à l'application de dire à *Direct3D* qu'il y a une transition d'état.

Par exemple, si l'on écrit dans une ressource (admettons une texture), on définit alors l'état de la texture à l'état de cible de rendu. En informant *Direct3D* de cette transition, le GPU peut prendre des mesures pour éviter les dangers avec par exemple l'attente de toutes les opérations d'écriture avant de lire depuis la ressource.

Une transition de ressource est spécifiée par la définition d'un tableau de *transition resource barriers* sur la liste de commandes. Une barrière de ressource est représentée par la structure `D3D12_RESOURCE_BARRIER_DESC` mais on se sert principalement du wrapper : 
```c++
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
    // ...

    static inline CD3DX12_RESOURCE_BARRIER Transition(
        _In_ ID3D12Resource* pResource, 
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE
    )
    {
        CD3DX12_RESOURCE_BARRIER result;
        ZeroMemory(&result, sizeof(result));
        D3D12_RESOURCE_BARRIER &barrier = result;
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        result.Flags = flags;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return result;
    }

    // ...
};

// Exemple : permet de dire à *Direct3D* que l'on veut que la texture qui représente l'image qu'on affiche à l'écran passe de l'état de présentation à l'état de cible de rendu.
CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
    CurrentBackBuffer(),
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_RENDER_TARGET
);
mCommandList->ResourceBarrier(1, &resourceBarrier);
```