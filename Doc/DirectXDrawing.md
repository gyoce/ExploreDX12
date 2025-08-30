# Dessiner avec DirectX

[Page d'accueil](README.md)

Sommaire : 
- [Sommets et input layouts](#sommets-et-input-layouts)
- [Vertex Buffer](#vertex-buffer)
- [Index Buffer](#index-buffer)
- [Vertex shader](#vertex-shader)
- [Pixel shader](#pixel-shader)
- [Constant buffers](#constant-buffers)
- [Compilation des shaders](#compilation-des-shaders)

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

## Vertex buffer
Pour que le GPU accède à un tableau de sommets, ils doivent être placés dans une ressource GPU `ID3D12Resource` qu'on appelle tampon (*buffer*), plus particulièrement tampon de sommet (*vertex buffer*). On peut donc créer une `ID3D12Resource` en remplissant une structure `D3D12_RESOURCE_DESC` décrivant le tampon et ensuite appeler la méthode `ID3D12Device::CreateCommittedResource`. *Direct3D 12* nous fournit un wrapper C++ `CD3DX12_RESOURCE_DESC` qui dérive de `D3D12_RESOURCE_DESC` qui nous permet de plus facilement manipuler un `D3D12_RESOURCE_DESC` comme par exemple avec la méthode : 
```c++
static inline CD3DX12_RESOURCE_DESC Buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0)
{
    return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
}
// Pour un tampon, la largeur (width) correspond au nombre d'octets dans le tampon.
// Par exemple pour un buffer de 64 floats, le width = 64 * sizeof(float).
```
Pour de la géométrie statique (qui ne change pas chaque frame), on met les tampons de sommet dans le tas par défaut `D3D12_HEAP_TYPE_DEFAULT` pour des questions de perfomances.

En plus de la création de la ressource du tampon de sommet, on doit aussi créer un *upload* buffer avec le type de tas `D3D12_HEAP_TYPE_UPLOAD` car on alloue une ressource au tas d'*upload* quand on a besoin de copier des données du CPU vers la mémoire du GPU. Après la création de ce tampon d'*upload*, on copie les données des sommets vers ce tampon puis on copie les données des sommets vers le tampon de sommet. Comme on a besoin d'un tampon intermédiaire pour initialiser les données d'un *default buffer*, on peut créer une fonction qui nous permet de faire le travail : 
```c++
ComPtr<ID3D12Resource> Utils::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Ici on créé le default buffer.
    ThrowIfFailed(
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
            D3D12_HEAP_FLAG_NONE, 
            &CD3DX12_RESOURCE_DESC::Buffer(byteSize), 
            D3D12_RESOURCE_STATE_COMMON, 
            nullptr, 
            IID_PPV_ARGS(defaultBuffer.GetAddressOf())
        )
    );

    // ici on créé le tas d'upload intermédiaire.
    ThrowIfFailed(
        device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(uploadBuffer.GetAddressOf())
        )
    );

    // On décrit les données que l'on veut copier dans le default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Ici, on programme une copie des données vers le default buffer.
    // On utilise la fonction d'aide UpdateSubresource qui copiera depuis la mémoire CPU vers le tas d'upload intermédiaire.
    // Puis on utilise ID3D12CommandList::CopySubresourceRegion pour copier ce qu'il y a dans le tas d'upload intermédiaire vers mbuffer
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Attention : uploadBuffer doit être maintenu en vie après l'appel à cette fonction parce que la commandlist n'a pas encore exécuté la copie. 
    // L'appelant peut libérer uploadVBuffer après qu'il sait que la copie s'est exécutée.
    return defaultBuffer;
}

typedef struct D3D12_SUBRESOURCE_DATA
{
    const void *pData; // Pointeur vers de la mémoire système qui contient les données qui seront utilisées pour initialiser le buffer.
    LONG_PTR RowPitch; // Pour des tampons, la taille des données que l'on copie en octets.
    LONG_PTR SlicePitch; // Pour des tampons, la taille des données que l'on copie en octets.
} D3D12_SUBRESOURCE_DATA;
```
Et voici un petit exemple en utilisant la méthode ci-dessus pour créer un tampon avec 8 sommets pour former un cube avec pour chaque sommet une couleur qui lui est associée.
```c++
Vertex vertices[] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   },
    { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
    { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
    { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
    { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)    },
    { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
    { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)    },
    { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) },
};
const UINT64 vbByteSize = 8 * sizeof(Vertex);
ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
VertexBufferGPU = Utils::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices, vbBytesSize, VertexBufferUploader);
```
Pour lier un vertex buffer à la pipeline de rendu, on doit créer une vue de tampon de sommet (*vertex buffer view*) vers la ressource de vertex buffer. On a pas besoin d'un descripteur de tas pour un *vertex buffer view* comme pour un RTV.
```c++
typedef struct D3D12_VERTEX_BUFFER_VIEW
{
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    UINT StrideInBytes;
} D3D12_VERTEX_BUFFER_VIEW;
```
1) `Bufferlocation` est l'adresse virtuelle de la ressource vertex buffer avec laquelle on veut créer une vue. On peut utiliser la méthode `ID3D12Resource::GetGPUVirtualAddress` pour l'obtenir.
2) `SizeInBytes` est le nombre d'octets à prendre en compte dans le vertex buffer en commençant à *BufferLocation*.
3) `StrideInBytes` est la taille en octets de chaque sommet.

Après qu'un vertex buffer a été créé et qu'on a la *view* associée, on peut la lier à un *input slot* de la pipeline pour fournir les sommets dans l'étape d'*Input Assembler*. On le fait grâce à la méthode : 
```c++
void ID3D12GraphicsCommandList::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, const D3D12_VERTEX_BUFFER_VIEW *pViews);
```
1) `StartSlot` est le slot d'entrée pour commencer à lier les vertex buffers. Il y a 16 slots d'entrée indexées de 0 à 15.
2) `NumBuffers` le nombre de vertex buffers que l'on lie aux slots d'entrées.
3) `pViews` est le pointeur vers le premier élément d'un tableau de *vertex buffer views*.

Un petit exemple : 
```c++
D3D12_VERTEX_BUFFER_VIEW vbv;
vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
vbv.StrideInBytes = sizeof(Vertex);
vbv.SizeInBytes = 8 * sizeof(Vertex);
D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);
```
Un vertex buffer restera lié à un slot d'entrée jusqu'à qu'on le change. Attention, définir le vertex buffer pour un slot d'entrée ne va pas dessiner les sommets, cela permet simplement de dire que le sommets sont prêts à être donner à la pipeline. Pour dessiner les sommets on utilise : 
```c++
void ID3D12GraphicsCommandList::DrawInstance(
    UINT VertexCountPerInstance,
    UINT InstanceCount,
    UINT StartVertexLocation,
    UINT StartInstanceLocation
);
```
1) `VertexCountPerInstance` est le nombre de sommets à dessiner (par instance).
2) `InstanceCount` est le nombre d'instances à dessiner, mais pour le moment on mettra 1 ici.
3) `StartVertexLocation` est l'index du premier sommet à dessiner.
4) `StartInstanceLocation` est l'index de la première instance à dessiner, mais pour le moment on mettra 0 ici.

