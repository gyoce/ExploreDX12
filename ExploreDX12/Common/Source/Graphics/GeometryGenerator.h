#pragma once

#include "Graphics/D3DUtils.h"

namespace GeometryGenerator
{
    struct Vertex
    {
        Vertex() = default;
        Vertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT2& uv)
            : Position(p), Normal(n), TangentU(t), TexC(uv) { }
        Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
            : Position(px, py, pz), Normal(nx, ny, nz), TangentU(tx, ty, tz), TexC(u, v) { }

        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT3 TangentU;
        XMFLOAT2 TexC;
    };

    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<std::uint32_t> Indices32;

        std::vector<std::uint16_t>& GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); i++)
                    mIndices16[i] = static_cast<std::uint16_t>(Indices32[i]);
            }
            return mIndices16;
        }

    private:
        std::vector<std::uint16_t> mIndices16;
    };

    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount);
    MeshData CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
    MeshData CreateGeosphere(float radius, std::uint32_t numSubdivisions);
    MeshData CreateBox(float width, float height, float depth, std::uint32_t numSubdivisions);
    MeshData CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n);

} // GeometryGenerator