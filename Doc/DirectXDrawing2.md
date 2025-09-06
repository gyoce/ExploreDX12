# Dessiner avec DirectX Partie 2

[Page d'accueil](README.md)

Sommaire : 
- [Frame Resource](#frame-resource)

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