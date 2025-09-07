# Dessiner avec DirectX Partie 2

[Page d'accueil](README.md)

Sommaire : 
- [Frame Resource](#frame-resource)
- [Objets de rendu](#objets-de-rendu)
- [Pass constants](#pass-constants)

## Frame Resource
Pour rappel, le CPU et le GPU travaille en parallèle. Le CPU prépare et soumet des listes de commandes et le GPU traite les commandes dans la file de commande. Le but est de garder le CPU et le GPU occupés pour profiter aux maximum de la puissance de la machine. Jusqu'à présent, on a synchronisé le CPU et le GPU une fois par frame parce que : 
1) L'allocateur de commande ne peut pas être réinitialisé tant que le GPU n'a pas fini d'exécuter les commandes. 
2) Un buffer constant ne peut pas être mis à jour par le CPU tant que le GPU n'a pas fini d'exécuter les commandes de dessin qui référencent ce buffer.

On fait donc appel à la méthode `FlushCommandQueue` à chaque fin de frame pour s'assurer que le GPU a bien fini d'exécuter toutes les commandes pour la frame. Cette solution fonctionne mais n'est pas optimale parce que : 
1) Au début de chaque frame, le GPU n'a aucune commande à traiter puisqu'on attend que la file d'attente de commande soit vide. Il va devoir attendre que le CPU prépare et soumette de nouvelles commandes à exécuter.
2) Á la fin de chaque frame, le CPU attend que le GPU finisse de traiter les commandes.

Une solution pour pallier ce problème est de créer un tableau circulaire des ressources dont le CPU a besoin de modifier chaque frame. On appelle ça des `Frame Resources` et on utilise généralement un tableau de 3 Frame Resources éléments. L'idée est que pour une frame *n*, le CPU parcourra le tableau des Frame Resources pour obtenir la prochaine Frame Resource. Le CPU va ensuite mettre à jour les ressources, puis créera et soummettra les listes de commandes pour la frame *n* tandis que le GPU travaille sur les frames précédentes. Ensuite le CPU travaillera sur la frame *n+1* et ainsi de suite. Si la taille du tableau est de 3, cela permet au CPU d'avoir jusqu'à deux frames d'avance sur le GPU. Voici un exemple d'une classe de Frame Resource : 
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
Et le changement a faire dans la boucle principale de l'application : 
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

    // Ici on peut mettre à jour les ressources de la frame courante (comme les constant buffer par exemple).&
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