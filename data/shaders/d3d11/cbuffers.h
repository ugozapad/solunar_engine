#ifndef CBUFFERS_H
#define CBUFFERS_H

// NOTE: Please note this macro should be up to date with C++ code: graphics\light.h
#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 32

// We are use row major matrices
#pragma pack_matrix( row_major )

cbuffer GlobalData : register(b0)
{
	float4x4 g_modelMatrix;
	float4x4 g_viewMatrix;
	float4x4 g_projectionMatrix;
	float4x4 g_modelViewProjection;
	float4x4 g_inverseModelMatrix;
	float4x4 g_LightView;
	float4x4 g_LightViewProjection;
	float4x4 g_inverseViewProjection;

	float4 g_viewPos;
	float4 g_viewDir;
};

cbuffer SkinningData : register(b1)
{
	float4x4 g_bonesMatrices[256];
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

struct SpotLight
{
	float4 position;
	float4 color;
	float4 specular;
	float4 lightData; // r - cutoff, gba - direction
};

cbuffer PointLightCB : register(b3)
{
	PointLight g_pointLights[MAX_POINT_LIGHTS];
};

cbuffer SpotLightCB : register(b4)
{
	SpotLight g_spotLights[MAX_SPOT_LIGHTS];
};

cbuffer LightGlobalDataCB : register(b5)
{
	uint g_pointLightCount;
	uint g_spotLightCount;
	uint padding0;
	uint padding1;
};

#endif
