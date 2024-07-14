#include "graphicspch.h"
#include "graphics/materials/materialinstance_generic.h"
#include "graphics/ShaderProgramManager.h"
#include "graphics/model.h"
#include "graphics/animatedmodel.h"

namespace engine
{

static InputLayoutDesc s_vertexInputLayout[] =
{
	{ "POSITION", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_position), INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_normal),  INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, ImageFormat::RG32F, 0, (UINT)offsetof(Vertex, m_texcoord0),  INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_tangent),  INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_bitangent),  INPUT_PER_VERTEX_DATA, 0 }
};

static InputLayoutDesc s_animatedVertexInputLayout[] =
{
	{ "POSITION", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_position), INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_normal),  INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, ImageFormat::RG32F, 0, (UINT)offsetof(AnimatedVertex, m_texcoord),  INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_tangent),  INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_bitangent),  INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_bitangent),  INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDINDICES", 0, ImageFormat::RGBA32, 0, (UINT)offsetof(AnimatedVertex, m_bitangent),  INPUT_PER_VERTEX_DATA, 0 }
};

std::string getPixelVariationDefine(uint32_t pixelVariation)
{
	std::string defines;
	
	if (pixelVariation & PixelVariation_Unlit)
		defines.append("UNLIT\n");

	return defines;
}

std::string getPixelVariationName(const std::string& instanceName, uint32_t pixelVariation)
{
	std::string shaderName = instanceName;
	if (pixelVariation & PixelVariation_Unlit)
		shaderName += "_unlit";
	if (pixelVariation & PixelVariation_Lit)
		shaderName += "_lit";
	return shaderName;
}

MaterialInstance_Generic::MaterialInstance_Generic()
{
	//m_staticMeshShader = g_shaderManager->createShaderProgram("materialinstance_generic.vsh",
	//	"materialinstance_generic.psh");

	//m_skinnedMeshShader = g_shaderManager->createShaderProgram("materialinstance_generic.vsh",
	//	"materialinstance_generic.psh",
	//	"#define SKINNED\n");

	//m_particleShader = g_shaderManager->createShaderProgram("materialinstance_generic.vsh",
	//	"materialinstance_generic.psh",
	//	"#define PARTICLE\n");
}

MaterialInstance_Generic::~MaterialInstance_Generic()
{
}

IShaderProgram* MaterialInstance_Generic::getShaderProgram_StaticVertexFactory_Variation(uint32_t pixelVariation)
{
	std::string shaderName = "materialinstance_generic";
	if (pixelVariation & PixelVariation_Unlit)
		shaderName += "_unlit";
	if (pixelVariation & PixelVariation_Lit)
		shaderName += "_lit";

	auto it = m_pixelVariations[VertexFactory_StaticMesh].find(shaderName);
	if (it != m_pixelVariations[VertexFactory_StaticMesh].end())
		return it->second;

	Core::msg("Graphics: Generation variation '%s' for MaterialInstance_Generic", shaderName.c_str());

	std::string defines = getPixelVariationDefine(pixelVariation);

	InputLayoutDesc inputLayout[] =
	{
		{ "POSITION", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_position), INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_normal),  INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, ImageFormat::RG32F, 0, (UINT)offsetof(Vertex, m_texcoord0),  INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_tangent),  INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(Vertex, m_bitangent),  INPUT_PER_VERTEX_DATA, 0 }
	};

	IShaderProgram* shaderProgram = g_shaderManager->createShaderProgram(
		"materialinstance_generic.vsh",
		"materialinstance_generic.psh",
		defines.c_str(),
		inputLayout,
		sizeof(inputLayout) / sizeof(inputLayout[0]));

	m_pixelVariations[VertexFactory_StaticMesh].emplace(shaderName, shaderProgram);

	return shaderProgram;
}

IShaderProgram* MaterialInstance_Generic::getShaderProgramVariation(VertexFactory vertexFactory, uint32_t pixelVariation)
{
	Assert(vertexFactory >= VertexFactory_Count);

	std::string variationName = getPixelVariationName("materialinstance_generic", pixelVariation);
	auto& pixelVariationMap = m_pixelVariations[vertexFactory];

	auto it = pixelVariationMap.find(variationName);
	if (it != pixelVariationMap.end())
		return it->second;

	Core::msg("Graphics: Generation variation '%s' for MaterialInstance_Generic", variationName.c_str());

	std::string defines = getPixelVariationDefine(pixelVariation);

	IShaderProgram* shaderProgram = nullptr;

	switch (vertexFactory)
	{
	case VertexFactory_StaticMesh:
		shaderProgram = g_shaderManager->createShaderProgram(
			"materialinstance_generic.vsh",
			"materialinstance_generic.psh",
			defines.c_str(),
			s_vertexInputLayout,
			sizeof(s_vertexInputLayout) / sizeof(s_vertexInputLayout[0]));
		break;
	case VertexFactory_SkinnedMesh:
		defines += "SKINNED\n";

		shaderProgram = g_shaderManager->createShaderProgram(
			"materialinstance_generic.vsh",
			"materialinstance_generic.psh",
			defines.c_str(),
			s_animatedVertexInputLayout,
			sizeof(s_animatedVertexInputLayout) / sizeof(s_animatedVertexInputLayout[0]));
		break;
	//case VertexFactory_Billboard:
	//	break;
	}

	Assert2(shaderProgram, "Unknowed vertex factory");

	pixelVariationMap.emplace(variationName, shaderProgram);

	return shaderProgram;
}

}