# Glossaire

[Page d'accueil](README.md)

Sommaire : 
- [D3D12_CPU_DESCRIPTOR_HANDLE](#d3d12_cpu_descriptor_handle)
- [D3D12_GPU_DESCRIPTOR_HANDLE](#d3d12_gpu_descriptor_handle)
- [D3D12_RESOURCE_BARRIER](#d3d12_resource_barrier)
- [D3D12_ROOT_PARAMETER](#d3d12_root_parameter)
- [ID3DBlob](#id3dblob)
- [ID3D12CommandAllocator](#id3d12commandallocator)
- [ID3D12CommandList](#id3d12commandlist)
- [ID3D12CommandQueue](#id3d12commandqueue)
- [ID3D12DescriptorHeap](#id3d12descriptorheap)
- [ID3D12Device](#id3d12device)
- [ID3D12Fence](#id3d12fence)
- [ID3D12Resource](#id3d12resource)
- [ID3D12RootSignature](#id3d12rootsignature)
- [IDXGISwapChain](#idxgiswapchain)
- [Input layout](#input-layout)
- [Pipeline State Object (PSO)](#pipeline-state-object-pso)
- [Views / Descriptors](#views--descriptors)

## D3D12_CPU_DESCRIPTOR_HANDLE
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_cpu_descriptor_handle)

Un descripteur CPU est une structure qui représente un pointeur vers un descripteur dans un tas de descripteurs (*descriptor heap*). Il est utilisé par le CPU pour accéder et manipuler les descripteurs. On peut obtenir le *handle* du premier descripteur dans un tas de descripteurs avec la méthode `ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart()` ou à une certaine position avec la fonction `CD3DX12_CPU_DESCRIPTOR_HANDLE::Offset()`.

## D3D12_GPU_DESCRIPTOR_HANDLE
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_gpu_descriptor_handle)

Un descripteur GPU est une structure qui représente un pointeur vers un descripteur dans un tas de descripteurs (*descriptor heap*). Il est utilisé par le GPU pour accéder aux descripteurs lors de l'exécution des shaders. On peut obtenir le *handle* du premier descripteur dans un tas de descripteurs avec la méthode `ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart()` ou à une certaine position avec la fonction `CD3DX12_GPU_DESCRIPTOR_HANDLE::Offset()`.

## D3D12_INPUT_ELEMENT_DESC
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc) | [Explications](/Doc/DirectXDrawing.md#sommets-et-input-layouts)

Un élément d'entrée (*input element*) est une structure qui décrit un composant individuel d'un sommet dans un vertex buffer. Chaque élément d'entrée spécifie le nom du composant, son format de données, son emplacement dans la structure de sommet, etc. Un tableau de `D3D12_INPUT_ELEMENT_DESC` est utilisé pour définir un *input layout* lors de la création du Pipeline State Object (PSO).

## D3D12_RESOURCE_BARRIER
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_barrier) | [Explications](/Doc/Overview.md#resource-transitions)

Une barrière de ressource (*resource barrier*) est une structure qui décrit une transition d'état pour une ressource. Elle est utilisée pour informer le GPU qu'une ressource va changer d'utilisation, par exemple passer d'une utilisation de rendu à une utilisation de lecture par un shader. 

## D3D12_ROOT_PARAMETER
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_root_parameter)

Un paramètre racine (*root parameter*) est une structure qui fait partie d'une *root signature*. Il définit comment une ressource est liée à la pipeline de rendu. Un paramètre racine peut être un descripteur unique, une table de descripteurs ou un constant buffer.

## ID3DBlob
[Documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ff728743(v=vs.85))

Le type `ID3DBlob` est un simple bloc de mémoire générique qui a deux méthodes :
1. `LPVOID GetBufferPointer` : Retourne un `void*` vers les données, donc il doit être casté au type approprié avant utilisation.
2. `SIZE_T GetBufferSize` : Retourne la taille en octets du buffer.

## ID3D12CommandAllocator
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandallocator) | [Explications](/Doc/Overview.md#file-et-liste-de-commande)

Un allocateur de commandes (*command allocator*) est responsable de la gestion de la mémoire pour l'enregistrement des commandes dans une liste de commandes (*command list*). Quand les commandes sont enregistrées dans une *command list*, elles sont stockées dans un *command allocator*. 

## ID3D12CommandList
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandlist) | [Explications](/Doc/Overview.md#file-et-liste-de-commande)

Une liste de commandes (*command list*) est une séquence d'instructions que le GPU doit exécuter. Le CPU enregistre les commandes dans une *command list* comme dessiner des objets, copier des données entre des ressources, etc. La *command list* `ID3D12GraphicsCommandList` hérite de `ID3D12CommandList` et permet d'enregistrer des commandes graphiques comment la définition du viewport, le nettoyage de la cible de rendu, le dessin d'objets, etc.

## ID3D12CommandQueue
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandqueue) | [Explications](/Doc/Overview.md#file-et-liste-de-commande)

Une file de commandes (*command queue*) est une file d'attente qui stocke les listes de commandes (*command lists*) avant qu'elles soient exécutées par le GPU. C'est le CPU qui soumet les *command lists* à la *command queue* avec la méthode `ID3D12CommandQueue::ExecuteCommandLists(UINT count, ID3D12CommandList* const *ppCommandLists)`.

## ID3D12DescriptorHeap
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12descriptorheap)

Un tas de descripteur est une sorte de tableau de descripteur. C'est de la mémoire qui stocke tous les descripteurs d'un type particulier (CBV, SRV, UAV ...).

## ID3D12Device
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12device)

Un périphérique (*device*) représente l'adaptateur graphique qui est généralement un composant matériel qui permet de faire de la 3D. Dans DirectX, le *device* est utilisé pour vérifier la disponibilité des fonctionnalités et créer des objets comme des ressources, des vues et des listes de commandes.

## ID3D12Fence
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12fence) | [Explications](/Doc/Overview.md#cpugpu-synchronisation)

Une barrière (*fence*) est utilisé pour synchroniser le GPU et le CPU. La *fence* stocke une valeur `UINT64` qui est un entier qui permet d'identifier un instant dans le temps. On commence avec la valeur 0 et on l'incrémente à chaque fois qu'on veut marquer un instant.

## ID3D12Resource
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12resource)

Permet de représenter une ressource de Direct3D 12, c'est-à-dire un tampon de mémoire dans lequel on peut stocker des données comme des sommets, des indices ou autres.

## ID3D12RootSignature
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12rootsignature) | [Explications](/Doc/DirectXDrawing.md#root-signature)

Une *root signature* est une structure qui définit les ressources que les shaders peuvent utiliser. Elle spécifie comment les ressources sont liées à la pipeline de rendu, y compris les descripteurs, les tampons de constantes et les tables de descripteurs. La *root signature* est créée avant la création du Pipeline State Object (PSO) et doit être compatible avec les shaders utilisés dans le PSO.

## IDXGISwapChain
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nn-dxgi-idxgiswapchain) | [Explications](/Doc/Overview.md#swap-chain)

Une chaîne d'échange (*swap chain*) est une série de tampons d'images utilisés pour afficher des images à l'écran. Généralement on parle de *front buffer* et de *back buffer* (qui ne sont que des textures). Elle nous fournit des méthodes pour redimensionner les buffers ou pour "présenter" i.e. afficher le buffer à l'écran. 

## Input layout
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc) | [Explications](/Doc/DirectXDrawing.md#sommets-et-input-layouts)

Un *input layout* est une structure qui décrit la disposition des données de sommet dans un vertex buffer. On définit une structure pour nos sommets puis on crée un tableau de `D3D12_INPUT_ELEMENT_DESC` qui décrit chaque composant de la structure. Ce tableau sera utilisé lors de la création du PSO (Pipeline State Object) pour informer le GPU de la manière dont les données de sommet sont organisées.

## Pipeline State Object (PSO)
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc) | [Explications](/Doc/DirectXDrawing.md#pipeline-state-object)

Un Pipeline State Object (PSO) est une structure qui encapsule tous les états de la pipeline graphique. Il inclut des informations sur les shaders, le format des rendus, les états de rasterization, les états de profondeur/stencil, etc. Le PSO est créé une fois et peut être réutilisé pour dessiner plusieurs objets. On a alors un objet de type `ID3D12PipelineState` qui est créé avec la méthode `ID3D12Device::CreateGraphicsPipelineState()` et qui sera utilisé lors du rendu avec la méthode `CommandList::Reset(pCommandAllocator, pPipelineState)`.

## Views / Descriptors
[Documentation](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors) | [Explications](/Doc/Overview.md#ressources-et-descripteurs)

Une vue (*view*) ou descripteur (*descriptor*) est une structure qui décrit comment accéder à une ressource. On peut lier une ressource à la pipeline de rendu en spécifiant les descripteurs appropriés. Ils peuvent être de différents types selon la ressource qu'ils représentent, par exemple :
- CBV (Constant Buffer View) : Pour les tampons de constantes.
- SRV (Shader Resource View) : Pour les ressources accessibles en lecture par les shaders.
- UAV (Unordered Access View) : Pour les ressources accessibles en lecture/écriture par les shaders.
- RTV (Render Target View) : Pour les cibles de rendu.
- DSV (Depth Stencil View) : Pour les tampons de profondeur/stencil.