Les deux paramètres *VertexCountPerInstance* et *StartVertexLocation* définissent un sous-ensemble continu de sommets à dessiner à partir du vertex buffer. Attention, ne pas oublier de définir comment sont dessiner les primitives avec la méthode `ID3D12GraphicsCommandList::IASetPrimitiveTopology`.

## Index Buffer
Similairement aux sommets, pour que le GPU ait accès à un tableau d'indices, ils doivent être placés dans un buffer GPU ressource (*ID3D12Resource*), on appelle ce buffer qui stocke des indices un **index buffer**. On peut réutiliser la fonction `Utils::CreateDefaultBuffer` pour créer un index buffer. Pour lier un index buffer à la pipeline de rendu, on doit d'abord créer une vue d'index buffer qui est représentée par : 
```c++
typedef struct D3D12_INDEX_BUFFER_VIEW
{
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    DXGI_FORMAT Format;
} D3D12_INDEX_BUFFER_VIEW;
```
1) `BufferLocation` est l'adresse virtuelle de la ressource index buffer avec laquelle on veut créer une vue. On peut obtenir cette adresse grâce à la méthode `ID3D12_Resource::GetGPUVirtualAddress()`
2) `SizeInBytes` est le nombre d'octets dans l'index buffer en commençant à partir de *BufferLocation*
3) `Format` est le format des indices qui doit être soit `DXGI_FORMAT_R16_UINT` pour des indices sur 16 bits ou `DXGI_FORMAT_R32_UINT` pour des indices sur 32 bits.

