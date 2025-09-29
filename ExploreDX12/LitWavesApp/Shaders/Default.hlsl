// Définition par défaut des nombres de lumières.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtils.hlsl"

// Données constantes qui varient par frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

// Données constantes qui varient par matériau.
cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

// Données constantes qui varient par passe.
cbuffer cbPass : register(b2)
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
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    // Transformation en espace monde.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    // On assume un scaling non uniform, on doit donc utiliser la matrice inverse-transpose du monde.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    // Transformation en espace homogeneous clip.
    vout.PosH = mul(posW, gViewProj);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // L'interpolation des normales peut les dénormaliser, on doit donc les renormaliser.
    pin.NormalW = normalize(pin.NormalW);
    // Vecteur du point éclairé vers l'oeil.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    // Éclairage indirect.
    float4 ambient = gAmbientLight * gDiffuseAlbedo;
    // Éclairage direct.
    const float shininess = 1.0f - gRoughness;
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);
    float4 litColor = ambient + directLight;
    // Convention courante de prendre l'alpha depuis le matériau diffus.
    litColor.a = gDiffuseAlbedo.a;
    return litColor;
}