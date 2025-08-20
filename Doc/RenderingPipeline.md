# La pipeline de rendu

[Page d'accueil](README.md)

Sommaire : 
- [Couleurs](#couleurs)
- [Vue d'ensemble de la pipeline de rendu](#vue-densemble-de-la-pipeline-de-rendu)
- [L'étape *Input Assembler*](#létape-input-assembler)
    - [Topologie](#topologie)
    - [Indices](#indices)
- [L'étape *Vertex Shader*](#létape-vertex-shader)

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

## L'étape *Vertex Shader*
Après que les primitives aient été assemblées, les sommets sont données au *Vertex Shader*. On peut voir ce dernier comme une fonction qui prend en entrée un sommet et qui produit en sortie un sommet transformé. 

### Local space vs World space
Généralement, on construit des objets dans un système de coordonnées local (*local space*), une fois ceci fait, on les place dans le monde. Pour cela, on doit définir comment l'espace local et l'espace du monde sont liés. Cela se fait en spécifiant ou on veut placer l'origine et les axes du système de coordonnées de l'espace local par rapport au système de coordonnées global de la scène puis en exécutant une transformation de coordonnées. Ce processus de transformation de coordonnées d'un espace local en un espace du monde est appelé **World Transformation** et la matrice associée est la **World Matrix**. Chaque objet dans la scène a sa propre matrice monde.

### View space
Pour créer une image 2D de la scène, on doit placer une caméra virutelle dans la scène. La caméra définit quel volume du monde le spectateur peut voir. Admettons que l'on attache un système de coordonnées local à la caméra. Celle-ci repose à l'origine et regarde vers le bas de l'axe *z* positif, l'axe *x* pointe vers la droite de la caméra et l'axe *y* pointe au dessus de la caméra. Au lieu de décrire les sommets de notre scène par rapport à l'espace monde, il est plus pratique pour les étapes suivantes de la pipeline de les décrire par rapport au système de coordonnées de la caméra. Le changement de transformation de coordonnées de l'espace monde vers l'espace de vue est appelé **View Transformation** et la matrice associée est la **View Matrix**.

### Projection et *Homogeneous Clip space*
On a décrit la position et l'orientation de la caméra dans le monde mais il y a un autre composant pour la caméra qui est le *volume* de l'espace que la caméra perçoit. Ce volume est décrit par un *frustum* (pyramide tronquée). Il faut donc projeter les géométrie 3D à l'intérieur de ce frustum vers une fenêtre de projection 2D. La projection doit être faite de sorte à ce que les lignes parallèles converge vers un point de fuite.

On peut définir un frustum dans l'espace de vue, avec le centre de projection à l'origine et orienté vers le bas de l'axe *z* positif. On définit quatre composantes : le plan de près (*near plane*) `n`, le plan de loin (*far plane*) `f`, l'angle de champs vertical (*vertical field of view angle*) `α`, et l'aspect ration `r`. Á noter que dans l'espace de vue, le *near plane* et le *far plane* sont parallèles au plan *xy*.

Les coordonnées des points projetés sont calculés dans l'espace de vue. Dans l'espace de vue, la fenêtre de projection a une hauteur de 2 et une largeur de `2*r` avec `r` l'aspect ratio. Le problème avec ça est que les dimensions dependent de l'aspect ratio, cela veut dire qu'on doit dire au GPU l'aspect ratio. Il est plus pratique d'enlever cette dépendance, on peut le faire en mettant à l'échelle la coordonnée *x* projetée de l'intervalle `[-r, +r]` à `[-1, +1]`. Après cette manipulation, les coordonnées *x* et *y* sont dites **Normalized Device Coordinates** (NDC).