#include "../Croissant/shaders/common/taa_cb.h"

[numthreads(16, 16, 1)]
void main_cs(
    in int2 i_groupIdx  : SV_GroupID,
    in int2 i_threadIdx : SV_GroupThreadID,
    in int2 i_globalIdx : SV_DispatchThreadID
)
{
    uint2 pixelID  = i_globalIdx.xy;
    uint2 textureSize;
} 