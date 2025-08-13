# DirectX Math

[Page d'accueil](README.md)

Sommaire : 
- [Généralités](#généralités)
- [Types de vecteur](#types-de-vecteur)
- [Méthodes de chargement et de stockage](#méthodes-de-chargement-et-de-stockage)
- [Passage de paramètre](#passage-de-paramètre)

## Généralités
Pour Windows 8 et les versions ultérieures, *Direct X Math* est une bibliothèque de calcul mathématique qui fait partie du SDK de Windows. La bibliothèque utilise le *SSE2* (Streaming SIMD Extensions 2) pour effectuer des calculs vectoriels et matriciels de manière efficace. Avec des registres SIMD (*Single instruction multiple data*) de 128 bits, les instructions SIMD peuvent opérer sur quatre flottants/entiers avec une seule instruction. Par exemple le calcul de `u + v` avec *u* et *v* étant des vecteurs de quatre éléments peut être effectué en une seule instruction.

Pour pouvoir utiliser cette bibliothèque, il est nécessaire d'inclure `#include <DirectXMath.h>` dans le code source et pour certains types de données additionnels, il peut être nécessaire d'inclure `#include <DirectXPackedVector.h>`. Tout le code `DirecXMath` vie dans le namespace `DirectX`.

## Types de vecteur
Le type principal utilisé dans *DirectX Math* est `XMVECTOR`, qui est un type de données SIMD de 128 bits qui peut contenir quatre flottants. Il peut être défini comme `typedef __m128 XMVECTOR`. Quand on fait des calculs, les vecteurs doivent être de ce type pour profiter des optimisations SIMD. 

Les `XMVECTOR` doivent être alignés sur une frontière de 16 octets, cela est automatiquement fait pour les variables locales et globales. Pour les membres de classes, on utilisera plutôt `XMFLOAT2`, `XMFLOAT3` ou `XMFLOAT4`. Attention, si on utilise ces types pour le calcul on ne profitera pas des optimisations SIMD, il faut donc convertir les `XMFLOAT` en `XMVECTOR` avant de faire des calculs.

## Méthodes de chargement et de stockage
Pour charger un vecteur `XMFLOATn` dans un `XMVECTOR` on utilise : 
```cpp
XMVECTOR XM_CALLCONV XMLoadFloat2(const XMFLOAT2* pSource);
XMVECTOR XM_CALLCONV XMLoadFloat3(const XMFLOAT3* pSource);
XMVECTOR XM_CALLCONV XMLoadFloat4(const XMFLOAT4* pSource);
```

Pour stocker un `XMVECTOR` dans un `XMFLOATn`, on utilise : 
```cpp
void XM_CALLCONV XMLoadFloat2(XMFLOAT2* pDestination, XMVECTOR V);
void XM_CALLCONV XMLoadFloat3(XMFLOAT3* pDestination, XMVECTOR V);
void XM_CALLCONV XMLoadFloat4(XMFLOAT4* pDestination, XMVECTOR V);
```

Parfois on veut juste *get* ou *set* une seule composante d'un vecteur, on peut alors utiliser : 
```cpp
float XM_CALLCONV XMVectorGetX(XMVECTOR V);
float XM_CALLCONV XMVectorGetY(XMVECTOR V);
float XM_CALLCONV XMVectorGetZ(XMVECTOR V);
float XM_CALLCONV XMVectorGetW(XMVECTOR V);

XMVECTOR XM_CALLCONV XMVectorSetX(XMVECTOR V, float X);
XMVECTOR XM_CALLCONV XMVectorSetY(XMVECTOR V, float Y);
XMVECTOR XM_CALLCONV XMVectorSetZ(XMVECTOR V, float Z);
XMVECTOR XM_CALLCONV XMVectorSetW(XMVECTOR V, float W);
```

## Passage de paramètres
Pour des raisons d'efficacité, les `XMVECTOR` peuvent être passés par argument de fonction dans des registres *SSE*/*SSE2* à la place de la pile. Le nombre d'argument qui peuvent être passés de cette manière dépend de l'architecture. Donc pour être indépendant du compilateur et/ou de la plateforme, on utilisera les types `FXMVECTOR`, `GXMVECTOR`, `HXMVECTOR`et `CXMVECTOR` pour passer les vecteurs. 
De plus, on doit avoir l'annotation `XM_CALLCONV` de spécifiée avant le nom de la fonction.

On utilise les règles suivantes pour passer des `XMVECTOR` en paramètres : 
1. Les trois premiers doivent être de type `FXMVECTOR`
2. Le quatrième doit être de type `GXMVECTOR`
3. Le cinquième et sixième doivent être de type `HXMVECTOR`.
4. Tous les suivants doivent être de type `CXMVECTOR`.
Exemple : 
```cpp
inline XMMATRIX XM_CALLCONV XMMatrixTransformation(
    FXMVECTOR ScalingOrigin,
    FXMVECTOR ScalingOrientationQuaternion,
    FXMVECTOR Scaling,
    GXMVECTOR RotationOrigin,
    HXMVECTOR RotationQuaternion,
    HXMVECTOR Translation
);
```

On peut avoir des paramètres qui ne sont pas de `XMVECTOR`, les mêmes règles s'appliquent et on fait comme si les autres paramètres n'existaient pas. 
Exemple : 
```cpp
inline XMMATRIX XM_CALLCONV XMMatrixTransformation2D(
    FXMVECTOR ScalingOrigin,
    float ScalingOrientation,
    FXMVECTOR Scaling,
    FXMVECTOR RotationOrigin,
    float Rotation,
    GXMVECTOR Translation
);
```
Il faut aussi savoir que les règles du passage de paramètre ne s'appliquent qu'au paramètre *entrants*, pour les paramètres *sortants* (`XMVECTOR&` ou `XMVECTOR*`), cela n'utilisera pas les registres SSE/SSE2 donc ils seront traités comme des paramètres normaux.

