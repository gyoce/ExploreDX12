# Dessiner avec DirectX

[Page d'accueil](README.md)

Sommaire : 
- [Sommets et input layouts](#sommets-et-input-layouts)

## Sommets et input layouts
Un sommet avec *Direct3D* peut être défini par un ensemble de données en plus de ses coordonnées 3D. Pour créer un format de sommet personnalisé, il faut d'abord définir une structure qui permet de décrire les données que l'on veut. Exemple : 
```c++
struct Vertex1
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct Vertex2 
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
    XMFLOAT2 Tex0;
    XMFLOAT2 Tex1;
}
```
Une fois qu'on a défini notre structure de sommet, on doit en informer *Direct3D* avec une description pour savoir quoi faire avec chaque composant de la structure. Cette description est fournie à *Direct3D* avec la structure : 
```c++
typedef struct D3D12_INPUT_LAYOUT_DESC
{
    const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
    UINT NumElements;
} D3D12_INPUT_LAYOUT_DESC;
// On peut voir cette structure comme un tableau de `D3D12_INPUT_ELEMENT_DESC`
```
Chaque élément de la struture `D3D12_INPUT_LAYOUT_DESC` décrit et correspond à un composant de notre structure de sommet. 
```c++
typedef struct D3D12_INPUT_ELEMENT_DESC
{
    LPCSTR SemanticName;
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D12_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;
} D3D12_INPUT_ELEMENT_DESC;
```
1) `SemanticName` est un string associé avec l'élément. C'est utilisé pour mapper les éléments de la structure de sommet aux éléments de la signature d'entrée du vertex shader.
2) `SemanticIndex`est un index à associer à une sémantique. Par exemple pour une structure de sommet avec laquelle on a plus qu'un ensemble de coordonnées de texture, on peut préciser `0` pour le premier ensemble et `1` pour le second afin de bien les distinguer. 
3) `Format` est un membre de l'énumération `DXGI_FORMAT` qui spécifie le format (type de données). Par exemple : 
    - `DXGI_FORMAT_R32_FLOAT` : 1D 32-bit float scalar
    - `DXGI_FORMAT_R32G32_FLOAT` : 2D 32-bit float vector
    - `DXGI_FORMAT_R32G32B32_FLOAT` : 3D 32-bit float vector
    - `DXGI_FORMAT_R32G32B32A32_FLOAT` : 4D 32-bit float vector
    - `DXGI_FORMAT_R8_UINT` : 1D 8-bit unsigned integer scalar
    - `DXGI_FORMAT_R16G16_SINT` : 2D 16-bit signed integer vector
4) `InputSlot` spécifie l'index du slot d'entrée de cet élément. *Direct3D* supporte jusqu'à 16 slots d'entrée (d'indice 0-15) avec lesquels on peut nourir le vertex shader.
5) `AlignedByteOffset` est l'offset (le décalage) en octet depuis le début de la structure C++ de l'élément de la structure. Par exemple pour la structure `Vertex2`, l'élément *Pos* a un offset de 0 et l'élément *Normal* a un offset de 12 car on skip les octets de *Pos*.
6) `InputSlotClass` pour le moment on y spécifie `D3D12_INPUT_PER_VERTEX_DATA`.
7) `InstanceDataStepRate` pour le moment on y spécifie 0.
Voici un exemple pour la structure `Vertex2` définie plus haut : 
```c++
D3D12_INPUT_ELEMENT_DESC desc2[] = 
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_PER_VERTEX_DATA, 0},
}
```