On lie un index buffer à l'étape d'*Input Assembler* avec la méthode `ID3D12CommandList::SetIndexBuffer`. Voici un petit exemple pour définir les triangles d'un cube, créer une vue et la lier à la pipeline : 
```c++
std::uint16_t indices[] = {
    0, 1, 2,   0, 2, 3 // face avant
    4, 6, 5,   4, 7, 6 // face arrière
    4, 5, 1,   4, 1, 0 // face gauche
    3, 2, 6,   3, 6, 7 // face droite
    1, 5, 6,   1, 6, 2 // face du haut
    4, 0, 3,   4, 3, 7 // face du dessous
};
const UINT ibByteSize = 36 * sizeof(std::uint16_t);
ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;
IndexBufferGPU = Utils::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices, ibByteSize, IndexBufferUploader);

D3D12_INDEX_BUFFER_VIEW ibv;
ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
ibv.Format = DXGI_FORMAT_R16_UINT;
ibv.SizeInBytes = ibByteSize;
mCommandList->IASetIndexBuffer(&ibv);
```
Et enfin lorsqu'on utilise des indices, on doit utilise la méthode (à la place de la méthode `ID3D12GraphicsCommandList::DrawInstance`): 
```c++
void ID3D12GraphicsCommandList::DrawIndexedInstanced(
    UINT IndexCountPerInstance,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT BaseVertexLocation,
    UINT StartInstanceLocation
);
```
1) `IndexCountPerInstance` est le nombre d'indices à dessiner (par instance).
2) `InstanceCount` est le nombre d'instances à dessiner, mais pour le moment on mettra 1 ici.
3) `StartIndexLocation` est l'indice vers un élément qui marque le point d'entrée pour le début de la lecture des indices.
4) `BaseVertexLocation` est un entier qui sera ajouté à chaque indice utilisé lors de l'appel de dessin avant que les sommets soient récupérés.
5) `StartInstanceLocation` est l'indice de la première instance à dessiner, mais pour le moment on mettra 0 ici.

## Vertex shader
Voici un exemple d'un vertex shader basique écrit en HLSL (High Level Shading Language): 
```hlsl
// Buffer constant
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

void VS(float3 iPosL : POSITION, float4 iColor : COLOR, out float4 oPosH : SV_POSITION, out float4 oColor : COLOR)
{
    // On transforme vers l'espace d'homogeneous clip
    oPosH = mul(float4(iPosL, 1.0f), gWorldViewProj);

    // On passe la couleur au pixel shader
    oColor = iColor;
}
```
Ici on peut voir que le vertex shader est la fonction qui se nomme `VS`, on peut lui donner n'importe quel nom valide. Ce vertex shader prend 4 paramètres, deux en entrée et deux en sortie. Les deux premiers paramètres forment la signature d'entrée du vertex shader et correspondent aux membres de notre structure de sommet. Les paramètres sémantiques `: POSITION` et `: COLOR` sont utilisés pour mapper les éléments de la structure de sommet aux paramètres d'entrée du vertex shader. 
```c++
struct Vertex {
    XMFLOAT3 Pos; // 1
    XMFLOAT4 Color; // 2
};

D3D12_INPUT_ELEMENT_DESC vertexDesc[] = { ... };

void VS(
    float3 iPosL : POSITION // 1
    float4 iColor : COLOR // 2
    ...
)
```
Pour les paramètres de sortie, on y a aussi attaché des sémantiques `: SV_POSITION` et `: COLOR`, ils sont utilisés pour mapper les sorties du vertex shader vers les entrées de la prochaine étape (soit le geometry shader, soit le pixel shader). À noter que `SV_POSITION` est une sémantique spéciale (SV = system value), c'est utilisé pour désigner l'élément de sortie du vertex shader qui contient la position du sommet dans l'espace homogeneous clip. On doit attacher cette sémantique parce que le GPU doit connaitre cette valeur car elle intervient dans des opérations auxquelles les autres attributs ne participent pas comme le clipping, test de profondeur et la raatérisation. On peut réécrire le vertex shader en utilisant des structures pour la sortie comme pour l'entrée : 
```hlsl
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR:
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.Color = vin.Color;
    
    return vout;
}
```

## Pixel shader
Pendant la rasterisation, les attributs de sommet générés par le vertex shader (ou le geometry shader) sont interpolés entre les pixels d'un triangle. Ces valeurs interpolées sont ensuite données au pixel shader en tant qu'entrées. Voici un exemple d'un pixel shader basique : 
```hlsl
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

float4 PS(VertexOut pin) : SV_TARGET
{
    return pin.Color;
}
```
Il faut noter que les entrées du pixel shader doivent matchées **exactement** les sorties du vertex shader. Le pixel shader ici renvoie une valeur de couleur en 4D.

