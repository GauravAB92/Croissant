#if !defined(COMMON_HLSL_DEFINES_H)
#define COMMON_HLSL_DEFINES_H

//conditional flag for host vs hlsl
#if defined(__cplusplus)
    #include <render/backend/dx12/CustomHLSLTypes.h>
#endif

// Flags: bit 0 = silhouette, 1 = discontinuity, 2 = intersection, 3 = conservative
static const uint FLAG_SILHOUETTE           = 1u << 0;
static const uint FLAG_DISCONTINUITY        = 1u << 1;
static const uint FLAG_INTERSECTION         = 1u << 2;
static const uint FLAG_CONSERVATIVE         = 1u << 3;
static const uint FLAG_FALSE_INTERSECTION   = 1u << 4;


struct
#if defined(__cplusplus)
  alignas(16)
#endif 
EdgeDebugData
{
    uint    flags; uint _padFlags0; uint _padFlags1; uint _padFlags2; // 16B (nice header)
    float4  edgeCoordinates[3];                   // 48B  //encodes start and end  coords for each intersecting edge
    float4  triangleCoordinates[3];               // 48B (store xyz, w=0/1)
    float   coverageValue;                        // 16B total
    float   extentValue;                          // +
    float   extentFramebuffer;                    // 4B
    float   depthValue;                           // +
    
    float   depthFramebuffer;                     // 4B
    float   depthDelta;                           // 4B
    float   E_prime;                              // 4B
    float  __pad0;                                // 4B

	float4  adjExtentFramebuffer;                 // 16B
	float4  adjDepthFramebuffer;                  // 16B
    float4  adjDepthDeltas;                       // 16B
    float4  adjTriPlaneValues;                    // 16B
    float4  eraaOffsets;                          // 16B
    float4  barycentricCoords;                    // 16B
    
    uint2   readPixelPosition;                    // 8B
    uint    intersectionPixelID;                  // 4B
	uint   _pad3;                                 // 4B
    float4  interceptsAndArea;                    // 16B
};

struct  
#if defined(__cplusplus)
  alignas(16)
#endif 
  GeneralDebugData
    {
	 uint2  pixelPositionCurrent;      //Mouse hovered pixel
	 uint2  pixelPositionSelected;     //Left click selected pixel
	 float4 pixelColorValue;           //RGBA
     float3 pixelNormal;
     float  pixelDepthValue;
     float2 padder;
    };

struct 
#if defined(__cplusplus)
  alignas(256)
#endif 
  ConstantBufferEntry
  {
      float4x4  viewProjMatrix;      // 64
      float4x4  viewMatrix;          // 64  (128)
      float4x4  projMatrix;          // 64  (192)
      float4x4  inverseProjMatrix;   // 64  (256)
      float2    resolution;          // 8   (264)

	  uint2     debugPixel;          // 8   (272)
      float     _pad[54];            // 62 * 4 = 248  -> total = 512 bytes
  };
#endif // COMMON_HLSL_DEFINES_H

struct
#if defined(__cplusplus)
      alignas(16)
#endif 
  ResolveDebugData
  {
    float4 pixelColor;
    float4 offsets;
    float4 edgeTypeInfo;
    float4 pad0;
};











