#ifndef CBUFFERS_H
#define CBUFFERS_H

// NOTE: Please note this macro should be up to date with C++ code: graphics\light.h
#define MAX_POINT_LIGHTS 32

cbuffer GlobalData : register(b0)
{
	row_major float4x4 g_modelMatrix;
	row_major float4x4 g_viewMatrix;
	row_major float4x4 g_projectionMatrix;
	row_major float4x4 g_modelViewProjection;

	float4 g_viewPos;
	float4 g_viewDir;
};

cbuffer SkinningData : register(b1)
{
	row_major float4x4 g_bonesMatrices[256];
};

cbuffer DirectionalLightCB : register(b2)
{
	float4 g_DirLight_direction;
	float4 g_DirLight_ambientColor;
	float4 g_DirLight_color;
};

struct PointLight
{
	float4 position;
	float4 color;
	float4 specular;
	float4 lightData; // r - radius
};

cbuffer PointLightCB : register(b3)
{
	PointLight g_pointLights[MAX_POINT_LIGHTS];
};

cbuffer LightGlobalDataCB : register(b4)
{
	uint g_pointLightCount;
	uint g_spotLightCount;
	uint padding0;
	uint padding1;
};

#endif