## Constant buffers
Un tampon constant est une ressource GPU (`ID3D12Resource`) dont ses données peuvent être référencées dans un programme de shader.
```hlsl
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};
```
Ce code permet de définir un objet de type `cbuffer` nommé `cbPerObject`. Ici ce buffer stocke une matrice 4x4 nommé `gWorldViewProj` qui représente les matrices combinées world, view et proj utilisées pour transformer un point de l'espace local en espace homogeneous clip. Contrairement aux vertex buffer et index buffer, les buffers constant sont généralement mis à jour une fois par frame par le CPU. Par conséquent, on crée les buffers constant dans un tas d'*upload* plutôt que dans le tas default, on pourra alors mettre à jour le contenu depuis le CPU. On aura souvent besoin de plusieurs buffers constant d'un même type. Par exemple, pour le buffer `cbOerObject` qui stocke des constantes qui varient par objet, si on a *n* objet on aura alors besoin de *n* buffers constant. Voici un exemple de code qui nous permet de créer un buffer qui stocke `NumElements` buffers constant : 
```c++
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 WorldViewProj = MathUtils::Identity4x4();
};

UINT elementByteSize = Utils::CalcConstantBufferByteSize(sizeof(ObjectConstants));
ComPtr<ID3D12Resource> uploadCBuffer;
device->CreateCommitedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOUCE_DESC::Buffer(elementByteSize * NumElements),
    D3D12_RESOURCE_STATE_GENERRIC_READ,
    nullptr,
    IID_PPV_ARGS(&uploadCBuffer)
);

UINT Utils::CalcConstantBufferByteSize(UINT byteSize)
{
    // Les buffer constant doivent être multiple de la taille minimal d'allocation matérielle (généralement 256) donc on arrondi au multiple 256 du dessus.
    return (byteSize + 255) & ~255;
}
```
Avec *DirectX12* qui a introduit le shader model 5.1, il existe une syntaxe alternative pour définir un buffer constant : 
```hlsl
struct ObjectConstants
{
    float4x4 gWorldViewProj;
    uint matIndex;
};
ConstantBuffer<OBjectConstants> gObjConstants : register(b0);
```
On peut mettre à jour les buffers constant mais pour ce faire on doit d'abord obtenir un pointeur vers les données de ressource qui peut être fait : 
```c++
ComPtr<ID3D12Resource> uploadBuffer;
BYTE* mappedData = nullptr;
uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(mappedData));
```
Le premier paramètre est un indice de sous-ressource qui identifie la sous ressource à mapper. Pour un buffer, la seule sous ressource c'est le buffer lui même donc on met 0. Le second paramètre est un pointeur optionnel vers une structure `D3D12_RANGE` qui décrit la portée de la mémoire à mapper, si on spécifie *null* alors on map toute la ressource. Le dernier paramètre retourne un pointeur vers les données mappées. Enfin, pour copier des données depuis la mémoire de notre système vers le buffer constant on peut simplement utiliser : 
```c++
memcpy(mappedData, &data, dataSizeInBytes);

// Quand on a fini avec le buffer constant : 
if (uploadBuffer != nullptr)
    uploadBuffer->Unmap(0, nullptr);
```
Il peut être utile de construire un *wrapper* pour nous aider à manipuler les *upload* buffer.
```c++
template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) 
        : mIsConstantBuffer(isConstantBuffer) 
    {
        mElementByteSize = sizeof(T);

        if (isConstantBuffer)
            mElementByteSize = Utils::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(
            device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&mUploadBuffer)
            )
        );

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource() const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};
```
Pour rappel, on lie une ressoruce à la pipeline grâce à un objet descripteur. On a donc aussi besoin de descripteurs pour lier les buffer constants à la pipeline. Les descripteurs de buffer constant vivent dans un tas de descripteur de type `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV`. Pour stocker ces nouveaux types de descripteurs on a besoin de créer un nouveau tas de descriptor de ce type : 
```c++
D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
cbvHeapDesc.NumDescriptor = 1;
cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; 
// Ce flag nous permet de dire que ces descripteurs seront utilisés par des programmes de shader 
cbvHeapDesc.NodeMask = 0;
ComPtr<ID3D12DescriptorHeap> cbvHeap;
d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap));
```
Pour créer une vue d'un constant buffer on peut faire : 
```c++
struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathUtils::Identity4x4();
};

// Buffer constant pour stocker les constantes de *n* objets
std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), n, true);

UINT objCBByteSize = Utils::CalcConstantBufferByteSize(sizeof(ObjectConstants));

// Adresse de début de la mémoire du buffer
D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

// Offset vers le ième objet constant buffer dans le buffer
int boxCBufIndex = i;
cbAddress += boxCBufIndex * objCBByteSize;

D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
cbvDesc.BufferLocation = cbAddress;
cbvDesc.SizeInBytes = Utils::CalcConstantBufferByteSize(sizeof(ObjectConstants));
md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
```
La structure `D3D12_CONSTANT_BUFFER_VIEW_DESC` décrit un sous-ensemble de la ressource du buffer constant à lier à la structure du buffer constant de celui dans le HLSL.

