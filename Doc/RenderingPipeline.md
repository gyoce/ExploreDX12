# La pipeline de rendu

[Page d'accueil](README.md)

Sommaire : 
- [Couleurs](#couleurs)
- [Vue d'ensemble de la pipeline de rendu](#vue-densemble-de-la-pipeline-de-rendu)
- [L'étape *Input Assembler*](#létape-input-assembler)
    - [Topologie](#topologie)
    - [Indices](#indices)

## Couleurs
Les écrans émettent une mixture de lumière rouge, vert et bleu pour chaque pixel. On utilise donc un modèle de couleur RGB (Red, Green, Blue) pour représenter les couleurs. Chaque écran possède une intensité maximale de lumière qu'il peut émettre, il est utile d'utiliser un intervalle normalisé de 0 à 1 pour ces intensités avec 0 représentant l'absence de lumière et 1 représentant l'intensité maximale. En plus du R,G,B on utilise aussi un canal alpha (A) pour représenter la transparence d'une couleur. On a donc la possibilité d'exprimer une couleur avec 128 bits (16 octets), on peut donc utiliser un `XMVECTOR`.

On peut aussi exprimer une couleur avec 32 bits (4 octets), un octet étant donné à chaque composante (R, G, B, A). Attention, ici chaque composante est un entier non signé donc sur l'intervalle [0, 255]. La bibliothèque `DirectX Math` nous fournit une structure `XMCOLOR` dans le namespace `DirectX::PackedVector` qui représente une couleur sur 32 bits. On peut convertir une couleur 32 bits vers une couleur 128 bits et inversement avec les fonctions :
```cpp
XMVECTOR XM_CALLCONV XMLoadColor(const XMCOLOR* pSource);
void XM_CALLCONV XMStoreColor(XMCOLOR* pDestination, FXMVECTOR V);
```

## Vue d'ensemble de la pipeline de rendu
La pipeline de rendu représente la séquence d'étapes nécessaire pour générer l'image 2D basée sur la vue de la caméra. À toutes les étapes sauf le *Tesselator Stage*, il y a une communication avec les ressources du GPU.
1) *Input Assembler Stage*
2) *Vertex Shader Stage*
3) *Hull Shader Stage*
4) *Tesselator Stage*
5) *Domain Shader Stage*
6) *Geometry Shader Stage*
7) *Rasterizer Stage*
8) *Pixel Shader Stage*
9) *Output Merger Stage*


## L'étape *Input Assembler*
Cette étape est responsable de lire les données géométriques (sommets et indices) à partir de la mémoire et les utilise pour assembler des primitives géométriques (triangles, lignes...). On peut se dire qu'un sommet n'est qu'un point spécial dans une primitive géométrique mais avec *Direct3D* il peut contenir d'autres données que seulement sa position comme par exemple des normales, des coordonnées de texture, etc.

Les sommets sont liés la pipeline de rendu dans une structure se nommant **Vertex Buffer**. C'est un tampon qui stocke une liste de sommets dans de la mémoire continue. Par contre, cela ne dit pas comment les sommets sont utilisés pour former des primitives. 

### Topologie 
On doit dire à *Direct3D* comment les sommets sont utilisés pour former des primitives en spécifiant la topologie de la primitive : 
```cpp
void ID3D12GraphicsCommandList::IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY Topology);

typedef enum D3D_PRIMITIVE_TOPOLOGY
{
    D3D_PRIMITIVE_TOPOLOGY_UNDEFINED = 0,
    D3D_PRIMITIVE_TOPOLOGY_POINTLIST = 1,
    D3D_PRIMITIVE_TOPOLOGY_LINELIST = 2,
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP = 3,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5,
    D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 10,
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 11,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 12,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 13,
    D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 33,
    D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 34,
    // ...
    D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64,
} D3D_PRIMITIVE_TOPOLOGY;
```
Tous les appels de dessin utilise la topologie précedemment spécifiée jusqu'à ce qu'une nouvelle topologie soit définie. 
```cpp
mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
// Dessine avec la topologie "Line list" ...
mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
// Dessine avec la topologie "Triangle list" ...
```
- **D3D_PRIMITIVE_TOPOLOGY_POINTLIST** : chaque sommet de la liste sera dessiné comme un point individuel.
- **D3D_PRIMITIVE_TOPOLOGY_LINESTRIP** : chaque sommet est lié au suivant pour former une ligne, on a donc `n + 1` sommets pour dessiner `n` segments de ligne.
- **D3D_PRIMITIVE_TOPOLOGY_LINELIST** : chaque couple de sommets consécutifs forme une ligne, on a donc `2n` sommets pour dessiner `n` segments de ligne.
- **D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP** : chaque triplet de sommets forme un triangle (0-1-2, 1-2-3, 2-3-4, ...)
- **D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST** : chaque groupe de trois sommets forme un triangle (0-1-2, 3-4-5, ...), on a donc `3n` sommets pour dessiner `n` triangles.
- **D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ** : pour chaque triangle, on inclut aussi ses trois triangles voisins nommés "triangles adjacents".

### Indices
On peut construire des primitives géométriques en utilisant des **indices**, admettons que l'on ait ce code qui nous permet de générer un quadrilatère et un octogone en utilisant la topologie *triangle list* : 
```cpp
Vertex quad[6] = {
    v0, v1, v2, // Triangle 0
    v0, v2, v3, // Triangle 1
};

Vertex octagon[24] = {
    v0, v1, v2, // Triangle 0
    v0, v2, v3, // Triangle 1
    v0, v3, v4, // Triangle 2
    v0, v4, v5, // Triangle 3
    v0, v5, v6, // Triangle 4
    v0, v6, v7, // Triangle 5
    v0, v7, v8, // Triangle 6
    v0, v8, v1, // Triangle 7
};
```
Attention, l'ordre dans lequel on spécifie les sommets d'un triangle est important et est appelé **winding order**. Comme on peut le constater, dans le quadrilatère, on a un sommet qui se répète (v0) et dans l'octogone beaucoup de sommets se repètent. Pour éviter cette répétition, on peut définir une liste d'indice : 
```cpp
Vertex v[4] = { v0, v1, v2, v3 };
UINT indexList[6] = { 
    0, 1, 2, // Triangle 0
    0, 2, 3, // Triangle 1
};
```