// Material Instance shader - Generic material
//

// C++ defines:
//	SKINNED

#include "cbuffers.h"

struct VSInput
{
	float3 position		: POSITION;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD;
	float3 tangent		: TANGENT;
	float3 bitangent	: BINORMAL;
#ifdef SKINNED
	float4 weights		: BLENDWEIGHT;
	float4 boneIds		: BLENDINDICES;
#endif
};

struct VSOutput
{
	float4 position		: SV_Position;
	float4 worldPos		: TEXCOORD0;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD1;
	float3 tangent		: TANGENT;
	float3 bitangent	: BINORMAL;
	float4 lightSpace	: TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

#ifdef SKINNED
	// calculate bone transform
	row_major float4x4 skinMatrix = input.weights.x * g_bonesMatrices[int(input.boneIds.x)]
		+ input.weights.y * g_bonesMatrices[int(input.boneIds.y)]
		+ input.weights.z * g_bonesMatrices[int(input.boneIds.z)]
		+ input.weights.w * g_bonesMatrices[int(input.boneIds.w)];

	// World position
	output.worldPos = mul(float4(input.position, 1.0f), skinMatrix);
	output.worldPos = mul(output.worldPos, g_modelMatrix);
#else
	// World position
	output.worldPos = mul(float4(input.position, 1.0f), g_modelMatrix);
#endif

	// #TODO: multiply output.worldPos by g_viewProjection matrix.

	// Position
	output.position = mul(output.worldPos, g_viewMatrix);
	output.position = mul(output.position, g_projectionMatrix);

	// texcoord
	output.texcoord = input.texcoord;

#ifdef SKINNED
	// normal
	output.normal = normalize(mul(float4(input.normal, 0.0f), skinMatrix));
	output.normal = mul(float4(output.normal, 0.0f), g_inverseModelMatrix);
#else
	// normal
	//output.normal = normalize(mul(float4(input.normal, 0.0f), g_inverseModelMatrix));
	output.normal = normalize(mul(input.normal, (float3x3)g_inverseModelMatrix));
#endif

	// Light space position
	output.lightSpace = mul(output.worldPos, g_LightViewProjection);
	
	return output;
}