#ifndef COMMON_UTILS_HLSLI
#define COMMON_UTILS_HLSLI

float3 ToNDCPosition(float3 pVector, float4x4 projectionMatrix)
{
    float4 clipPos = mul(projectionMatrix, float4(pVector, 1.0));
    return clipPos.xyz / clipPos.w;
}

float3 ToNDCDirection(float3 dVector, float4x4 projectionMatrix)
{
    // Convert view space vector to NDC
    float4 clipSpaceVector = mul(projectionMatrix, float4(dVector, 1.0f));
    return clipSpaceVector.xyz / clipSpaceVector.w; // Normalize by w to get NDC coordinates
}


float3 viewRayFromPixel(float2 screenPos, float2 viewportSize, float4x4 inverseProjection)
{
    float2 posNDC = (screenPos / viewportSize) * 2.0 - 1.0;
    posNDC.y = -posNDC.y; // Invert Y for NDC
    float4 clipSpacePos = float4(posNDC, 1.0, 1.0); //z = 1 plane hence w = 1
    float4 v = mul(inverseProjection, clipSpacePos);
    float3 p = v.xyz / v.w;     // point on far plane in view space
    return normalize(p);
}


float3 getViewSpacePosFromScreenSpacePos(float3 screenPos, float2 viewPortSz, float4x4 inverseProjection)
{
    //Screen to NDC
    float2 ndcPos = ( (screenPos.xy - float2(0.5f, 0.5f)) / viewPortSz) * 2.0 - 1.0;

    ndcPos.y = -ndcPos.y; // Invert Y for NDC

    float4 clipPos = float4(ndcPos, screenPos.z, 1.0);
    float4 viewPos = mul(clipPos, inverseProjection);
    return viewPos.xyz / viewPos.w;
}


bool equalsOne(float4 v)
{
    return (v.x == 1.0f && v.y == 1.0f && v.z == 1.0f && v.w == 1.0f);
}

bool equalsOne(float v[5])
{
    return (v[0] == 1.0f && v[1] == 1.0f && v[2] == 1.0f && v[3] == 1.0f && v[4] == 1.0f);
}

bool equalsZero(float4 v)
{
    return (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f && v.w == 0.0f);
}


bool equalsZero(float v[5])
{
    return (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f && v[3] == 0.0f && v[4] == 0.0f);
}


#endif // COMMON_UTILS_HLSLI