

#pragma pack_matrix(row_major)

struct MotionVectorConstants
{
    float4x4 reprojectionMatrix;
    float2  inputViewSize;
    float2  inputViewOrigin;
    float2  _pad;
};


Texture2D<float>    t_depthCurrent : register(t0);
RWTexture2D<float2> rw_motionVectors : register(u0);
cbuffer c_MotionVector : register(b0)
{
    MotionVectorConstants mvConstants;
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
    t_depthCurrent.GetDimensions(textureSize.x, textureSize.y);

    if (any(pixelID >= textureSize))
        return;

    float depth = t_depthCurrent.Load(int3(pixelID, 0));

    if (depth >= 1.0)
    {
        rw_motionVectors[pixelID] = float2(0, 0);
        return;
    }

    float2 uv = (float2(pixelID) + 0.5) / float2(textureSize);

    float4 clipPos;
    clipPos.x = 2 * uv.x - 1;
    clipPos.y = 1 - uv.y * 2;
    clipPos.z = depth;
    clipPos.w = 1;

    float4 prevClipPos = mul(clipPos, mvConstants.reprojectionMatrix);

    if (prevClipPos.w <= 0)
    {
        rw_motionVectors[pixelID] = 0;
        return;
    }

    prevClipPos.xyz /= prevClipPos.w;

    float2 prevUV;

    prevUV.x = 0.5 + prevClipPos.x * 0.5;
    prevUV.y = 0.5 - prevClipPos.y * 0.5;
    float2 prevWindowPos = prevUV * mvConstants.inputViewSize + mvConstants.inputViewOrigin;

    float2 motionVector = prevWindowPos.xy - float2(pixelID);

    // Convert from pixel space to UV space for SMAA
    rw_motionVectors[pixelID] = motionVector / mvConstants.inputViewSize;
    // Visualize: map pixel position to color to confirm shader is executing
    //rw_motionVectors[pixelID] = float2(pixelID) / float2(textureSize);


} 

