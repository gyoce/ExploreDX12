# Dessiner avec DirectX

[Page d'accueil](README.md)

Sommaire : 
- [Sommets et input layouts](#sommets-et-input-layouts)
- [Vertex Buffer](#vertex-buffer)

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
void ID3D12CommandList::DrawInstance(
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