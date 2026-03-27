cbuffer CB : register(b0)
{
    float4x4 g_Transform;
	float4x4 g_ForwardView;  // Forward view matrix, used for screen space calculations
    float4x4 g_Projection;   // Projection matrix
	float4x4 g_InverseProjection;
    float2   g_ViewportSize; // Viewport size for screen space calculations
	float 	pad[62];
};


struct VS_INPUT
{
	float3 position : POSITION;
	float2 uv       : UV;
	float3 normal   : NORMAL;
};

struct PS_INPUT
{
	float4 pos 		: SV_Position;
	float3 normalWS : COLOR1;
	float3 normalVS : COLOR4;
	float2 uv 		: UV;
};

void main_vs(
	in  VS_INPUT input,
	out PS_INPUT output
)
{
    output.pos 			= mul(g_Transform , float4(input.position.x, input.position.y, input.position.z, 1.0));
	output.normalWS 	= input.normal;
	output.uv           = input.uv;
	float3 viewNormal 	= mul(g_ForwardView, float4(input.normal.x,input.normal.y,input.normal.z, 0.0)).xyz; 	// Transform normal to view space
    output.normalVS 	= float3(viewNormal.x, viewNormal.y, viewNormal.z);								 			// Use the normal as color for this pass
}

void main_ps (
	in PS_INPUT input,
	out float4 o_color      : SV_Target)
{
	// directional light source
	float3 lightDir = float3(1.0f, 1.0f,-50.0f);
	//lightDir 		= mul(g_ForwardView, float4(lightDir, 0.0)).xyz; // Transform light direction to view space
	lightDir 		= normalize(lightDir);
	float3 normal 	= normalize(input.normalVS); // Use the normal in view space for lighting calculations
	float diffuse 	= max(dot(normal, lightDir), 0.0f) * 0.74f;

	// 	o_color 	= float4(diffuse, diffuse, diffuse, 1.0f); // Apply diffuse lighting
	// 	o_color 	= pow(o_color,(1.0f / 2.2f)); // Apply gamma correction
	// 	o_color.a 	= 1.0f; // Set alpha to 1.0
	//	o_color     = float4(input.uv.x,input.uv.y,0.0f,1.0f); // Output UV coordinates as color for debugging

	float2 c 		= floor(input.uv * 8.0);
	float checker 	= fmod(c.x + c.y, 2.0);
	float3 color 	= lerp(float3(0.2, 0.2, 0.2), float3(0.9, 0.9, 0.9), checker);
	o_color   		= float4(color , 1.0f); // Output the checker pattern modulated by diffuse lighting
}

void main_wireframe_ps (
	in PS_INPUT input,
	out float4 o_color      : SV_Target)
{
	// directional light source
	float3 lightDir = float3(1.0f, 1.0f,-50.0f);
	//lightDir 		= mul(g_ForwardView, float4(lightDir, 0.0)).xyz; // Transform light direction to view space
	lightDir 		= normalize(lightDir);
	float3 normal 	= normalize(input.normalVS); // Use the normal in view space for lighting calculations
	float diffuse 	= max(dot(normal, lightDir), 0.0f) * 0.74f;
	
	o_color   		= float4(1.0f,1.0f,1.0f,1.0f); // Output the checker pattern modulated by diffuse lighting
}






