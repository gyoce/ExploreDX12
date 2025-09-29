#pragma once

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <random>

using namespace DirectX;

namespace DirectXMathUtils
{
    inline const float Pi = 3.1415926535f;

    inline XMFLOAT4X4 Identity4x4()
    {
        static XMFLOAT4X4 I {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        return I;
    }

    template<typename T>
    inline T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    inline XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
    {
        return XMVectorSet(
            radius * sinf(phi) * cosf(theta),
            radius * cosf(phi),
            radius * sinf(phi) * sinf(theta),
            1.0f
        );
    }

    inline std::random_device rd;
    inline std::mt19937 gen(rd());

    inline float Randf(float a, float b)
    {
        std::uniform_real_distribution<float> dis(a, b);
        return dis(gen);
    }

    inline int Rand(int a, int b)
    {
        std::uniform_int_distribution<int> dis(a, b);
        return dis(gen);
    }
}