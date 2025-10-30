
struct BlitConstants
{
	float2 sourceOrigin;  // Origin in source texture coordinates
	float2 sourceSize;    // Size in source texture coordinates
	float2 targetOrigin;  // Origin in target texture coordinates
	float2 targetSize;    // Size in target texture coordinates
};

cbuffer BlitConstantsBuffer : register(b0)
{
	BlitConstants g_Blit;
};

void main(
	in uint iVertex : SV_VertexID,
	out float4 o_posClip : SV_Position,
	out float2 o_uv : UV)
{
	uint u = iVertex & 1;
	uint v = (iVertex >> 1) & 1;

    float2 src_uv = float2(u, v) * g_Blit.sourceSize + g_Blit.sourceOrigin;
    float2 dst_uv = float2(u, v) * g_Blit.targetSize + g_Blit.targetOrigin;

	o_posClip = float4(dst_uv.x * 2 - 1, 1 - dst_uv.y * 2, 0, 1);
	o_uv = dst_uv;
}
