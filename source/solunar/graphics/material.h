#ifndef MATERIAL_H
#define MATERIAL_H

#include "graphics/shaderprogram.h"
#include "graphics/ShaderProgramManager.h"
#include "graphics/texturemap.h"
#include "graphics/graphicsobject.h"

namespace engine
{
	enum MAT_TEX
	{
		MAT_TEX_DIFFUSE,
		MAT_TEX_NORMALMAP,
		MAT_TEX_DETAIL,
	};

	class TextureMap;
	class VertexShader;
	class PixelShader;
	class MaterialInstance;
	class IShaderProgram;
	
	struct MaterialUniforms
	{
		// Material data
		uint32_t m_albedoColor;

		// Samplers
		uint32_t m_albedoTexture;
		uint32_t m_normalTexture;
		uint32_t m_hasNormalTexture;
		uint32_t m_specularTexture;
		uint32_t m_hasSpecularTexture;

		
		uint32_t m_selfillum;
		uint32_t m_viewPos;

		// Matrices
		uint32_t m_modelViewProjection;
		uint32_t m_modelMatrix;
		uint32_t m_viewMatrix;
		uint32_t m_projectionMatrix;

		// #TODO: Lights
		uint32_t m_lightsCount;

		// #TODO: CAMERA
		uint32_t m_znear;
		uint32_t m_zfar;

		// Constant buffers
		uint32_t m_perWorldMatricesLocation;
		uint32_t m_perObjectMatricesLocation;
		
	};

	class Material;
	
	IShaderProgram* createShaderProgramFromMaterial(Material* material);
	
	enum class MaterialType
	{
		Null,
		Generic,
		Masked
	};

	class Material : public GraphicsObject
	{
		friend IShaderProgram* createShaderProgramFromMaterial(Material* material);
		
		ImplementObject(Material, GraphicsObject);
	public:
		static void createMaterialFromImport(const char* name, const char* diffuseName, const char* normalName);

	public:
		Material();

		~Material();

		void load(const std::shared_ptr<DataStream>& dataStream) override;

		void loadXML(tinyxml2::XMLElement& element) override;
		void saveXML(tinyxml2::XMLElement& element) override;

	public:
		void setName(const std::string& name) { m_materialName = name; }

		void createHw() override;
		void releaseHw() override;

		void bindSamplers();

		void bindUniformsCustom(IShaderProgram* shader);

		void bind();

		std::string& getName() { return m_materialName; }

		std::weak_ptr<TextureMap> getTexture(MAT_TEX tex);
		MaterialType getMaterialType() { return m_materialType; }

		MaterialInstance* getMaterialInstance() { return m_materialInstance; }

	public:
		bool m_isFirstPersonWeapon;

	private:
		MaterialInstance* m_materialInstance;

		ISamplerState* m_albedoSampler;

		std::weak_ptr<TextureMap> m_albedoTexture;
		std::weak_ptr<TextureMap> m_normalTexture;
		std::weak_ptr<TextureMap> m_specularTexture;

		std::string m_materialName;
		std::string m_shaderName;
		
		std::string m_albedoTextureName;
		std::string m_normalTextureName;
		std::string m_specularTextureName;

		MaterialUniforms m_uniforms;

		glm::vec3 m_albedoColor;

		MaterialType m_materialType;

		bool m_depthWrite;
		bool m_skipmips;
		bool m_selfillum;
		bool m_clampToEdge;
	};

	class NullMaterial : public Material
	{
		ImplementObject(NullMaterial, Material);
	public:
		NullMaterial();
		~NullMaterial();

		static void registerObject();
	};

	class GenericMaterial : public Material
	{
		ImplementObject(GenericMaterial, Material);
	public:
		GenericMaterial();
		~GenericMaterial();

		static void registerObject();
	};

	class LayeredMaskedMaterial : public Material
	{
		ImplementObject(LayeredMaskedMaterial, Material);
	public:
		LayeredMaskedMaterial();
		~LayeredMaskedMaterial();

		static void registerObject();
	};
}

#endif // !MATERIAL_H