Généralement, différent programmes de shader s'attendent à ce que différentes ressources soient liées à la pipeline de rendu avant qu'un appel de dessin soit exécuté. Les ressources sont liées à des slots de registres ou elles peuvent être accédées par les programmes de shader. Dans l'exemple précédent on a définit le registre du buffer constant par `b0`. La signature racine (*root signature*) définit les ressources que l'application va lier à la pipeline de rendu avant qu'un appel de dessin puisse être exécuté et l'endroit où ces ressource sont mappées aux registres d'entrée du shader. Une root signature est représenté par l'interface `ID3D12RootSignature`. Elle est définie par un tableau de paramètres racines qui décrivent les ressources que le shader attend pour un draw call. Un paramètre racine peut être une constantes, descripteur ou même une table de descripteur. Le code suivant crée une root signature qui a un paramètre racine qui est une table de descripteur assez grande pour y stocker un CBV (*constant buffer view*) : 
```c++
CD3DX12_ROOT_PARAMETER slotRootParameter[1];
CD3DX12_DESCRIPTOR_RANGE cbvTable;
cbvTable.Init(
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
    1, // Nombre de descripteur dans la table
    0 // Arguments du registre du shader de base qui sont liés à ce paramètre 
);
slotRootParameter[0].InitAsDescriptorTable(
    1, // Nombre de "portée" (range)
    &cbvTable // Pointeur vers un tableau de "portée" (range)
);

CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

ComPtr<ID3DBlob> serializedRootSig = nullptr;
ComPtr<ID3DBlob> errorBlob = nullptr;
HRESULT hr = D3D12SerializeRootSignature(
    &rootSigDesc,
    D3D_ROOT_SIGNATURE_VERSION_1,
    serializedRootSig.GetAddressOf(),
    errorBlob.GetAddressOf()
);

ThrowIfFailed(
    md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)
    )
);
```
La signature root définit seulement quelles ressource l'application va lier à la pipeline de rendu, elle ne fait pas de liaison en soit. Une fois que la root signature a été définit grâce à une *command list*, on peut s'en servir grâce à la méthode : 
```c++
ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable(
    UINT RootParameterIndex,
    D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor 
);
```
1) `RootParameterIndex` est l'indice du paramètre root que l'on veut utiliser.
2) `BaseDescriptor` est un *handle* vers un descripteur dans le tas qui spécifie le premier descripteur dans la table à utiliser.

Une fois que la root signature a été créé, on peut alors l'utiliser de la manière suivante : 
```c++
mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
mCommandList->SetGraphicsRootDescriptorTable(0, cbv);
```

## Compilation des shaders
Avec *DirectX* (ou d'autre API graphique) les programmes de shader doivent être compilés. Au runtime, on peut compiler un shader avec la méthode : 
```c++
HRESULT D3DCompileFromFile(
    LPCWSTR pFileName,
    const D3D_SHADER_MACRO *pDefines,
    ID3DInclude *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob **ppCode,
    ID3DBlob **ppErrorMsgs
);
```
1) `pFileName` est le nom du fichier .hlsl qui contient le source code du shader.
2) `pDefines` est une option avancée qu'on n'utilise pas pour le moment.
3) `pInclude` est une option avancée qu'on n'utilise pas pour le moment.
4) `pEntrypoint` est le nom de la fonction d'entrée du shader, un fichier .hlsl peut contenir plusieurs programmes de shader (par exemple un VS et un PS).
5) `pTarget` est une chaîne de caractère qui spécifie le type du programme de shader et la version : 
    - *vs_5_0* / *vs_5_1* pour un vertex shader version 5.0 ou 5.1
    - *ps_5_0* / *ps_5_1* pour un pixel shader version 5.0 ou 5.1
    - *gs_5_0* / *gs_5_1* pour un geometry shader version 5.0 ou 5.1
    - *hs_5_0* / *hs_5_1* pour un hull shader version 5.0 ou 5.1
    - *ds_5_0* / *ds_5_1* pour un domain shader version 5.0 ou 5.1
    - *cs_5_0* / *cs_5_1* pour un compute shader version 5.0 ou 5.1
