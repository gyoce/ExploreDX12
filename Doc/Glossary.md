# Glossaire

[Page d'accueil](README.md)

Sommaire : 
- [ID3DBlob](#id3dblob)
- [ID3D12DescriptorHeap](#id3d12descriptorheap)
- [ID3D12Resource](#id3d12resource)
- [Views / Descriptors](#views--descriptors)

## ID3DBlob
[Documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ff728743(v=vs.85))

Le type `ID3DBlob` est un simple bloc de mémoire générique qui a deux méthodes :
1. `LPVOID GetBufferPointer` : Retourne un `void*` vers les données, donc il doit être casté au type approprié avant utilisation.
2. `SIZE_T GetBufferSize` : Retourne la taille en octets du buffer.

## ID3D12DescriptorHeap
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12descriptorheap)

Un tas de descripteur est une sorte de tableau de descripteur. C'est de la mémoire qui stocke tous les descripteurs d'un type particulier (CBV, SRV, UAV ...).

## ID3D12Resource
[Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12resource)

Permet de représenter une ressource de Direct3D 12, c'est-à-dire un tampon de mémoire dans lequel on peut stocker des données comme des sommets, des indices ou autres.

## Views / Descriptors
[Documentation](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors) | [Explications](/Doc/Overview.md#ressources-et-descripteurs)

Une vue (*view*) ou descripteur (*descriptor*) est une structure qui décrit comment accéder à une ressource. On peut lier une ressource à la pipeline de rendu en spécifiant les descripteurs appropriés. 