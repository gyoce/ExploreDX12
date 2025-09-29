#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    // La brillance est l'inverse de la rugosité : Shininess = 1 - roughness.
    float Shininess;
};

// Implémentation d'un facteur d'atténuation linéaire, s'applique pour les points lumineux et les projecteurs.
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// L'approximation de Schlick pour les équations de Fresnel, cela permet d'approximer le pourcent de lumière réfléchie d'une surface avec une normale n avec un angle entre le vecteur lumière L et la normale n.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    return reflectPercent;
}

// Calcul de la quantité de lumière réfléchie dans l'oeil, c'est le somme de la réflectance diffuse et de la réflectance spéculaire.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    float3 specAlbedo = fresnelFactor * roughnessFactor;
    // L'équation spéculaire va en dehors de l'interval [0,1], mais on fait du rendu LDR, donc on la scale un peu.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // Le vecteur lumière pointe vers la source de lumière.
    float3 lightVec = -L.Direction;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Le vecteur depuis la surface vers la lumière.
    float3 lightVec = L.Position - pos;
    // La distance entre la surface et la lumière.
    float d = length(lightVec);
    // Test de portée.
    if (d > L.FalloffEnd)
        return 0.0f;
    // Normaliser le vecteur de lumière.
    lightVec /= d;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    // Atténuer la lumière par la distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Le vecteur depuis la surface vers la lumière.
    float3 lightVec = L.Position - pos;
    // La distance entre la surface et la lumière.
    float d = length(lightVec);
    // Test de portée.
    if (d > L.FalloffEnd)
        return 0.0f;
    // Normaliser le vecteur de lumière.
    lightVec /= d;
    // Scale la lumière par la loi des cosinus de Lambert.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    // Atténuer la lumière par la distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;
    // Scale par le projecteur
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 result = 0.0f;
    int i = 0;
#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; i++)
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
#endif
#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i++)
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
#endif
#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; i++)
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
#endif
    return float4(result, 0.0f);
}