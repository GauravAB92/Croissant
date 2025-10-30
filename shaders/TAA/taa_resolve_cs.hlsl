#include "../Croissant/shaders/common/taa_cb.h"

Texture2D<float4> g_unresolvedColor : register(t0);
Texture2D<float2> g_motionVectors   : register(t1);
Texture2D<float4> g_feedback1Color  : register(t2);
RWTexture2D<float4> g_resolvedColor : register(u0);
RWTexture2D<float4> g_feedback2Color : register(u1);

SamplerState g_sampler : register(s0);


cbuffer c_TemporalAA : register(b0)
{
    TemporalAntiAliasingConstants g_TemporalAA;
};

[numthreads(16, 16, 1)]
void main_cs(
    in int2 i_groupIdx  : SV_GroupID,
    in int2 i_threadIdx : SV_GroupThreadID,
    in int2 i_globalIdx : SV_DispatchThreadID
)
{
    uint2 pixelID  = i_globalIdx.xy;
    uint2 textureSize;

    g_unresolvedColor.GetDimensions(textureSize.x, textureSize.y);

    if(pixelID.x >= textureSize.x || pixelID.y >= textureSize.y)
        return; //out of bounds

    int2 inputPosInt = int2(round(pixelID));
    float clampingFactor      = g_TemporalAA.clampingFactor;
    float newFrameWeight      = g_TemporalAA.newFrameWeight;
    float2 motionVector       = g_motionVectors[pixelID].xy;
    float2 sourcePos          = float2(pixelID.xy);
    float3 thisPixelColor     = g_unresolvedColor[pixelID].rgb;

    float3 result             = thisPixelColor;
    float2 centerUV           = (sourcePos + float2(0.5f,0.5f)) * g_TemporalAA.outputTextureSizeInv;
    float3 history            = g_feedback1Color.SampleLevel(g_sampler, centerUV, 0).rgb;

    if(newFrameWeight < 1.0f)
    {
        result = lerp(history, thisPixelColor,  0.1f);
    }

    g_resolvedColor[pixelID]  = float4(result, 1.0);
    g_feedback2Color[pixelID] = float4(result, 1.0);
} 