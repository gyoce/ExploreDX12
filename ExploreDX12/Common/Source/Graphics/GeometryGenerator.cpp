#include "Graphics/GeometryGenerator.h"

namespace GeometryGenerator
{
    void Subdivide(MeshData& meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
    void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData);
    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData);

    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount)
    {
        MeshData meshData;
        float stackHeight = height / stackCount;
        float radiusStep = (topRadius - bottomRadius) / stackCount; // Δr
        std::uint32_t ringCount = stackCount + 1;

        // On calcule les sommets pour chaque anneau en partant du bas et en allant vers le haut.
        for (std::uint32_t i = 0; i < ringCount; i++)
        {
            float y = -0.5f * height + i * stackHeight;
            float r = bottomRadius + i * radiusStep;
            float dTheta = 2.0f * XM_PI / sliceCount;
            for (std::uint32_t j = 0; j <= sliceCount; j++)
            {
                Vertex vertex;
                float c = cosf(j * dTheta);
                float s = sinf(j * dTheta);
                vertex.Position = XMFLOAT3(r * c, y, r * s);
                vertex.TexC.x = (float)j / sliceCount;
                vertex.TexC.y = 1.0f - (float)i / stackCount;
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
        std::uint32_t ringVertexCount = sliceCount + 1;

        // Pour chaque segments, on calcule les indices.
        for (std::uint32_t i = 0; i < stackCount; i++)
        {
            for (std::uint32_t j = 0; j < sliceCount; j++)
            {
                meshData.Indices32.push_back(i       * ringVertexCount + j    );
                meshData.Indices32.push_back((i + 1) * ringVertexCount + j    );
                meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
                meshData.Indices32.push_back(i       * ringVertexCount + j    );
                meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
                meshData.Indices32.push_back(i       * ringVertexCount + j + 1);
            }
        }

        BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
        BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
        return meshData;
    }

    MeshData CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount)
    {
        MeshData meshData;

        // Calcule les sommets en partant du pôle supérieur et en descendant les segments.

        // Pour les poles il faut noter qu'il y aura une distorsion des coordonnées de texture car il n'y a pas de point unique sur la carte de texture à assigner au pôle lorsqu'on mappe une texture rectangulaire sur une sphère.
        Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        meshData.Vertices.push_back(topVertex);

        float phiStep = XM_PI / stackCount;
        float thetaStep = 2.0f * XM_PI / sliceCount;

        // Calcule des sommets pour chaque anneau de segment (on ne compte pas les pôles comme des anneaux).
        for (std::uint32_t i = 1; i <= stackCount - 1; i++)
        {
            float phi = i * phiStep;

            // Pour les sommets de l'anneau.
            for (std::uint32_t j = 0; j <= sliceCount; j++)
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
        for (std::uint32_t i = 1; i <= sliceCount; i++)
        {
            meshData.Indices32.push_back(0);
            meshData.Indices32.push_back(i + 1);
            meshData.Indices32.push_back(i);
        }

        // Calcule des indices pour les segments intérieurs.
        // Décale les indices à l'indice du premier sommet du premier anneau. (On saute juste le sommet du pôle supérieur.)
        std::uint32_t baseIndex = 1;
        std::uint32_t ringVertexCount = sliceCount + 1;
        for (std::uint32_t i = 0; i < stackCount - 2; i++)
        {
            for (std::uint32_t j = 0; j < sliceCount; ++j)
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
        std::uint32_t southPoleIndex = (std::uint32_t)meshData.Vertices.size() - 1;
        // Décale les indices à l'indice du premier sommet du dernier anneau.
        baseIndex = southPoleIndex - ringVertexCount;
        for (std::uint32_t i = 0; i < sliceCount; ++i)
        {
            meshData.Indices32.push_back(southPoleIndex);
            meshData.Indices32.push_back(baseIndex + i);
            meshData.Indices32.push_back(baseIndex + i + 1);
        }

        return meshData;
    }

    MeshData CreateGeosphere(float radius, std::uint32_t numSubdivisions)
    {
        MeshData meshData;
        // On met une limite sur le nombre de subdivisions.
        numSubdivisions = std::min<std::uint32_t>(numSubdivisions, 6u);
        // Approximation d'une sphère en tesselant un icosaèdre.
        const float X = 0.525731f;
        const float Z = 0.850651f;
        XMFLOAT3 pos[12] = {
            XMFLOAT3(-X  , 0.0f, Z   ), XMFLOAT3(X   , 0.0f, Z   ),
            XMFLOAT3(-X  , 0.0f, -Z  ), XMFLOAT3(X   , 0.0f, -Z  ),
            XMFLOAT3(0.0f, Z   , X   ), XMFLOAT3(0.0f, Z   , -X  ),
            XMFLOAT3(0.0f, -Z  , X   ), XMFLOAT3(0.0f, -Z  , -X  ),
            XMFLOAT3(Z   , X   , 0.0f), XMFLOAT3(-Z  , X   , 0.0f),
            XMFLOAT3(Z   , -X  , 0.0f), XMFLOAT3(-Z  , -X  , 0.0f),
        };
        std::uint32_t k[60] = {
            1, 4 ,0,   4,9,0,  4,5 ,9,  8,5,4 ,  1 ,8,4,
            1, 10,8,  10,3,8,  8,3 ,5,  3,2,5 ,  3 ,7,2,
            3, 10,7,  10,6,7,  6,11,7,  6,0,11,  6 ,1,0,
            10,1 ,6,  11,0,9,  2,11,9,  5,2,9 ,  11,2,7
        };
        meshData.Vertices.resize(12);
        meshData.Indices32.assign(&k[0], &k[60]);

        for (std::uint32_t i = 0; i < 12; ++i)
            meshData.Vertices[i].Position = pos[i];

        for (std::uint32_t i = 0; i < numSubdivisions; ++i)
            Subdivide(meshData);

        // Projection des sommets sur la sphère et mise à l'échelle.
        for (std::uint32_t i = 0; i < meshData.Vertices.size(); ++i)
        {
            // Projection sur la sphère unité.
            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));
            XMVECTOR p = radius * n;
            XMStoreFloat3(&meshData.Vertices[i].Position, p);
            XMStoreFloat3(&meshData.Vertices[i].Normal, n);
            // Calcul des coordonnées de texture à partir des coordonnées sphériques.
            float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);
            // Mise dans l'intervalle [0, 2pi].
            if (theta < 0.0f)
                theta += XM_2PI;

            float phi = acosf(meshData.Vertices[i].Position.y / radius);
            meshData.Vertices[i].TexC.x = theta / XM_2PI;
            meshData.Vertices[i].TexC.y = phi / XM_PI;
            // Dérivée partielle de P par rapport à theta
            meshData.Vertices[i].TangentU.x = -radius * sinf(phi) * sinf(theta);
            meshData.Vertices[i].TangentU.y = 0.0f;
            meshData.Vertices[i].TangentU.z = +radius * sinf(phi) * cosf(theta);
            XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
            XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
        }

        return meshData;
    }

    MeshData CreateBox(float width, float height, float depth, std::uint32_t numSubdivisions)
    {
        MeshData meshData;

        float w2 = 0.5f * width;
        float h2 = 0.5f * height;
        float d2 = 0.5f * depth;

        // Création des sommets.
        Vertex v[24];
        
        // Face avant.
        v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Face arrière.
        v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Face du haut.
        v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Face du bas.
        v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Face de gauche.
        v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
        v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
        v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
        v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

        // Face de droite.
        v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

        meshData.Vertices.assign(&v[0], &v[24]);

        // Création des indices.
        std::uint32_t i[36];

        // Indices de la face avant.
        i[0] = 0; i[1] = 1; i[2] = 2;
        i[3] = 0; i[4] = 2; i[5] = 3;

        // Indices de la face arrière.
        i[6] = 4; i[7] = 5; i[8] = 6;
        i[9] = 4; i[10] = 6; i[11] = 7;

        // Indices de la face du haut.
        i[12] = 8; i[13] = 9; i[14] = 10;
        i[15] = 8; i[16] = 10; i[17] = 11;

        // Indices de la face du bas.
        i[18] = 12; i[19] = 13; i[20] = 14;
        i[21] = 12; i[22] = 14; i[23] = 15;

        // Indices de la face de gauche.
        i[24] = 16; i[25] = 17; i[26] = 18;
        i[27] = 16; i[28] = 18; i[29] = 19;

        // Indices de la face de droite.
        i[30] = 20; i[31] = 21; i[32] = 22;
        i[33] = 20; i[34] = 22; i[35] = 23;

        meshData.Indices32.assign(&i[0], &i[36]);

        // On limite le nombre de subdivision.
        numSubdivisions = std::min<std::uint32_t>(numSubdivisions, 6u);

        for (std::uint32_t i = 0; i < numSubdivisions; ++i)
            Subdivide(meshData);

        return meshData;
    }

    MeshData CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n)
    {
        MeshData meshData;

        std::uint32_t vertexCount = m * n;
        std::uint32_t faceCount = (m - 1) * (n - 1) * 2;

        float halfWidth = 0.5f * width;
        float halfDepth = 0.5f * depth;

        float dx = width / (n - 1);
        float dz = depth / (m - 1);

        float du = 1.0f / (n - 1);
        float dv = 1.0f / (m - 1);
        
        // Création des sommets.
        meshData.Vertices.resize(vertexCount);
        for (std::uint32_t i = 0; i < m; ++i)
        {
            float z = halfDepth - i * dz;
            for (std::uint32_t j = 0; j < n; ++j)
            {
                float x = -halfWidth + j * dx;

                meshData.Vertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
                meshData.Vertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                meshData.Vertices[i * n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

                // On étire la texture sur la grille.
                meshData.Vertices[i * n + j].TexC.x = j * du;
                meshData.Vertices[i * n + j].TexC.y = i * dv;
            }
        }

        
        // Création des indices.
        meshData.Indices32.resize(faceCount * 3); // 3 indices par face.

        // On itére sur chaque quad et on calcule les indices.
        std::uint32_t k = 0;
        for (std::uint32_t i = 0; i < m - 1; ++i)
        {
            for (std::uint32_t j = 0; j < n - 1; ++j)
            {
                meshData.Indices32[k] = i * n + j;
                meshData.Indices32[k + 1] = i * n + j + 1;
                meshData.Indices32[k + 2] = (i + 1) * n + j;

                meshData.Indices32[k + 3] = (i + 1) * n + j;
                meshData.Indices32[k + 4] = i * n + j + 1;
                meshData.Indices32[k + 5] = (i + 1) * n + j + 1;

                k += 6;
            }
        }

        return meshData;
    }

    Vertex MidPoint(const Vertex& v0, const Vertex& v1)
    {
        XMVECTOR p0 = XMLoadFloat3(&v0.Position);
        XMVECTOR p1 = XMLoadFloat3(&v1.Position);

        XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
        XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

        XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
        XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

        XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
        XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

        // On calcule les points médiants de tous les attributs. 
        // Les vecteurs doivent être normalisés puisque l'interpolation linéaire peut les rendre non unitaires.
        XMVECTOR pos = 0.5f * (p0 + p1);
        XMVECTOR normal = XMVector3Normalize(0.5f * (n0 + n1));
        XMVECTOR tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
        XMVECTOR tex = 0.5f * (tex0 + tex1);

        Vertex v;
        XMStoreFloat3(&v.Position, pos);
        XMStoreFloat3(&v.Normal, normal);
        XMStoreFloat3(&v.TangentU, tangent);
        XMStoreFloat2(&v.TexC, tex);

        return v;
    }

    void Subdivide(MeshData& meshData)
    {
        // On sauvegarde localement une copie de la géométrie d'entrée.
        MeshData inputCopy = meshData;

        meshData.Vertices.resize(0);
        meshData.Indices32.resize(0);

        //       v1
        //       *
        //      / \
    	//     /   \
    	//  m0*-----*m1
        //   / \   / \
    	//  /   \ /   \
    	// *-----*-----*
        // v0    m2     v2

        std::uint32_t numTris = (std::uint32_t)inputCopy.Indices32.size() / 3;
        for (std::uint32_t i = 0; i < numTris; ++i)
        {
            Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
            Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
            Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

            // On génère les points médians.
            Vertex m0 = MidPoint(v0, v1);
            Vertex m1 = MidPoint(v1, v2);
            Vertex m2 = MidPoint(v0, v2);

            // On ajoute la nouvelle géométrie (des sommets + indices).
            meshData.Vertices.push_back(v0); // 0
            meshData.Vertices.push_back(v1); // 1
            meshData.Vertices.push_back(v2); // 2
            meshData.Vertices.push_back(m0); // 3
            meshData.Vertices.push_back(m1); // 4
            meshData.Vertices.push_back(m2); // 5

            meshData.Indices32.push_back(i * 6 + 0);
            meshData.Indices32.push_back(i * 6 + 3);
            meshData.Indices32.push_back(i * 6 + 5);

            meshData.Indices32.push_back(i * 6 + 3);
            meshData.Indices32.push_back(i * 6 + 4);
            meshData.Indices32.push_back(i * 6 + 5);

            meshData.Indices32.push_back(i * 6 + 5);
            meshData.Indices32.push_back(i * 6 + 4);
            meshData.Indices32.push_back(i * 6 + 2);

            meshData.Indices32.push_back(i * 6 + 3);
            meshData.Indices32.push_back(i * 6 + 1);
            meshData.Indices32.push_back(i * 6 + 4);
        }
    }

    void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
    {
        std::uint32_t baseIndex = (std::uint32_t)meshData.Vertices.size();
        float y = 0.5f * height;
        float dTheta = 2.0f * XM_PI / sliceCount;
        // On duplique les sommets du capuchon parce que les coordonnées de texture et les normales sont différentes.
        for (std::uint32_t i = 0; i <= sliceCount; i++)
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
        std::uint32_t centerIndex = (std::uint32_t)meshData.Vertices.size() - 1;
        for (std::uint32_t i = 0; i < sliceCount; i++)
        {
            meshData.Indices32.push_back(centerIndex);
            meshData.Indices32.push_back(baseIndex + i + 1);
            meshData.Indices32.push_back(baseIndex + i);
        }
    }

    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount, MeshData& meshData)
    {
        std::uint32_t baseIndex = (std::uint32_t)meshData.Vertices.size();
        float y = -0.5f * height;
        float dTheta = 2.0f * XM_PI / sliceCount;

        for (std::uint32_t i = 0; i <= sliceCount; i++)
        {
            float x = bottomRadius * cosf(i * dTheta);
            float z = bottomRadius * sinf(i * dTheta);
            float u = x / height + 0.5f;
            float v = z / height + 0.5f;
            meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
        }

        meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

        std::uint32_t centerIndex = (std::uint32_t)meshData.Vertices.size() - 1;
        for (std::uint32_t i = 0; i < sliceCount; i++)
        {
            meshData.Indices32.push_back(centerIndex);
            meshData.Indices32.push_back(baseIndex + i);
            meshData.Indices32.push_back(baseIndex + i + 1);
        }
    }
}