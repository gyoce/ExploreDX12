# Vue d'ensemble

[Page d'accueil](Main.md)

Sommaire :
- [COM](#com)
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

Un des méthodes principales de la file de commande est `ExecuteCommandLists` qui permet d'ajouter des listes de commande à la file de commande : 
```cpp
void ID3D12CommandQueue::ExecuteCommandLists(
    // Number of commands lists in the array
    UINT Count, 
    // Pointer to the first element in an array of command lists
    ID3D12CommandList *const *ppCommandLists 
);
```

