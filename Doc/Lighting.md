# Lighting / Éclairage

[Page d'accueil](README.md)

Sommaire : 
- [Interaction entre la lumière et les matériaux](#interaction-entre-la-lumière-et-les-matériaux)
- [Vecteurs normaux](#vecteurs-normaux)
    - [Calcul des vecteurs normaux](#calcul-des-vecteurs-normaux)
    - [Transformation des vecteurs normaux](#transformation-des-vecteurs-normaux)
- [Vecteurs importants pour l'éclairage](#vecteurs-importants-pour-léclairage)
- [La loi des cosinus de Lambert](#la-loi-des-cosinus-de-lambert)

## Interaction entre la lumière et les matériaux
Quand on utilise de l'éclairage, on ne précise plus la couleur de chaque sommet directement mais on précise des matériaux et des lumières puis on applique une formule pour calculer la couleur finale de chaque pixel en fonction de la lumière et du matériau. Les matériaux peuvent être vu comme des propriétés qui définissent comment la lumière intéragit avec la surface d'un objet. Par exemple, la couleur de la lumière que la surface réfléchit et absorbe, l'indice de refraction du matériau sous la surface, combien la surface est lisse ou combien la surface est transparente.

Il existe plusieurs modèles d'éclairage, parmi les plus connus on trouve le modèle d'illumination locale. Avec ce modèle, chaque objet est éclairé indépendamment des autres objets et seulement la lumière directement émise depuis les sources de lumières est prise en compte dans le processus d'éclairage. Une autre modèle connu est le modèle d'illumination globale qui éclaire les objets en prénant en compte la lumière des sources de lumière mais aussi la lumière indirecte qui est réfléchie par d'autres objets dans la scène. 

## Vecteurs normaux
Une face normale est un vecteur unitaire qui décrit la direction vers laquelle un polygone est orienté. Une surface normale est un vecteur unitaire orthogonal au plan tangent à un point de la surface.

![Face et surface normales](/Doc/Imgs/FaceSurfaceNormal.png)

Pour les calculs d'éclairage, on a besoin de connaître la surface normale en chaque point sur la surface d'un maillage triangulaire ce qui nous permet de déterminer l'angle à laquelle la lumière frappe la surface. Pour obtenir les normales de surface on spécifie les normales de surface pour chaque sommet (*vertex normals*), puis on interpole les normales de surface pour chaque pixel à l'intérieur du triangle.

### Calcul des vecteurs normaux
Pour obtenir la face normale d'un triangle $`p_0`$, $`p_1`$, $`p_2`$, on a d'abord besoin de calculer deux vecteurs se trouvant sur les côtés du triangle $`u = p_1 - p_0`$ et $`v = p_2 - p_0`$. Ensuite, on peut calculer la face normale grâce à $`n = \frac{u \times v}{\|u \times v\|}`$. Voici un exemple de code qui permet de calculer la face normale d'un triangle :
```c++
XMVECTOR ComputeNormal(FXMVECTOR p0, FXMVECTOR p1, FXMVECTOR p2)
{
    XMVECTOR u = p1 - p0;
    XMVECTOR v = p2 - p0;
    return XMVECTOR3Normalize(XMVector3Cross(u, v));
}
```
Pour une surface dérivable, on peut utiliser le calcul pour trouver des normales des points sur la surface mais malheureusement les maillages triangulaires ne sont pas dérivables. La technique qui est généralement utilisée pour les maillages triangulaires est appelée *vertex normal averaging* (moyennage des normales de sommet). La normale au sommet $`n`$ ou à un sommet arbitraire $`v`$ dans un maillage est obtenue en calculant la moyenne des normales aux faces de chaque polygone dans le maillage qui partagent le sommet $`v`$, la formule est donc quelque chose comme : $`\frac{n_0 + n_1 + ...}{| n_0 + n_1 + ... |}`$. Voici un exemple en pseudo code : 
```c++
// Entrée : 1. tableau de sommets (mVertices) 2. tableau d'indices (mIndices)

// Pour chaque triangle dans le maillage : 
for (UINT i = 0; i < mNumTriangles; i++)
{
    // Les indices du ième triangle
    UINT i0 = mIndices[i*3+0];
    UINT i1 = mIndices[i*3+1];
    UINT i2 = mIndices[i*3+2];
    // Les sommets du ième triangle
    Vertex v0 = mVertices[i0];
    Vertex v1 = mVertices[i1];
    Vertex v2 = mVertices[i2];
    // Calcul de la face normale
    Vector3 e0 = v1.pos - v0.pos;
    Vector3 e1 = v2.pos - v0.pos;
    Vector3 faceNormal = Cross(e0, e1);
    // Ce triangle partage les trois sommets suivants, donc on ajoute cette face normale dans la moyenne de ces normales de sommet.
    mVertices[i0].normal += faceNormal;
    mVertices[i1].normal += faceNormal;
    mVertices[i2].normal += faceNormal;
}

// Pour chaque sommet v, on a additionné les normales de face de tous les triangles qui partagent v, donc maintenant on doit juste normaliser.
for(UINT i = 0; i < mNumVertices; ++i)
    mVertices[i].normal = Normalize(&mVertices[i].normal);
```
### Transformation des vecteurs normaux
Admettons que l'on ait un vecteur tangent $`u = v_1 - v_0`$ orthogonal au vecteur normal $`n`$.Si on applique une transformation de mise à l'échelle non uniforme $`A`$, on peut alors voir que le vecteur tangent transformé n'est plus orthogonal au vecteur normal transformé. Notre problème est donc le suivant : étant donné une matrice de transformation $`A`$ qui transforme les points et vecteurs (non normaux), comment trouver une matrice de transformation $`B`$ qui transforme les vecteurs normaux de telle sorte que l'orthogonalité soit préservée ? La réponse est que $`B = (A^{-1})^T`$ (la matrice inverse transposée de $`A`$). Voici un exemple de code qui permet de calculer la matrice inverse transposée : 
```c++
static XMMATRIX InverseTranspose(CXMMATRIX M)
{
    XMMATRIX A = M;
    A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR det = XMMatrixDeterminant(A);
    return XMMatrixTranspose(XMMatrixInverse(&det, A));
}
```
Ici on se débarrasse de la partie translation de la matrice de transformation en mettant la dernière ligne à $`(0,0,0,1)`$ parce que on utilise la transposée inverse pour transformer les vecteurs et que la translation s'applique uniquement sur les points.

## Vecteurs importants pour l'éclairage
![Vecteurs importants pour l'éclairage](/Doc/Imgs/LightingVectors.png)

Soit $`E`$ la position de l'oeil, $`p`$ le point que l'oeil voit le long de la ligne de visée définie par le vecteur unitaire $`v`$. Au point $`p`$, la surface a une normale $`n`$ et le point est frappé par un rayon lumineux se déplaçant dans la direction incidente $`I`$. 

Le vecteur lumière $`L`$  est le vecteur unitaire qui cible la direction opposée du rayon lumineux frappant le point de surface. Si on calcule la loi des cosinus de Lambert (*lambert's Cosine Law*), le vecteur $`L`$ est utilisé pour calculer $`L \cdot n = cos(\theta_i)`$ avec $`\theta_i`$ qui est l'angle entre $`L`$ et $`n`$. 

Le vecteur de réflexion $`r`$ est la réflexion du vecteur de lumière incidente par rapport à la normale de surface $`n`$. Le vecteur de vue (*view vector* / *to-eye vector*) $`v = \text{normalize}(E - p)`$ est le vecteur unitaire du point de la surface $`p`$ vers l'oeil $`E`$. 

Le vecteur de réflexion est donné par $`r = I - 2(I \cdot n)n`$, on assume ici que $`n`$ est un vecteur unitaire. Cependant, on peut se servir de la fonction HLSL `reflect(I, n)` pour calculer le vecteur de réflexion (qui nous donne donc $`r`$).

## La loi des cosinus de Lambert
On peut voir la lumière comme une collection de photons voyageant à travers l'espace dans une certaine direction. Chaque photon transporte une certaine quantité d'énergie lumineuse. La quantité d'énergie (lumineuse) émise par seconde est appelé flux énergétique (*radiant flux*). La densité de flux énergétique par unité de surface (appelé irradiance) est importante parce qu'elle va déterminer la quantité d'énergie lumineuse qui frappe une surface. 

La lumière qui frappe une surface de plein fouet (i.e. $`L`$ est égal au vecteur normal $`n`$) est plus intense que la lumière qui effleure une surface avec un angle. Si l'on considère un petit faisceau lumineux de largeur $`A_1`$ traversé par un flux énergétique $`P`$. Si on cible ce faisceau sur une surface de plein fouet, alors le faisceau lumineux frappe une surface de largeur $`A_1`$ et l'irradiance est donc $`E_1 = \frac{P}{A_1}`$. Maintenant si on incline le faisceau lumineux pour qu'il frappe la surface avec un certain angle, alors le faisceau lumineux va frapper une surface plus grande $`A_2`$ et l'irradiance devient $`E_2 = \frac{P}{A_2}`$. Par trigonométrie, $`A_1`$ et $`A_2`$ sont liés par : $`cos(\theta) = \frac{A_1}{A_2}`$ ou $`\frac{1}{A_2} = \frac{cos(\theta)}{A_1}`$. On peut donc écrire : $`E_2 = \frac{P}{A_2} = \frac{P}{A_1} \cdot cos(\theta) = E_1 \cdot cos(\theta) = E_1(n \cdot L)`$.

En d'autre termes, l'irradiance qui touche la zone $`A_2`$ est égal à l'irradiance à la zone $`A_1`$ perpendiculaire à la direction de la lumière mise à l'échelle par $`n \cdot L = cos(\theta)`$. C'est ce qu'on appelle la loi des cosinus de Lambert. Attention, pour le cas ou la lumière frappe la surface par en dessous (i.e. un produit scalaire négatif), on *clamp* le résultat avec la fonction *max* : $`f(\theta) = \text{max}(cos(\theta), 0) = \text{max}(n \cdot L, 0)`$