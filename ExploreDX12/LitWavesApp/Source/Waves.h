#pragma once

#include "Graphics/DirectXMathUtils.h"
#include <vector>

class Waves
{
public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves& rhs) = delete;
    Waves& operator=(const Waves& rhs) = delete;
    ~Waves() = default;

    int RowCount() const { return mNumberOfRows; }
    int ColumnCount() const { return mNumberOfColumns; }
    int VertexCount() const { return mVertexCount; }
    int TriangleCount() const { return mTriangleCount; }
    float Width() const { return mNumberOfColumns * mSpatialStep; }
    float Depth() const { return mNumberOfRows * mSpatialStep; }

    const XMFLOAT3& Position(int i) const { return mCurrentSolution[i]; }
    const XMFLOAT3& Normal(int i) const { return mNormals[i]; }
    const XMFLOAT3& TangentX(int i) const { return mTangentX[i]; }

    void Update();
    void Disturb(int i, int j, float magnitude);

private:
    int mNumberOfRows = 0;
    int mNumberOfColumns = 0;
    int mVertexCount = 0;
    int mTriangleCount = 0;

    float mK1 = 0.0f;
    float mK2 = 0.0f;
    float mK3 = 0.0f;

    float mTimeStep = 0.0f;
    float mSpatialStep = 0.0f;

    std::vector<XMFLOAT3> mPreviousSolution;
    std::vector<XMFLOAT3> mCurrentSolution;
    std::vector<XMFLOAT3> mNormals;
    std::vector<XMFLOAT3> mTangentX;
};