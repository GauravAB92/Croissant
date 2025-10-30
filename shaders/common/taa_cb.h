#ifndef TAA_CB_H
#define TAA_CB_H

#ifdef __cplusplus
#include <../source/render/backend/dx12/CustomHLSLTypes.h>
#endif

struct TemporalAntiAliasingConstants
{
	float4x4 reprojectionMatrix;                //reproject current pixel into history pixel space

    float2 inputViewOrigin;
    float2 inputViewSize;

    float2 outputViewOrigin;
    float2 outputViewSize;

    float2 inputPixelOffset;
    float2 outputTextureSizeInv;

    float2 inputOverOutputViewSize;
    float2 outputOverInputViewSize;

    float clampingFactor;
    float newFrameWeight;
	float pqC;                                  // pqC is a constant used in the PQ color space conversion
	float invPqC;                               // invPqC is the inverse of pqC, used for converting back from PQ to linear color space

    uint stencilMask;
    uint useHistoryClampRelax;
    uint padding0;
    uint padding1;
};

#endif // TAA_CB_H