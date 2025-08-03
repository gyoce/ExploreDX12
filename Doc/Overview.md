# Vue d'ensemble

[Page d'accueil](Main.md)

Sommaire :
- [COM](#com)
- [Ressources et descripteurs](#ressources-et-descripteurs)

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
Pendant le rendu, le GPU va écrire vers des ressources et lire depuis des ressources. Avant de demander à dessiner, on doit d’abord lier les ressources à la pipeline de rendu. Cependant, les ressources GPU ne sont pas liées directement, on utilise des **descriptors** qui peut être vu comme des structures légères qui décrivent la ressource au GPU.