cbuffer CB : register(b0)
{
    float4x4 g_Transform;
	float4x4 g_ForwardView;  // Forward view matrix, used for screen space calculations
    float4x4 g_Projection;   // Projection matrix
	float4x4 g_InverseProjection;
    float2   g_ViewportSize; // Viewport size for screen space calculations
	float 	pad[62];
};


void main_vs(
	float3 i_position   : POSITION, 
	float2 i_uv			: UV,
	float3 i_normal   	: NORMAL, 
	out float4 o_pos	: SV_Position,
	out float3 o_normalWS	: COLOR1,
	out float3 o_normalVS   : COLOR4 
)
{
    o_pos =  mul(g_Transform , float4(i_position.x, i_position.y, i_position.z, 1.0));
	o_normalWS = i_normal;
	float3 viewNormal = mul(g_ForwardView, float4(i_normal.x,i_normal.y,i_normal.z, 0.0)).xyz; 	// Transform normal to view space
    o_normalVS = float3(viewNormal.x, viewNormal.y, viewNormal.z);								 // Use the normal as color for this pass
}

void main_ps(
	in float4 i_pos : SV_Position,
	in float3 o_normalWS	: COLOR1,
	in float3 o_normalVS   : COLOR4,
	out float4 o_color     : SV_Target
)
{
	// directional light source
	float3 lightDir = float3(1.0f, 1.0f,-50.0f);
	//lightDir 		= mul(g_ForwardView, float4(lightDir, 0.0)).xyz; // Transform light direction to view space
	lightDir 		= normalize(lightDir);
	float3 normal 	= normalize(o_normalVS);
	float diffuse 	= max(dot(normal, lightDir), 0.0f) * 0.74f;

	
	o_color = float4(diffuse, diffuse, diffuse, 1.0f); // Apply diffuse lighting
	o_color = pow(o_color,(1.0f/2.2f)); // Apply gamma correction
	o_color.a = 1.0f; // Set alpha to 1.0

}
