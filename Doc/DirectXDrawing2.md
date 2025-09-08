# Dessiner avec DirectX Partie 2

[Page d'accueil](README.md)

Sommaire : 
- [Frame Resource](#frame-resource)
- [Objets de rendu](#objets-de-rendu)
- [Pass constants](#pass-constants)
- [Shape geometry](#shape-geometry)
    - [Le cylindre](#le-cylindre)
    - [La sphère](#la-sphère)

## Frame Resource
Pour rappel, le CPU et le GPU travaillent en parallèle. Le CPU prépare et soumet des listes de commandes et le GPU traite les commandes dans la file de commande. Le but est de garder le CPU et le GPU occupés pour profiter au maximum de la puissance de la machine. Jusqu'à présent, on a synchronisé le CPU et le GPU une fois par frame parce que : 
1) L'allocateur de commande ne peut pas être réinitialisé tant que le GPU n'a pas fini d'exécuter les commandes. 
2) Un buffer constant ne peut pas être mis à jour par le CPU tant que le GPU n'a pas fini d'exécuter les commandes de dessin qui référencent ce buffer.

On fait donc appel à la méthode `FlushCommandQueue` à chaque fin de frame pour s'assurer que le GPU a bien fini d'exécuter toutes les commandes pour la frame. Cette solution fonctionne mais n'est pas optimale parce que : 
1) Au début de chaque frame, le GPU n'a aucune commande à traiter puisqu'on attend que la file d'attente de commande soit vide. Il va devoir attendre que le CPU prépare et soumette de nouvelles commandes à exécuter.
2) À la fin de chaque frame, le CPU attend que le GPU finisse de traiter les commandes.

Une solution pour pallier ce problème est de créer un tableau circulaire des ressources dont le CPU a besoin de modifier chaque frame. On appelle ça des `Frame Resources` et on utilise généralement un tableau de 3 Frame Resources éléments. L'idée est que pour une frame *n*, le CPU parcourra le tableau des Frame Resources pour obtenir la prochaine Frame Resource. Le CPU va ensuite mettre à jour les ressources, puis créera et soumettra les listes de commandes pour la frame *n* tandis que le GPU travaille sur les frames précédentes. Ensuite le CPU travaillera sur la frame *n+1* et ainsi de suite. Si la taille du tableau est de 3, cela permet au CPU d'avoir jusqu'à deux frames d'avance sur le GPU. Voici un exemple d'une classe de Frame Resource : 
```c++
// Permet de stocker les ressources nécessaires au CPU pour construire les listes de commande pour une frame.
struct FrameResource
{
public:
    FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));
        PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    }
    ~FrameResource() = default;
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;

    // On ne peut pas réinitialiser l'allocateur tant que le GPU n'a pas fini de traiter les commandes. Donc chaque frame a son propre allocateur.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // On ne peut pas mettre à jour un constant buffer tant que le GPu n'a pas fini de traiter les commandes qui le référencent. Donc chaque frame a son propre cbuffer.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Valeur de la barrière pour marquer les commandes jusqu'à ce point. Cela nous permet de vérifier si ces ressources de frame sont toujours utilisées par le GPU.
    UINT64 Fence = 0;
};
```
Ensuite notre application va instancier un *vector* de trois Frame Resources, et conservera les variables membres pour suivre la frame resource courante : 
```c++
static const int NumFrameResources = 3;
std::vector<std::unique_ptr<FrameResource>> mFrameResources;
FrameResource* mCurrentFrameResource = nullptr;
int mCurrentFrameResourceIndex = 0;

// Petit exemple pour construire les Frame Resources.
void App::BuildFrameResources()
{
    for(int i = 0; i < NumFrameResources; i++)
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRitems.size()));
}
```
Et le changement à faire dans la boucle principale de l'application : 
```c++
void App::Update()
{
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % NumFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

    // Est-ce que le GPU a fini de traiter les commandes de la frame resource courante ? Si ce n'est pas le cas, on attend que le GPU ait fini de traiter les commandes jusqu'à cette barrière.
    if(mCurrFrameResource->Fence != 0 && mCommandQueue->GetLastCompletedFence() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mCommandQueue->SetEventOnFenceCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    // Ici on peut mettre à jour les ressources de la frame courante (comme les constant buffer par exemple).
}

void App::Draw()
{
    // Ici on peut construire et soumettre les listes de commandes pour cette frame.

    // On avance la valeur de la barrière pour marquer les commandes jusqu'à ce point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // On ajoute une instruction dans la file de commande pour définir un nouveau point de barrière.
    // Parce que nous sommes sur la timeline du GPU, le nouveau point de barrière ne sera défini tant que le GPU n'aura pas fini de traiter toutes les commandes précédant ce Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);

    // Note : le GPU peut encore travailler sur les commandes des frames précédentes, mais ce n'est pas grave, parce qu'on ne touche à aucune ressource de frame associée à ces frames.
}
```

## Objets de rendu
Pour dessiner un objet, on doit effectuer plusieurs choses comme la liaison de sommets et d'indices, la liaison d'objet constants, la définition du type de primitive et la spécification des paramètres de la fonction `DrawIndexedInstanced`. Cela peut être pratique de définir une structure qui permet de stocker les données dont on a besoin pour dessiner un objet. On appelle *Render Item* ou *Objet de rendu* l'ensemble des données nécessaires pour soumettre un appel de dessin à la pipeline. On peut la définir comme suit : 
```c++
// Lightweight structure stores parameters to draw a shape. This will vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;

    // Matrice monde de l'objet qui décrit l'espace local de l'objet par rapport à l'espace monde.
    // Elle définit la position, l'orientation et l'échelle de l'objet dans le monde.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

    // Indicateur de modification indiquant que les données de l'objet ont changé et qu'on doit mettre à jour le constant buffer.
    // Parce qu'on a un objet cbuffer pour chaque FrameResource, on doit appliquer la mise à jour à chaque FrameResource. 
    // Donc, quand on modifie les données de l'objet, on doit définir NumFramesDirty = gNumFrameResources pour que chaque ressource de frame reçoive la mise à jour.
    int NumFramesDirty = gNumFrameResources;

    // Indice dans le buffer constant GPU correspondant à l'ObjectCB pour ce render item.
    UINT ObjCBIndex = -1;

    // Geometry associé à ce render item. Note : plusieurs render items peuvent partager la même géométrie.
    MeshGeometry* Geo = nullptr;

    // Type de primitive.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // Paramètres de la fonction DrawIndexedInstanced.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

// Code de l'application : 
std::vector<std::unique_ptr<RenderItem>> mAllRitems;
std::vector<RenderItem*> mOpaqueRitems; // Liste des objets opaques seulement.
std::vector<RenderItem*> mTransparentRitems; // Liste des objets transparents seulement.
```

## Pass constants
Comme on peut le voir dans la structure `FrameResource`, on a un buffer constant pour les *Pass Constants*. Ce buffer stocke les données constantes qui sont fixées pour chaque passe de rendu comme la position de l'oeil (de caméra), les matrices de vues et de projection et des informations sur la cible de rendu. Voici un exemple de cbuffer de passe de rendu et d'objet avec plusieurs paramètres utiles : 
```hlsl
cbuffer cbPass : register(b1)
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
};

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};
```
L'idée de ces changements est de grouper les constantes par fréquence de changement. Le buffer `cbPerObject` contient des données qui changent lorsque l'objet associé est modifié, alors que le buffer `cbPass` contient des données qui changent uniquement à chaque passe de rendu. On peut alors implémenter des méthodes qui permettent de gérer ces constants buffers (appelées dans la méthode `Update` de l'application) : 
```c++
void App::UpdateObjectCBs()
{
    UploadBuffer<ObjectConstants>* currentObjectCB = mCurrentFrameResource->ObjectCB.get();
    for(std::unique_ptr<RenderItem>& e : mAllRitems)
    {
        // On ne met à jour les données du cbuffer que si les constantes ont changé.
        // Attention : cela doit être suivi pour chaque frame ressource.
        if(e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            currentObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Le prochain FrameResource doit aussi être mis à jour.
            e->NumFramesDirty--;
        }
    }
}

void App::UpdateMainPassCB()
{
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gameTimer.TotalTime();
    mMainPassCB.DeltaTime = gameTimer.DeltaTime();

    UploadBuffer<PassConstants>* currentPassCB = mCurrFrameResource->PassCB.get();
    currentPassCB->CopyData(0, mMainPassCB);
}
```
On doit aussi mettre à jour le vertex shader : 
```hlsl
VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // On transforme vers l'espace d'homogeneous clip
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);

    // On passe la couleur au pixel shader
    vout.Color = vin.Color;
    return vout;
}
```
On doit aussi mettre à jour la root signature pour prendre en compte deux tables de descripteurs : 
```c++
CD3DX12_DESCRIPTOR_RANGE cbvTable0;
cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
CD3DX12_DESCRIPTOR_RANGE cbvTable1;
cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

CD3DX12_ROOT_PARAMETER slotRootParameter[2];

slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
```

## Shape geometry
On va voir comment créer la géométrie pour plusieurs formes comme des ellipsoides, des sphères, des cylindres et des cônes. Voici la classe qui va nous permettre de générer ces formes : 
```c++
using namespace DirectX;

class GeometryGenerator
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    struct Vertex
    {
        Vertex() = default;
        Vertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t,const XMFLOAT2& uv) 
            : Position(p), Normal(n), TangentU(t), TexC(uv) { }
        Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) 
            : Position(px,py,pz), Normal(nx,ny,nz), TangentU(tx, ty, tz), TexC(u,v) { }

        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT3 TangentU;
        XMFLOAT2 TexC;
    };

    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;
        std::vector<uint16>& GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for(size_t i = 0; i < Indices32.size(); i++)
                    mIndices16[i] = static_cast<uint16>(Indices32[i]);
            }
            return mIndices16;
        }

    private:
        std::vector<uint16> mIndices16;
    };
};
```

### Le cylindre
On définit le cylindre en spécifiant son *top radius*, son *bottom radius*, sa hauteur et le nombre de tranches (*slices*) et de segments (*stacks*). Sur l'image ci-dessous, le cylindre de gauche a 8 tranches et 4 segments, celui de droite à 16 trances et 8 segments.

![Cylindre](/Doc/Imgs/Cylinder.png)

On génère le cylindre centré à l'origine, parallèle à l'axe des y. Chaque sommet se situe sur un anneau (*ring*) du cylindre, il y a alors `stackCount + 1` anneaux et chaque anneau a `sliceCount` sommets uniques. On note la différence de rayon $`\Delta r`$ entre le rayon du haut et le rayon du bas et se calcule par $`\Delta r = (topRadius - bottomRadius) / stackCount`$. Si l'on commence par l'anneau le plus bas avec un indice de 0, alors le rayon à l'anneau $`i`$ est $`r_i = bottomRadius + i * \Delta r`$ et la hauteur de l'anneau $`i`$ est $`h_i = -\frac{h}{2} + i \Delta h`$ avec $`\Delta h`$ qui est la hauteur de segment et $`h`$ la hauteur du cylindre.

L'idée est donc d'itérer sur chaque anneau et de générer les sommets qui le composent. Voici un exemple d'implémentation : 
```c++
GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;
    float stackHeight = height / stackCount;
    float radiusStep = (topRadius - bottomRadius) / stackCount; // Δr
    uint32 ringCount = stackCount + 1;
    // On calcule les sommets pour chaque anneau en partant du bas et en allant vers le haut.
    for(uint32 i = 0; i < ringCount; i++)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;
        float dTheta = 2.0f * XM_PI / sliceCount;
        for(uint32 j = 0; j <= sliceCount; j++)
        {
            Vertex vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);
            vertex.Position = XMFLOAT3(r * c, y, r * s);
            vertex.TexC.x = (float) j / sliceCount;
            vertex.TexC.y = 1.0f - (float) i / stackCount;
            vertex.TangentU = XMFLOAT3(-s, 0.0f, c);
            float dr = bottomRadius - topRadius;
            XMFLOAT3 bitangent(dr * c, -height, dr * s);
            XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.Normal, N);
            meshData.Vertices.push_back(vertex);
        }
    }

    // On ajoute 1 parce qu'on duplique le premier et le dernier sommet par anneau comme les coordonnées de texture sont différentes.
    uint32 ringVertexCount = sliceCount + 1;
    // Pour chaque segments, on calcule les indices.
    for(uint32 i = 0; i < stackCount; i++)
    {
        for(uint32 j = 0; j < sliceCount; j++)
        {
            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
            meshData.Indices32.push_back(i * ringVertexCount + j + 1);
        }
    }

    BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
    BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
    return meshData;
}
```
Le cylindre peut être paramétré comme suit, on introduit le paramètre $`v`$ qui va dans la même direction que la coordonnée de texture v. Ainsi la tangente va dans la même direction que la coordonnée de texture v. Soit $`r_0`$ le rayon du bas et $`r_1`$ le rayon du haut, on a : 
```math
\begin{aligned}
&y(v) = h - hv \quad \text{pour } v \in [0,1]. \\
&r(v) = r_1 + (r_0 - r_1)v \\ \\

&x(t, v) = r(v) \cos(t) \\
&y(t, v) = h - hv \\
&z(t, v) = r(v) \sin(t) \\ \\

&\frac{dx}{dt} = -r(v) \sin(t) \\
&\frac{dy}{dt} = 0 \\
&\frac{dz}{dt} = +r(v) \cos(t) \\ \\

&\frac{dx}{dv} = (r_0 - r_1) \cos (t) \\
&\frac{dy}{dv} = -h \\
&\frac{dz}{dv} = (r_0 - r_1) \sin (t)
\end{aligned}
```
La génération du capuchon (*cap*) du haut et du bas revient à générer les triangles de tranche des anneaux supérieur et inférieur afin d'approximer un cercle : 
```c++
void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();
    float y = 0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;
    // On duplique les sommets du capuchon parce que les coordonnées de texture et les normales sont différentes.
    for(uint32 i = 0; i <= sliceCount; i++)
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);
        // On réduit par la hauteur pour essayer de rendre la zone des coordonnées de texture du capuchon proportionnelle à la base.
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }
    // Sommets du centre du capuchon.
    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
    // Indice du sommet central.
    uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;
    for(uint32 i = 0; i < sliceCount; i++)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i + 1);
        meshData.Indices32.push_back(baseIndex + i);
    }
}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();
    float y = -0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;
    
    for(uint32 i = 0; i <= sliceCount; i++)
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    uint32 centerIndex = (uint32)meshData.Vertices.size()-1;
    for(uint32 i = 0; i < sliceCount; i++)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }
}
```

### La sphère
On peut définir une sphère en spécifiant son rayon et le nombre de tranches (*slice*) et de segments (*stack*). L'algorithme pour générer une sphère est très similaire à celui du cylindre, à l'exception du rayon par anneau qui change de manière non linéaire en fonction des fonctions trigonométriques. Voici un exemple d'implémentation :

![Sphère](/Doc/Imgs/Sphere.png)

```c++
GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;

    // Calcule les sommets en partant du pôle supérieur et en descendant les segments.

    // Pour les poles il faut noter qu'il y aura une distorsion des coordonnées de texture car il n'y a pas de point unique sur la carte de texture à assigner au pôle lorsqu'on mappe une texture rectangulaire sur une sphère.
    Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    meshData.Vertices.push_back(topVertex);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI /sliceCount;

    // Calcule des sommets pour chaque anneau de segment (on ne compte pas les pôles comme des anneaux).
    for(uint32 i = 1; i <= stackCount - 1; i++)
    {
        float phi = i * phiStep;

        // Pour les sommets de l'anneau.
        for(uint32 j = 0; j <= sliceCount; j++)
        {
            float theta = j * thetaStep;

            Vertex v;

            // Coordonnées sphériques vers cartésiennes
            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * sinf(theta);

            // Dérivée partielle de P par rapport à theta
            v.TangentU.x = -radius * sinf(phi) * sinf(theta);
            v.TangentU.y = 0.0f;
            v.TangentU.z = +radius * sinf(phi) * cosf(theta);

            XMVECTOR T = XMLoadFloat3(&v.TangentU);
            XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

            XMVECTOR p = XMLoadFloat3(&v.Position);
            XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

            v.TexC.x = theta / XM_2PI;
            v.TexC.y = phi / XM_PI;

            meshData.Vertices.push_back(v);
        }
    }

    meshData.Vertices.push_back(bottomVertex);

    // Calcule des indices pour le segment du haut. 
    // Le segment du haut a été écrit en premier dans le buffer de sommets et connecte le pôle supérieur au premier anneau.
    for(uint32 i = 1; i <= sliceCount; i++)
    {
        meshData.Indices32.push_back(0);
        meshData.Indices32.push_back(i + 1);
        meshData.Indices32.push_back(i);
    }
    
    // Calcule des indices pour les segments intérieurs.
    // Décale les indices à l'indice du premier sommet du premier anneau. (On saute juste le sommet du pôle supérieur.)
    uint32 baseIndex = 1;
    uint32 ringVertexCount = sliceCount + 1;
    for(uint32 i = 0; i < stackCount - 2; i++)
    {
        for(uint32 j = 0; j < sliceCount; ++j)
        {
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // Calcule des indices pour le segment du bas.
    // Le segment du bas a été écrit en dernier dans le buffer de sommets et connecte le pôle inférieur au dernier anneau. Le segment du bas a été ajouté en dernier.
    uint32 southPoleIndex = (uint32)meshData.Vertices.size()-1;
    // Décale les indices à l'indice du premier sommet du dernier anneau.
    baseIndex = southPoleIndex - ringVertexCount;    
    for(uint32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(southPoleIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }

    return meshData;
}
```