6) `Flags1` sont les flags pour spécifier comment le shader doit être compilé par exemple : 
    - *D3DCOMPILE_DEBUG* pour compiler le shader en mode débug.
    - *D3DCOMPILE_SKIP_OPTIMIZATION*
7) `Flags2` sont les flags pour des effets avancés de la compilation qu'on n'utilise pas pour le moment.
8) `ppCode` retourne un pointeur vers une structure `ID3DBlob` qui stocke le shader bytecode compilé.
9) `ppErrorMsgs` retourne un pointeur vers une structure `ID3DBlob` qui stocke une chaîne de caractères qui contient les erreurs de compilations s'il y en a.

Le type `ID3DBlob` n'est juste qu'un bloc mémoire générique qui a deux méthodes : 
```c++
// Retourne un void* vers les données. 
LPVOID ID3DBlob::GetBufferPointer();

// Retourne la taille en octets du buffer.
SIZE_T ID3DBlob::GetBufferSize();
```
On peut implémenter une fonction qui nous permet de compiler et d'afficher les erreurs s'il y en lors de la compilation : 
```c++
ComPtr<ID3DBlob> Utils::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target) 
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = S_OK;
    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(
        filename.c_str(), 
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), 
        target.c_str(), 
        compileFlags,
        0,
        &byteCode, 
        &errors
    );

    if(errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());
    ThrowIfFailed(hr);
    return byteCode;
}
```
Avec un petit exemple d'appel à cette fonction : 
```c++
ComPtr<ID3DBlob> mvsByteCode = nullptr;
ComPtr<ID3DBlob> mpsByteCode = nullptr;
mvsByteCode = Utils::CompileShader(L"Shaders\color.hlsl", nullptr, "VS", "vs_5_0");
mpsByteCode = Utils::CompileShader(L"Shaders\color.hlsl", nullptr, "PS", "ps_5_0");
```
On peut aussi compiler des shaders de manière *offline* à savoir lors du build de l'application ou un autre moment. Pour ce faire, on utilise l'outil *FXC* qui est fournit avec *DirectX* dont on se sert en ligne de commande. Par exemple pour compiler un vertex et un pixel shader dans un fichie `color.hlsl` avec les noms de fonctions d'entrée `VS` et `PS` on peut faire : 
```bat
:: /Od permet de désactiver les optimisations
:: /Zi permet d'activer les informations de debug
:: /T <string> permet de spécifier le type de shader et sa version
:: /E <string> permet de spécifier le point d'entrée
:: /Fo <string> permet de spécifier le fichier de sortie du bytecode
:: /Fc <string> permet de spécifier le fichier de sortie de l'assemblage

:: En mode debug
fxc "color.hlsl" /Od /Zi /T vs_5_0 /E "VS" /Fo "color_vs.cso" /Fc "color_vs.asm"
fxc "color.hlsl" /Od /Zi /T ps_5_0 /E "PS" /Fo "color_ps.cso" /Fc "color_ps.asm"

:: En mode release
fxc "color.hlsl" /T vs_5_0 /E "VS" /Fo "color_vs.cso" /Fc "color_vs.asm"
fxc "color.hlsl" /T ps_5_0 /E "PS" /Fo "color_ps.cso" /Fc "color_ps.asm"
```
On a donc compilé nos shaders et généré les fichiers de bytecode mais on doit tout de même au runtime lire ces bytecode on peut donc le faire de la manière suivante : 
```c++
ComPtr<ID3DBlob> Utils::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);
    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);
    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));
    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();
    return blob;
}

// ...
ComPtr<ID3DBlob> mvsByteCode = Utils::LoadBinary(L"Shaders\color_vs.cso");
ComPtr<ID3DBlob> mpsByteCode = Utils::LoadBinary(L"Shaders\color_ps.cso");
```