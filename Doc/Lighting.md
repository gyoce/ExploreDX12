# Lighting / Éclairage

[Page d'accueil](README.md)

Sommaire : 
- [Interaction entre la lumière et les matériaux](#interaction-entre-la-lumière-et-les-matériaux)
- [Vecteurs normaux](#vecteurs-normaux)
    - [Calcul des vecteurs normaux](#calcul-des-vecteurs-normaux)
    - [Transformation des vecteurs normaux](#transformation-des-vecteurs-normaux)
- [Vecteurs importants pour l'éclairage](#vecteurs-importants-pour-léclairage)
- [La loi des cosinus de Lambert](#la-loi-des-cosinus-de-lambert)
- [Éclairage diffus](#éclairage-diffus)
- [Éclairage ambiant](#éclairage-ambiant)
- [Éclairage spéculaire](#éclairage-spéculaire)
    - [Effet de Fresnel](#effet-de-fresnel)
    - [La rugosité](#la-rugosité)
- [Récapitulatif](#récapitulatif)
- [Matériaux](#matériaux)
- [Lumières parallèles](#lumières-parallèles)
- [Points lumineux](#points-lumineux)
    - [Atténuation](#atténuation)
- [Projecteurs](#projecteurs)

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

## Éclairage diffus
Considérons un objet opaque. Quand la lumière frappe un point sur la surface, un peu de lumière entre à l'intérieur de l'objet et interagit avec la matière proche de la surface. La lumière rebondira à l'intérieur, sera absorbée et dispersée dans diverses directions, c'est ce qu'on appelle une réflexion diffuse. La quantité d'absorption et de dispersion dépend du matériau (bois, terre, brique, tuile, etc.). Pour simplifier, on va considérer que la lumière est dispersée uniformément dans toutes les directions, en conséquence, la lumière réfléchie atteindra l'oeil indépendamment de la position de l'oeil.

Pour le calcul de l'éclairage diffus, on précise une couleur de lumière et une couleur d'albedo (couleur diffuse). L'albedo spécifie la quantité de lumière entrante que la surface réfléchit. Par exemple, supposons qu'un point sur la surface réfléchisse 50% de la lumière rouge, 100% de la lumière verte et 75% de la lumière bleue et que la couleur de la lumière entrante est de 80% d'intensité blanche. On a alors $`B_L = (0.8, 0.8, 0.8)`$ et l'albedo $`m_d = (0.5, 1.0, 0.75)`$. Alors la quantité de lumière réfléchie est donné par : $`c_d = B_L \cdot m_d = (0.8, 0.8, 0.8) \otimes (0.5, 1.0, 0.75) = (0.4, 0.8, 0.6)`$. 

Attention, cela ne suffit pas pour la rendre correcte, il faut inclure la loi des cosinus de Lambert. On a $`B_L`$ qui représente la quantité de lumière entrante, $`m_d`$ qui est l'albedo, $`L`$ le vecteur lumière et $`n`$ la normale à la surface. Alors la quantité de lumière diffus réfléchie par la surface est donnée par : $`c_d = \text{max}(L \cdot n) \cdot B_L \otimes m_d`$.

## Éclairage ambiant
Dans le monde réel, la plupart de la lumière qu'on voit est indirecte. Pour en quelque sorte simuler cette lumière indirecte, on introduit un terme d'éclairage ambiant à l'équation de la lumière : $`c_a = A_L \otimes m_d`$. La couleur $`A_L`$ spécifie la quantité totale de lumière (ambiante) indirecte qu'une surface reçoit. Elle peut être différente de la lumière émise par la source à cause de l'absorption qui s'est produit lorsque la lumière rebondi sur d'autres surfaces. L'albedo diffus $`m_d`$ spécifie la quantité de lumière incidente que la surface réfléchit.

## Éclairage spéculaire
On utilise l'éclaire diffus pour modéliser la réflexion diffuse, où la lumière pénètre dans un milieu, rebondit, une partie est absorbée et le reste est diffusé hors du milieu dans toutes les directions. Un second type de reflection survient à cause de l'effet de Fresnel qui est un phénomène physique. Quand la lumière atteint l'interface entre deux milieux ayants des indices de réfraction différents, un peu de lumière est réfléchie et le reste est réfractée. L'indice de réfraction est une propriété physique d'un milieu qui est le rapport entre la vitesse de la lumière dans le vide et la vitesse de la lumière dans le milieu. 

![Effet de Fresnel](/Doc/Imgs/FresnelEffect.png)
(a) L'effet de Fresnel pour un miroir parfaitement plat avec une normale $`n`$. La lumière incidente $`I`$ est divisée : une partie est réfléchie dans la direction $`r`$ et l'autre partie est réfractée dans la direction $`t`$. Tous ces vecteurs sont dans le même plan. L'angle entre le vecteur de reflexion et la normale est toujours $`\theta_i`$ et qui est le même que l'angle entre le vecteur lumière $`L = -I`$ et la normale $`n`$. L'angle $`\theta_t`$ entre le vecteur de réfraction et $`-n`$ dépend des indices de réfraction entre les deux milieux.

(b) La plupart des objets ne sont pas des miroirs parfaitement plats mais ont une certaine rugosité microscopique. Cela provoque la diffusion de la lumière réfléchie et réfractée autour des vecteurs de réflexion et de réfraction.

Pour des objets opaques, la lumière réfractée entre dans le milieu et subit une réflectance diffuse. On a donc la quantité de lumière qui se réflète sur une surface et atteint l'oeil est une combinaison de la lumière réfléchie (diffuse) par le coprs et de réflexion spéculaire. Contrairement à la lumière diffuse, la lumière spéculaire peut ne pas atteindre l'oeil car elle se reflète dans une direction spécifique. Le calcul de l'éclairage spéculaire dépend donc de la position de l'oeil.

### Effet de Fresnel
Considérons une surface plane avec une normale $`n`$ qui sépare deux milieux avec des indices de réfraction différents. Á cause de la discontinuité de l'indice de réfraction à la sur face, quand la lumière frappe la surface, une partie de celle-ci est réfléchie et une autre partie est réfractée dans la surface. Les équations de Fresnel décrivent le pourcentage de lumière entrante qui est réfléchie $`0 \leqslant R_F \leqslant 1`$. Par conservation d'énergie, si $`R_F`$ est la quantité de lumière réfléchie, alors $`1 - R_F`$ est la quantité de lumière réfractée. La valeur $`R_F`$ est un vecteur RGB parce que la quantité de réflection dépend de la couleur de la lumière. 

La quantité de lumière réfléchie du milieu mais aussi de l'angle d'incidence $`\theta_i`$ entre le vecteur lumière $`L`$ et la normale $`n`$. Les équations de Fresnel sont plutôt complexes, on utilise plutôt l'approximation de Schlick : $`R_F(\theta_i) = R_F(0^{\circ}) + (1 - R_F(0^{\circ}))(1 - cos(\theta_i))^5`$ avec $`R_F(0^{\circ})`$ qui est une propriété du milieu. Par exemple :
| Milieu           | $R_F(0^{\circ})$   |
|------------------|--------------------|
| Eau              | (0.02, 0.02, 0.02) |
| Verre            | (0.08, 0.08, 0.08) |
| Plastique        | (0.05, 0.05, 0.05) |
| Or               | (1.00, 0.71, 0.29) |
| Argent           | (0.95, 0.93, 0.88) |
| Cuivre           | (0.95, 0.64, 0.54) |

![Effet de Fresnel 2](/Doc/Imgs/FresnelEffect2.png)

La quantité de reflection augmente quand l'angle d'incidence se rapproche de 90 degrés. Si on regarde vers le bas (a), on voit principalement le fond de l'eau parce que la lumière provenant de l'environnement qui se reflète dans notre oeil forme un petit angle $`\theta_i`$ proche de 0 degré, la réflection est alors faible et par conservation d'énergie, la réfraction est élevée. A contrario, si on regarde l'eau à un angle rasant (b), on voit principalement la réflection de l'environnement parce que la lumière qui se reflète dans notre oeil forme un grand angle $`\theta_i`$ proche de 90 degrés, la réflection est alors élevée et par conservation d'énergie, la réfraction est faible. Ce comportement est souvent appelé l'**effet de Fresnel**.

### La rugosité
Les objets réfléchissants dans le monde réel ne sont généralement pas des miroirs parfaits. Même si la surface de l'objet semble lisse à l'oeil nu, elle a souvent une certaine rugosité microscopique. Quand la rugosité augmente, la direction des micro-normales varie plus, ce qui provoque la diffusion de la lumière réfléchie en un lobe spéculaire. Pour modéliser la rugosité mathématiquement, on emploie le modèle de micro-facettes, on modélise la surface microscopique comme une collection de petits éléments plats qu'on appelle des micro-facettes. Les micro-normales sont les normales de ces micro-facettes. 

Pour une vue donnée $`v`$ et un vecteur lumière $`L`$, on veut savoir la fraction de micro-facettes qui réfléchissent la lumière $`L`$ dans la direction $`v`$. Autrement dit, la fraction de micro-facettes avec une normale $`h = \text{normalize}(L + v)`$. Cela nous donnera l'indication de la quantité de lumière réfléchit dans l'oeil depuis la réflection spéculaire. Plus il ya de micro-facettes qui réfléchissent $`L`$ dans la direction $`v`$, plus la lumière spéculaire perçue par l'oeil sera intense.

![Micro-facettes](/Doc/Imgs/Microfacets.png)
Le vecteur $`h`$ est appelé le vecteur demi (*halfway vector*). On introduit aussi l'angle $`\theta_h`$ entre le halfway vecteur $`h`$ et la macro-normale $`n`$. 

On définit une fonction de distribution normalisée $`\rho(\theta_h) \in [0, 1]`$ pour designer la fraction de micro-facettes dont les normales $`h`$ forment un angle $`\theta_h`$ avec la macro-normale $`n`$. Intuitivement, on s'attend à ce que $`\rho(\theta_h)`$ soit maximale quand $`\theta_h = 0^{\circ}`$. C'est-à-dire qu'on s'attend à ce que les normales des micro-facettes soient biaisées vers la macro-normale mais aussi à mesure que $`\theta_h`$ augmente (à mesure que $`h`$ diverge de $`n`$), on s'attend à ce que la fraction de micro-facettes avec la normale $`h`$ diminue. On utilise souvent la fonction $`\rho(\theta_h) = cos^m(\theta_h) = cos^m(n \cdot h)`$. Ici $`m`$ contrôle la rugosité, à mesure que $`m`$ diminue, la surface devient plus rugueuse et a contrario, à mesure que $`m`$ augmente, la surface devient plus lisse.

On peut combiner $`\rho(\theta_h)`$ avec un facteur de normalisation pour obtenir une nouvelle fonction qui modélise la quantité de réflexion spéculaire de la lumière en fonction de la rugosité : $`S(\theta_h) = \frac{m+8}{8}cos^m(\theta_h) = \frac{m+8}{8}(n \cdot h)^{m}`$. Pour un $`m`$ petit, la surface est rugueuse et le lobe spéculaire s'élargit parce que l'énergie lumineuse est dispersée et inversement.

On peut combiner la réflexion de Fresnel avec la rugosité de la surface. On essaye de calculer la quantité de lumière qui est réfléchie dans la direction de vue $`v`$. Soit $`\alpha_h`$ l'angle entre le vecteur lumière et le halfway vecteur $`h`$, alors $`R_F(\alpha_h)`$ nous donne la quantité de lumière réfléchie entre $`h`$ et $`v`$. En multipliant la quantité de lumière réfléchie $`R_F(\alpha_h)`$ due à l'effet de Fresnel par la quantité de lumière réfléchie due à la rugosité $`S(\theta_h)`$, on obtient la quantité de lumière réfléchie spéculaire. Soit $`(\text{max}(L \cdot n, 0) \cdot B_L)`$ qui représente la quantité de lumière incidente qui frappe la surface en un point. Alors la fraction de $`(\text{max}(L \cdot n, 0) \cdot B_L)`$ réfléchie de manière spéculaire dans l'oeil en raison de la rugosité et de l'effet de Fresnel est donnée par : \
$`c_s = \text{max}(L \cdot n, 0) \cdot B_L \otimes R_F(\alpha_h) \frac{m + 8}{8}(n \cdot h)^{m}`$

## Récapitulatif
La lumière totale réfléchie par une surface est la somme de la réflectance de la lumière ambiante, de la réflectance de la lumière diffuse et de la réflectance de la lumière spéculaire : 
1) Lumière ambiante $`c_a`$ : modélise la quantité de lumière réfléchie par la surface en raison de la lumière indirecte.
2) Lumière diffuse $`c_d`$ : modélise la lumière qui entre dans le milieu, se diffuse sous la surface où une partie est absorbée et le reste est diffusé à nouveau hors de la surface. Comme il est difficile de modéliser cette diffusion sous la surface, on suppose que la lumière est réémise de manière égale dans toutes les directions au dessus de la surface autour du point d'incidence.
3) Lumière spéculaire $`c_s`$ : modélise la lumière qui est réfléchie par la surface en raison de l'effet de Fresnel et de la rugosité de la surface.

Ce qui nous amène à l'équation finale de l'éclairage : \
$`c = c_a + c_d + c_s`$ \
$`c = A_L \otimes m_d + \text{max}\left(L \cdot n, 0\right) \cdot B_L \otimes \left( m_d + R_F(\alpha_h) \frac{m + 8}{8}(n \cdot h)^{m} \right)`$ \
Tous les vecteurs de cette équation doivent être unitaires.
1. $`L`$ est le vecteur lumière qui pointe vers la source de lumière.
2. $`n`$ est la normale de surface.
3. $`h`$ est le halfway vecteur entre le vecteur lumière et le vecteur de vue.
4. $`A_L`$ représente la quantité de lumière ambiante incidente.
5. $`B_L`$ représente la quantité de lumière directe incidente.
6. $`m_d`$ spécifie la quantité de lumière incidente que la surface réfléchit en raison de la réflectance diffuse.
7. $`L \cdot n`$ est la loi des cosinus de Lambert.
8. $`\alpha_h`$ est l'angle entre le halfway vecteur et le vecteur lumière.
9. $`R_F(\alpha_h)`$ est la quantité de lumière réfléchie vers l'oeil en raison de l'effet de Fresnel.
10. $`m`$ contrôle la rugosité de la surface.
11. $`(n \cdot h)`$ spécifie la fraction de micro-facettes avec des normales $`h`$ qui forment un angle $`\theta_h`$ avec la macro-normale $`n`$.
12. $`(m + 8) / 8`$ est un facteur de normalisation pour modéliser la conservation d'énergie dans la réflection spéculaire.

## Matériaux
Voici un exemple de structure pour un matériau : 
```c++
struct Material
{
    // Nom unique du matériau pour la recherche.
    std::string Name;
    
    // Indice dans le buffer constant correspondant à ce matériau.
    int MatCBIndex = -1;
    
    // Index dans le tas SRV pour la texture diffuse.
    int DiffuseSrvHeapIndex = -1;

    // Flag indiquant si le matériau a changé et qu'on doit mettre à jour le buffer constant. 
    // Parce qu'on a un buffer constant de matériau pour chaque FrameResource, on doit appliquer la mise à jour à chaque FrameResource.
    // Donc, quand on modifie un matériau, on doit définir NumFramesDirty = gNumFrameResources pour que chaque FrameResource reçoive la mise à jour.
    int NumFramesDirty = gNumFrameResources;

    // Données du buffer constant de matériau utilisées pour le shading.
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;
    DirectX::XMFLOAT4X4 MatTransform = MathUtils::Identity4x4();
};
```
Pour modéliser des matériaux réalistes, on a besoin de définir des valeurs réalistes pour les propriétés du matériau (`DiffuseAlbedo` et `FresnelR0`). Pour compenser le fait qu'on ne fasse pas une simulation d'éclairage à 100%, on donne une valeur plutôt faible pour la variable `DiffuseAlbedo` plutôt que 0. Pour ce qui est de la rugosité, dans notre structure elle est définie entre 0.0 et 1.0. Une rugosité de 0 signifie que la surface est parfaitement lisse et une rugosité de 1 signifie que la surface est extrêmement rugueuse. On peut donc déduire la brillance (*shininess*) du matériau en utilisant la rugosité : $`\text{shininess} = 1.0 - \text{roughness} \in [0, 1]`$.

La question qui se pose maintenant est de savoir à quel niveau de granularité on doit spécifier les valeurs des matériaux. Les valeurs de matériau peuvent varier selon la surface, c'est-à-dire que différents points d'une surface peuvent avoir des valeurs de matériau différentes. Pour gérer cela, une solution possible est de spécifier les valeurs de matériau par sommet puis d'interpoler les valeurs durant la rasterisation. Cependant, ce n'est pas une bonne méthode pour modéliser de manière réaliste les détails fins.Une meilleure approche est d'utiliser le mappage de texture (*texture mapping*) que l'on verra plus tard. Pour le moment, on autorise les modifications des matériaux à la fréquence d'appel de dessin.
```c++
std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

void BuildMaterials()
{
    std::unique_ptr<Material> grass = std::make_unique<Material>();
    grass->Name = “grass”;
    grass->MatCBIndex = 0;
    grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.6f, 1.0f);
    grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    grass->Roughness = 0.125f;

    // Ce n'est pas une bonne définition de matériau pour l'eau, mais pour le moment on a pas encore tous les outils de rendu dont on a besoin (transparence, environnement, réflexion), donc on le simule pour l'instant.
    std::unique_ptr<Material> water = std::make_unique<Material>();
    water->Name = “water”;
    water->MatCBIndex = 1;
    water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->Roughness = 0.0f;
    mMaterials[“grass”] = std::move(grass);
    mMaterials[“water”] = std::move(water);
}
```
La map `mMaterials` permet de stocker les données des matériaux dans la mémoire CPU mais pour que le GPU accède aux données des matériaux dans les shaders, on doit reproduire les données dans un buffer constant. On a donc juste besoin d'ajouter un buffer constant de matériau pour chaque FrameResource.
```c++
struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct FrameResource
{ 
public:
    // ...
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    // ...
};
```
Dans la fonction `Update`, les données des matériaux sont copiées dans une sous-région du buffer constant chaque fois qu'ils sont modifiés (*dirty*).
```c++
void UpdateMaterialCBs()
{
    UploadBuffer<MaterialConstants>* currentMaterialCB = mCurrentFrameResource->MaterialCB.get();
    for(const std::unique_ptr<Material>& e : mMaterials)
    {
        // On met à jour les données du buffer constant seulement si les constantes ont changé. 
        // Si les données du buffer constant changent, elles doivent être mises à jour pour chaque FrameResource.
        Material* mat = e.second.get();
        if(mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            currentMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            // La prochaine FrameResource doit être mise à jour aussi.
            mat->NumFramesDirty--;
        }
    }
}
```
Maintenant, chaque *RenderItem* contient un pointeur vers le matériau qu'il utilise. Chaque matériau a un indice qui spécifie où se trouvent ses données constants dans le buffer constant de matériau. À partir de ça, on peut offsetter vers l'adresse virtuelle des données constantes nécessaires pour le RenderItem qu'on veut dessiner. Voici un exemple de code qui permet de dessiner des RenderItems avec différents matériaux : 
```c++
void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
    ID3D12Resource* objectCB = mCurrFrameResource->ObjectCB->Resource();
    ID3D12Resource* matCB = mCurrFrameResource->MaterialCB->Resource();
    for(size_t i = 0; i < ritems.size(); i++)
    {
        RenderItem* ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize; // Nouveau
        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress); // Nouveau
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}
```
On rappelle qu'on a besoin des vecteurs normaux à chaque point de la surface d'un maillage triangulé ce qui nous permet de déterminer l'angle auquel la lumière frappe la surface. Pour obtenir une approximation des normales de surface en chaque point, on spécifie les normales sur chaque sommet. Ces normales seront alors interpolées à travers le triangle pendant la rasterisation.

## Lumières parallèles
Une lumière parallèle (ou lumière directionnelle) approxime une source de lumière très éloignée. On peut donc approximer tous les rayons lumineux comme étant parallèles. De plus, comme la source lumineuse est très éloignée, on peut ignorer les effets de distance et supposer que l'intensité lumineuse est constante partout. Une source lumineuse parallèle est définie par un vecteur qui spécifie la direction que les rayons lumineux parcourent. Le vecteur lumière pointe vers la direction opposée.

## Points lumineux
Un bon exemple de point lumineux est une ampoule. Elle rayonne de manière sphérique dans toutes les directions. En particulier, pour un point arbitraire $`P`$, il existe un rayon lumineux provenant du point lumineux $`Q`$ et se déplaçant vers le point. On définit encore le vecteur lumière allant dans la direction opposée i.e. la direction allant du point $`P`$ vers le point source de lumière $`Q`$ : $`L = \frac{Q - P}{\|Q - P\|}`$. La différence principale entre un point lumineux et une lumière parallèle est la manière de calculer le vecteur lumière $`L`$.

### Atténuation
Physiquement, l'intensité lumineuse diminue en fonction de la distance selon la loi de l'inverse du carré. On peut donc définir l'intensité lumineuse à un point de distance $`d`$ de la source lumineuse comme : $`I = \frac{I_0}{d^2}`$ avec $`I_0`$ qui est l'intensité lumineuse à une distance $`d = 1`$ de la source lumineuse. Cela fonctionne bien pour des lumières réelles mais il existe une formule plus simple que l'on peut utiliser qui est une fonction de décroissante linéaire : $`\text{att}(d) = \text{saturate}(\frac{\text{falloffEnd} - d}{\text{falloffEnd} - \text{falloffStart}})`$, avec la fonction $`\text{saturate}(x)`$ qui vaut $`x`$ quand $`0 \leqslant x \leqslant 1`$, $`0`$ quand $`x < 0`$ et $`1`$ quand $`x > 1`$.

## Projecteurs
Un bon exemple de projecteur est une lampe torche. Un projecteur a une position $`Q`$, est dirigé dans une direction $`d`$ et rayonne la lumière dans un cône avec un angle $`\phi_{max}`$. Le calcul du vecteur lumière $`L`$ est le même que pour un point lumineux : $`L = \frac{Q - P}{\|Q - P\|}`$ avec $`P`$ la position du point qui est éclairé. On peut observer qu'un point est dans le cône du projecteur si et seulement si l'angle entre le vecteur $`-L`$ et la direction du projecteur $`d`$ est plus petit que l'angle $`\phi_{max}`$. 

Il faut savoir que la lumière d'un projecteur ne donne pas des résultats égaux partout, la lumière au centre du cône est plus intense que la lumière près du bord du cône. On peut utiliser une fonction qui permet de calculer cette intensité : $`k_{spot}(\phi) = \text{max}(cos(\phi), 0)^s = \text{max}(-L \cdot d, 0)^s`$. L'intensité diminue donc progressivement à mesure que $`\phi`$ augmente ; mais aussi en modifiant l'exposant $`s`$, on peut contrôler indirectemet $`\phi_{max}`$. Par exemple, si on prend $`s = 8`$, le cône a approximativement un demi-angle de $`45^{\circ}`$.

## Implémentation de l'éclairage
### Structure de la lumière
Voici un exemple de structure pour une lumière, elle peut représenter une lumière parallèle, un point lumineux ou un projecteur. Cependant, en fonction du type de lumière, certaines variables ne seront pas utilisées.
```c++
struct Light
{
    DirectX::XMFLOAT3 Strength;  // Couleur de la lumière
    float FalloffStart;          // Uniquement point lumineux et projecteurs
    DirectX::XMFLOAT3 Direction; // Uniquement directionnelle et projecteurs
    float FalloffEnd;            // Uniquement point lumineux et projecteurs
    DirectX::XMFLOAT3 Position;  // Uniquement point lumineux et projecteurs
    float SpotPower;             // Uniquement projecteurs
};
```
On peut aussi définir la structure qu'on utilisera dans le *hlsl* : 
```hlsl
struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};
```
L'ordre dans lequel on définit les membres de la structure n'est pas arbitraire, il est fait pour minimiser le *padding* (remplissage) dans la mémoire GPU. La structure ci-dessus sera packée en 3 vecteurs à 4 dimensions : 
- vecteur 1 : (Strength.x, Strength.y, Strength.z, FalloffStart)
- vecteur 2 : (Direction.x, Direction.y, Direction.z, FalloffEnd)
- vecteur 3 : (Position.x, Position.y, Position.z, SpotPower)

### Fonctions utiles d'éclairage
On peut définir des fonctions qui nous seront bien utiles dans le shader : 
```hlsl
// Implémentation d'un facteur d'atténuation linéaire, s'applique pour les points lumineux et les projecteurs.
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// L'approximation de Schlick pour les équations de Fresnel, cela permet d'approximer le pourcent de lumière réfléchie d'une surface avec une normale n avec un angle entre le vecteur lumière L et la normale n.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0*f0*f0*f0*f0);
    return reflectPercent;
}

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    // La brillance est l'inverse de la rugosité : Shininess = 1 - roughness.
    float Shininess;
};

// Calcul de la quantité de lumière réfléchie dans l'oeil, c'est le somme de la réflectance diffuse et de la réflectance spéculaire.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    // L'équation spéculaire va en dehors de l'interval [0,1], mais on fait du rendu LDR. Donc on la scale un peu.
    float3 specAlbedo = fresnelFactor * roughnessFactor;
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}
```
### Implémentation des lumières directionnelles
Soit la position de l'oeil $`E`$ et un point $`p`$ sur une surface visible pour l'oeil avec une normale $`n`$ et ses propriétés de matériau. La fonction suivante permet de calculer la quantité de lumière, depuis une source de lumière directionnelle, qui sera réfléchie dans la direction de l'oeil $`v = \text{normalize}(E - p)`$ : 
```hlsl
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // Le vecteur lumière pointe vers la source de lumière.
    float3 lightVec = -L.Direction;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}
```
### Implémentation des points lumineux
```hlsl
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Le vecteur depuis la surface vers la lumière.
    float3 lightVec = L.Position - pos;
    // La distance entre la surface et la lumière.
    float d = length(lightVec);
    // Test de portée.
    if(d > L.FalloffEnd)
        return 0.0f;
    // Normaliser le vecteur de lumière.
    lightVec /= d;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    // Atténuer la lumière par la distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}
```
### Implémentation des projecteurs
```hlsl
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Le vecteur depuis la surface vers la lumière.
    float3 lightVec = L.Position - pos;
    // La distance entre la surface et la lumière.
    float d = length(lightVec);
    // Test de portée.
    if(d > L.FalloffEnd)
        return 0.0f;
    // Normaliser le vecteur de lumière.
    lightVec /= d;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    // Atténuer la lumière par la distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;
    // Scale par le projecteur
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}
```
### Accumulation de plusieurs lumières
L'éclairage est additif, donc on peut avoir plusieurs lumières dans une scene, on a juste besoin d'itérer sur chaque source de lumière et sommer leur contribution. Ici on part du principe qu'on a un maximum de 16 lumières et on utilise la convention que les lumières directionnelles sont en premier dans le tableau, puis les points lumineux et enfin les projecteurs.
```hlsl
#define MaxLights 16

// Les données constantes qui varient par matériau.
cbuffer cbPass : register(b2)
{
    // ...
    // Indices [0, NUM_DIR_LIGHTS[ sont des lumières directionnelles.
    // Indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS + NUM_POINT_LIGHTS[ sont des points lumineux. 
    // indices [NUM_DIR_LIGHTS + NUM_POINT_LIGHTS, NUM_DIR_LIGHTS + NUM_POINT_LIGHT + NUM_SPOT_LIGHTS[ sont des projecteurs.
    Light gLights[MaxLights];
};

float4 ComputeLighting(Light gLights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 result = 0.0f;
    int i = 0;
#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; i++)
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
#endif
#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i++)
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
#endif
#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; i++)
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
#endif
    return float4(result, 0.0f);
}
```
### Fichier HLSL principal
Voici le code qui contient le vertex et le pixel shader qu'on pourra utiliser : 
```hlsl
// Définition par défaut des nombres de lumières.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


#include "LightingUtil.hlsl"

// Données constantes qui varient par frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

// Données constantes qui varient par matériau.
cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

// Données constantes qui varient par passe.
cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    // Transformation en espace monde.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    // On assume un scaling non uniform, on doit donc utiliser la matrice inverse-transpose du monde.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    // Transformation en espace homogeneous clip.
    vout.PosH = mul(posW, gViewProj);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // L'interpolation des normales peut les dénormaliser, on doit donc les renormaliser.
    pin.NormalW = normalize(pin.NormalW);
    // Vecteur du point éclairé vers l'oeil.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    // Éclairage indirect.
    float4 ambient = gAmbientLight*gDiffuseAlbedo;
    // Éclairage direct.
    const float shininess = 1.0f - gRoughness;
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);
    float4 litColor = ambient + directLight;
    // Convention courante de prendre l'alpha depuis le matériau diffus.
    litColor.a = gDiffuseAlbedo.a;
    return litColor;
}
```