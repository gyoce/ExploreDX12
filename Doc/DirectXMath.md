# DirectX Math

[Page d'accueil](README.md)

Sommaire : 
- [Généralités](#généralités)
- [Vecteurs : Types](#vecteurs--types)
- [Vecteurs : Méthodes de chargement et de stockage](#vecteurs--méthodes-de-chargement-et-de-stockage)
- [Vecteurs : Passage de paramètres](#vecteurs--passage-de-paramètres)
- [Vecteurs : Constants](#vecteurs--constants)
- [Vecteurs : Divers](#vecteurs--divers)
- [Vecteurs : Opérateurs surchargés](#vecteurs--opérateurs-surchargés)
- [Vecteurs : Fonctions de configuration](#vecteurs--fonctions-de-configuration)
- [Vecteurs : Fonctions utiles](#vecteurs--fonctions-utiles)

## Généralités
Pour Windows 8 et les versions ultérieures, *Direct X Math* est une bibliothèque de calcul mathématique qui fait partie du SDK de Windows. La bibliothèque utilise le *SSE2* (Streaming SIMD Extensions 2) pour effectuer des calculs vectoriels et matriciels de manière efficace. Avec des registres SIMD (*Single instruction multiple data*) de 128 bits, les instructions SIMD peuvent opérer sur quatre flottants/entiers avec une seule instruction. Par exemple le calcul de `u + v` avec *u* et *v* étant des vecteurs de quatre éléments peut être effectué en une seule instruction.

Pour pouvoir utiliser cette bibliothèque, il est nécessaire d'inclure `#include <DirectXMath.h>` dans le code source et pour certains types de données additionnels, il peut être nécessaire d'inclure `#include <DirectXPackedVector.h>`. Tout le code `DirecXMath` vie dans le namespace `DirectX`.

## Vecteurs : Types
Le type principal utilisé dans *DirectX Math* est `XMVECTOR`, qui est un type de données SIMD de 128 bits qui peut contenir quatre flottants. Il peut être défini comme `typedef __m128 XMVECTOR`. Quand on fait des calculs, les vecteurs doivent être de ce type pour profiter des optimisations SIMD. 

Les `XMVECTOR` doivent être alignés sur une frontière de 16 octets, cela est automatiquement fait pour les variables locales et globales. Pour les membres de classes, on utilisera plutôt `XMFLOAT2`, `XMFLOAT3` ou `XMFLOAT4`. Attention, si on utilise ces types pour le calcul on ne profitera pas des optimisations SIMD, il faut donc convertir les `XMFLOAT` en `XMVECTOR` avant de faire des calculs.

## Vecteurs : Méthodes de chargement et de stockage
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

## Vecteurs : Passage de paramètres
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

## Vecteurs : constants
Il est préférable pour les vecteurs `XMVECTOR` qui sont constants d'utiliser le type `XMVECTORF32`.
Exemple : 
```cpp
static const XMVECTORF32 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
static const XMVECTORF32 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };
```
Le `XMVECTOR32F` est une structure alignée sur 16 octets avec une conversion possible vers `XMVECTOR`.
On peut aussi créer un vecteur constant de valeur entières avec le type `XMVECTORU32`.

## Vecteurs : Opérateurs surchargés
Le type `XMVECTOR` a beaucoup d'opérateurs surchargés pour faciliter les opérations mathématiques. Par exemple : 
```cpp
XMVECTOR XM_CALLCONV operator+(FXMVECTOR V);
XMVECTOR XM_CALLCONV operator-(FXMVECTOR V);
XMVECTOR& XM_CALLCONV operator+=(XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR& XM_CALLCONV operator-=(XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR& XM_CALLCONV operator*=(XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR& XM_CALLCONV operator/=(XMVECTOR& V1, FXMVECTOR V2);
XMVECTOR& operator*=(XMVECTOR& V, float S);
XMVECTOR& operator/=(XMVECTOR& V, float S);
XMVECTOR XM_CALLCONV operator+(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR XM_CALLCONV operator-(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR XM_CALLCONV operator*(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR XM_CALLCONV operator/(FXMVECTOR V1, FXMVECTOR V2);
XMVECTOR XM_CALLCONV operator*(FXMVECTOR V, float S);
XMVECTOR XM_CALLCONV operator*(float S, FXMVECTOR V);
XMVECTOR XM_CALLCONV operator/(FXMVECTOR V, float S);
```

## Vecteurs : Divers
La bibliothèque `DirectX Math` nous fournit des valeurs constantes et des méthodes intéressantes comme : 
```cpp
const float XM_PI = ...;
const float XM_2PI = ...;
const float XM_1DIVPI = ...;
const float XM_1DIV2PI = ...;

inline float XMConvertToRadians(float fDegrees) { ... }
inline float XMConvertToDegrees(float fRadians) { ... }
inline T XMMin(T a, T b) { ... }
inline T XMMax(T a, T b) { ... }
```

## Vecteurs : fonctions de configuration
La bibliothèque nous fournit des fonctions pour configurer les vecteurs i.e. configurer le contenu des vecteurs comme : 
```cpp
// Returns the zero vector 0
XMVECTOR XM_CALLCONV XMVectorZero();

// Returns the vector (1, 1, 1, 1)
XMVECTOR XM_CALLCONV XMVectorSplatOne();

// Returns the vector (x, y, z, w)
XMVECTOR XM_CALLCONV XMVectorSet(float x, float y, float z, float w);

// Returns the vector (s, s, s, s)
XMVECTOR XM_CALLCONV XMVectorReplicate(float Value);

// Returns the vector (vx, vx, vx, vx)
XMVECTOR XM_CALLCONV XMVectorSplatX(FXMVECTOR V);

// Returns the vector (vy, vy, vy, vy)
XMVECTOR XM_CALLCONV XMVectorSplatY(FXMVECTOR V);

// Returns the vector (vz, vz, vz, vz)
XMVECTOR XM_CALLCONV XMVectorSplatZ(FXMVECTOR V);
```

## Vecteurs : fonctions utiles
`DirectX Math` nous fournit des fonctions qui permettent de faire des opérations particulières avec les vecteurs en 2D, 3D ou même 4D. Par exemple :
```cpp
// Returns ||v||
XMVECTOR XM_CALLCONV XMVector3Length(FXMVECTOR V);

// Returns ||v||²
XMVECTOR XM_CALLCONV XMVector3LengthSq(FXMVECTOR V);

// Returns v1.v2
XMVECTOR XM_CALLCONV XMVector3Dot(FXMVECTOR V1, FXMVECTOR V2);

// Returns v1 × v2
XMVECTOR XM_CALLCONV XMVector3Cross(FXMVECTOR V1, FXMVECTOR V2);

// Returns v / ||v||
XMVECTOR XM_CALLCONV XMVector3Normalize(FXMVECTOR V);

// Returns a vector orthogonal to v
XMVECTOR XM_CALLCONV XMVector3Orthogonal(FXMVECTOR V);

// Returns the angle between v1 and v2
XMVECTOR XM_CALLCONV XMVector3AngleBetweenVectors(FXMVECTOR V1, FXMVECTOR V2);

// Returns v1 = v2
bool XM_CALLCONV XMVector3Equal(FXMVECTOR V1, FXMVECTOR V2);

// Returns v1 != v2
bool XM_CALLCONV XMVector3NotEqual(FXMVECTOR V1, FXMVECTOR V2);

// Returns estimated ||v|| (faster)
XMVECTOR XM_CALLCONV XMVector3LengthEst(FXMVECTOR V);

// Returns estimated v / ||v|| (faster)
XMVECTOR XM_CALLCONV XMVector3NormalizeEst(FXMVECTOR V);

// Returns abs(U.x – V.x) <= Epsilon.x && abs(U.y – V.y) <= Epsilon.y && abs(U.z – V.z) <= Epsilon.z
bool XM_CALLCONV XMVector3NearEqual(FXMVECTOR U, FXMVECTOR V, FXMVECTOR Epsilon);
```
Ici on peut observer que de manière générale, ces fonctions retournent un `XMVECTOR` même pour les opérations qui retournent un scalaire. Le résultat est alors répliqué dans chaque composant du `XMVECTOR`.
