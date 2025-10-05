# Textures

[Page d'accueil](README.md)

Sommaire : 
- [Récapitulatif](#récapitulatif)

## Récapitulatif
On utilise déjà des textures à savoir le depth buffer et le back buffer qui sont des textures 2D représentées par l'interface `ID3D12Resource` avec leur `D3D12_RESOURCE_DESC::Dimension` qui vaut `D3D12_RESOURCE_DIMENSION_TEXTURE2D`. Une texture 2D est une matrice d'éléments de données. Une utilisation possible d'une texture 2D est de stocker les données d'une image 2D avec chaque élément dans la texture qui stocke la couleur d'un pixel de l'image. Que ce soit une texture 1D, 2D ou 3D, elles sont toutes représentées par l'interface `ID3D12Resource`.

Les textures sont différents des buffers, qui eux, stockent simplement des tableaux de données. Les textures peuvent avoir des niveaux de mipmap et le GPU peut effectuer des opérations spéciales sur elles comme le filtrage et l'échantillonnage. Les formats de données pour les textures sont décrits par l'énumération `DXGI_FORMAT` et incluent entre autres : 
- `DXGI_FORMAT_R32G32B32_FLOAT` : chaque élément a 3 composantes flottantes de 32 bits.
- `DXGI_FORMAT_R16G16B16A16_UNORM` : chaque élément a 4 composantes de 16 bits dans l'intervalle [0, 1].
- `DXGI_FORMAT_R32G32_UINT` : chaque élément a 2 composantes entières non signées de 32 bits.
- `DXGI_FORMAT_R8G8B8A8_UNORM` : chaque élément a 4 composantes de 8 bits dans l'intervalle [0, 1].
- `DXGI_FORMAT_R8G8B8A8_SNORM` : chaque élément a 4 composantes de 8 bits dans l'intervalle [-1, 1].
- `DXGI_FORMAT_R8G8B8A8_SINT` : chaque élément a 4 composantes entières signées de 8 bits, dans l'intervalle [-128, 127].
- `DXGI_FORMAT_R8G8B8A8_UINT` : chaque élément a 4 composantes entières non signées de 8 bits, dans l'intervalle [0, 255].

Une texture peut être liée à différente étape de la pipeline de rendu. Par exemple, une texture peut être utilisée comme cible de rendu ou comme ressource dans un shader. Le fait d'utiliser une texture comme cible de rendu puis de l'utiliser dans un shader est une méthode qu'on appelle *render-to-texture*. Pour qu'une texture soit utilisée comme cible de rendu et comme ressource de shader, on devra créer deux descripteurs différents pour la même texture : 1) un descripteur qui vit dans un heap de descripteurs de vues de rendu (RTV) et 2) un descripteur qui vit dans un heap de descripteurs de vues de ressources (SRV).