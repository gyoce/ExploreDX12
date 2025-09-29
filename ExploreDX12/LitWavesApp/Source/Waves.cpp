#include "Waves.h"

#include <ppl.h>

#include "Managers/TimeManager.h"

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
    : mNumberOfRows(m), mNumberOfColumns(n), mVertexCount(m * n), mTriangleCount((m - 1) * (n - 1) * 2), mTimeStep(dt), mSpatialStep(dx)
{
    float d = damping * dt + 2.0f;
    float e = (speed * speed) * (dt * dt) / (dx * dx);
    mK1 = (damping * dt - 2.0f) / d;
    mK2 = (4.0f - 8.0f * e) / d;
    mK3 = (2.0f * e) / d;

    mPreviousSolution.resize(m * n);
    mCurrentSolution.resize(m * n);
    mNormals.resize(m * n);
    mTangentX.resize(m * n);

    // Ici, on génère les sommets de la grille.
    float halfWidth = (n - 1) * dx * 0.5f;
    float halfDepth = (m - 1) * dx * 0.5f;
    for(int i = 0; i < m; i++)
    {
        float z = halfDepth - i * dx;
        for (int j = 0; j < n; j++)
        {
            float x = -halfWidth + j * dx;
            mPreviousSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            mCurrentSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            mNormals[i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            mTangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
        }
    }
}

void Waves::Update()
{
    static float t = 0.0f;
    t += TimeManager::GetDeltaTime();
    if (t < mTimeStep)
        return;

    Concurrency::parallel_for(1, mNumberOfRows - 1, [this](int i)
    {
        for (int j = 0; j < mNumberOfColumns - 1; j++)
        {
            mPreviousSolution[i * mNumberOfColumns + j].y =
                mK1 * mPreviousSolution[i * mNumberOfColumns + j].y +
                mK2 * mCurrentSolution[i * mNumberOfColumns + j].y +
                mK3 * (
                    mCurrentSolution[(i + 1) * mNumberOfColumns + j].y +
                    mCurrentSolution[(i - 1) * mNumberOfColumns + j].y +
                    mCurrentSolution[i * mNumberOfColumns + j + 1].y +
                    mCurrentSolution[i * mNumberOfColumns + j - 1].y
                    );
        }
    });

    std::swap(mPreviousSolution, mCurrentSolution);

    t = 0.0f;

    Concurrency::parallel_for(1, mNumberOfRows - 1, [this](int i)
    {
        for (int j = 1; j < mNumberOfColumns - 1; ++j)
        {
            float l = mCurrentSolution[i * mNumberOfColumns + j - 1].y;
            float r = mCurrentSolution[i * mNumberOfColumns + j + 1].y;
            float t = mCurrentSolution[(i - 1) * mNumberOfColumns + j].y;
            float b = mCurrentSolution[(i + 1) * mNumberOfColumns + j].y;
            mNormals[i * mNumberOfColumns + j].x = -r + l;
            mNormals[i * mNumberOfColumns + j].y = 2.0f * mSpatialStep;
            mNormals[i * mNumberOfColumns + j].z = b - t;

            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[i * mNumberOfColumns + j]));
            XMStoreFloat3(&mNormals[i * mNumberOfColumns + j], n);

            mTangentX[i * mNumberOfColumns + j] = XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
            XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[i * mNumberOfColumns + j]));
            XMStoreFloat3(&mTangentX[i * mNumberOfColumns + j], T);
        }
    });
}

void Waves::Disturb(int i, int j, float magnitude)
{
    // Ne pas troubler les frontières / bordures
    assert(i > 1 && i < mNumberOfRows - 2);
    assert(j > 1 && j < mNumberOfColumns - 2);

    float halfMagnitude = 0.5f * magnitude;

    // On trouble la hauteur du i/j ème sommet et ses voisins.
    mCurrentSolution[i * mNumberOfColumns + j].y += magnitude;
    mCurrentSolution[i * mNumberOfColumns + j + 1].y += halfMagnitude;
    mCurrentSolution[i * mNumberOfColumns + j - 1].y += halfMagnitude;
    mCurrentSolution[(i + 1) * mNumberOfColumns + j].y += halfMagnitude;
    mCurrentSolution[(i - 1) * mNumberOfColumns + j].y += halfMagnitude;
}
