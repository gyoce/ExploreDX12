#pragma once 

#include "Graphics/DirectXMathUtils.h"

constexpr int MaxLights = 16;

struct Light
{
    XMFLOAT3 Strength;  // Couleur de la lumière
    float FalloffStart; // Uniquement point lumineux et projecteurs
    XMFLOAT3 Direction; // Uniquement directionnelle et projecteurs
    float FalloffEnd;   // Uniquement point lumineux et projecteurs
    XMFLOAT3 Position;  // Uniquement point lumineux et projecteurs
    float SpotPower;    // Uniquement projecteurs
};