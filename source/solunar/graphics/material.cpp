#include "graphicspch.h"

#include "core/file/filesystem.h"

#include "graphics/material.h"
#include "graphics/texturemap.h"
#include "graphics/shaderprogram.h"
#include "graphics/ShaderProgramManager.h"
#include "graphics/rendercontext.h"
#include "graphics/shaderconstantmanager.h"
#include "graphics/graphicsoptions.h"

#include "graphics/core/device.h"
#include "graphics/core/statemanager.h"

#include "graphics/materials/materialinstance.h"
#include "graphics/materials/materialinstancefactory.h"

#include "engine/camera.h"

#include "engine/camera.h"
#include "engine/entity/world.h"
#include "graphics/graphicsworld.h"

#include "d3d11drv/d3d11texture2d.h"

#include <sstream>

namespace solunar
{
	MaterialType getMaterialTypeFromName(const std::string& shaderName)
	{
		if (shaderName == "diffuse")
		{
			return MaterialType::Generic;
		}
		else if (shaderName == "generic" || shaderName == "Generic")
		{
			return MaterialType::Generic;
		}
		else
		{
			Core::Error("getMaterialType: Doesnt support old shader %s", shaderName.c_str());	
		}

		return MaterialType::Null;
	}
	
	void Material::createMaterialFromImport(const char* name, const char* diffuseName, const char* normalName)
	{
		std::string materiaName;
		materiaName += "data/";
		materiaName += "materials/";
		materiaName += name;
		materiaName += ".xml";

		//File* file = FileDevice::GetInstance()->openFile(materiaName, FileAccess::Write);
		FileHandle file = g_fileSystem->Create(materiaName.c_str());

		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* rootElement = doc.NewElement("Material");

		Material* materialToWrite = TypeManager::GetInstance()->CreateObject<Material>();
		materialToWrite->m_albedoTextureFileName = diffuseName;
		materialToWrite->m_normalTextureName = normalName;
		
		materialToWrite->SaveXML(*rootElement);;

		doc.InsertFirstChild(rootElement);

		tinyxml2::XMLPrinter visitor;
		doc.Accept(&visitor);

		std::string text = visitor.CStr();
		text += '\0';

		g_fileSystem->Write(file, (void*)text.data(), text.size());

		mem_delete(materialToWrite);
		materialToWrite = nullptr;

		g_fileSystem->Close(file);
	}

	IMPLEMENT_OBJECT(Material, GraphicsObject);

	Material::Material()
	{
		m_albedoColor = glm::vec3(1.0f);
		m_albedoSampler = nullptr;
		m_depthWrite = true;
		m_selfillum = false;
		m_clampToEdge = false;
		m_skipmips = false;
		m_pointFilter = false;
		m_materialInstance = nullptr;
		m_isFirstPersonWeapon = false;
	}

	Material::~Material()
	{
		if (m_materialInstance)
		{
			mem_delete(m_materialInstance);
			m_materialInstance = nullptr;
		}

		//m_shader = nullptr;

		m_specularTexture.reset();
		m_normalTexture.reset();
		m_albedoTexture.reset();
	
		m_albedoSampler = nullptr;
	}

	void Material::Load(const std::shared_ptr<DataStream>& dataStream)
	{
		dataStream->Seek(Seek_End, 0);
		size_t length = dataStream->Tell();
		dataStream->Seek(Seek_Begin, 0);

		char* data = new char[length + 1];
		dataStream->Read((void*)data, length);
		data[length] = '\0';

		//parse(data, length);

		tinyxml2::XMLDocument doc;
		doc.Parse(data, length);

		tinyxml2::XMLElement* materialElement = doc.FirstChildElement("Material");
		LoadXML(*materialElement);

		delete[] data;
	}

	void Material::LoadXML(tinyxml2::XMLElement& element)
	{
		if (strcmp(element.Name(), "Material") != 0) {
			Core::Error("Material::LoadXML: failed to load material. XMLElement is not a \"Material\".");
		}

		tinyxml2::XMLElement* shaderElement = element.FirstChildElement("Shader");
		if (shaderElement) {
			m_materialType = getMaterialTypeFromName(shaderElement->GetText());
			
		} else {
			Core::Error("Material::LoadXML: failed to load material. Doesnt exist \"Shader\" element.");
		}

		// Albedo texture
		tinyxml2::XMLElement* albedoTexture = element.FirstChildElement("AlbedoTexture");
		if (albedoTexture) {
			const tinyxml2::XMLAttribute* filename = albedoTexture->FindAttribute("Filename");
			if (!filename)
				filename = albedoTexture->FindAttribute("filename");

			if (filename) {
				m_albedoTextureFileName = filename->Value();
			} else {
				m_albedoTextureFileName = "textures/notexture.png";
			}

			const tinyxml2::XMLAttribute* clampToEdge = albedoTexture->FindAttribute("ClampToEdge");
			if (clampToEdge) {
				m_clampToEdge = clampToEdge->BoolValue();
			}

			const tinyxml2::XMLAttribute* pointFilter = albedoTexture->FindAttribute("PointFilter");
			if (pointFilter) {
				m_pointFilter = pointFilter->BoolValue();
			}
		} else {
			m_albedoTextureFileName = "textures/notexture.png";
		}

		// Normal texture
		tinyxml2::XMLElement* normalTexture = element.FirstChildElement("NormalTexture");
		if (normalTexture) {
			const tinyxml2::XMLAttribute* filename = normalTexture->FindAttribute("Filename");
			if (!filename)
				filename = normalTexture->FindAttribute("filename");

			if (filename) {
				m_normalTextureName = filename->Value();
			}
		} else {
			m_normalTextureName = "";
		}

		// Specular texture
		tinyxml2::XMLElement* specularTexture = element.FirstChildElement("SpecularTexture");
		if (specularTexture) {
			const tinyxml2::XMLAttribute* filename = specularTexture->FindAttribute("Filename");
			if (!filename)
				filename = specularTexture->FindAttribute("filename");

			if (filename) {
				m_specularTextureName = filename->Value();
			}
		} else {
			m_specularTextureName = "";
		}

		// albedo color
		tinyxml2::XMLElement* albedoColor = element.FirstChildElement("AlbedoColor");
		if (albedoColor) {

			const tinyxml2::XMLAttribute* r = albedoColor->FindAttribute("r");
			const tinyxml2::XMLAttribute* g = albedoColor->FindAttribute("g");
			const tinyxml2::XMLAttribute* b = albedoColor->FindAttribute("b");

			m_albedoColor.r = r->FloatValue();
			m_albedoColor.g = g->FloatValue();
			m_albedoColor.b = b->FloatValue();
		}

		// Render state
		tinyxml2::XMLElement* renderState = element.FirstChildElement("RenderState");
		if (renderState) {

			tinyxml2::XMLElement* depthWrite = renderState->FirstChildElement("DepthWrite");
			if (depthWrite) {
				const tinyxml2::XMLAttribute* depthWriteAttribute = depthWrite->FindAttribute("value");
				if (strcmp(depthWriteAttribute->Value(), "true") == 0) {
					m_depthWrite = true;
				} else if (strcmp(depthWriteAttribute->Value(), "false") == 0) {
					m_depthWrite = false;
				} else {
					Core::Error("Material::LoadXML: failed to load material. Wrong value in the \"DepthWrite\" element. Possible only true/false.");
				}
			}

			tinyxml2::XMLElement* selfillum = renderState->FirstChildElement("SelfIllumination");
			if (selfillum) {
				const tinyxml2::XMLAttribute* selfillumAttribute = selfillum->FindAttribute("value");
				if (strcmp(selfillumAttribute->Value(), "true") == 0) {
					m_selfillum = true;
				}
				else if (strcmp(selfillumAttribute->Value(), "false") == 0) {
					m_selfillum = false;
				}
				else {
					Core::Error("Material::LoadXML: failed to load material. Wrong value in the \"SelfIllumination\" element. Possible only true/false.");
				}
			}

			tinyxml2::XMLElement* firstperson = renderState->FirstChildElement("FirstPersonWeapon");
			if (firstperson) {
				const tinyxml2::XMLAttribute* firstpersonAttribute = firstperson->FindAttribute("value");
				if (strcmp(firstpersonAttribute->Value(), "true") == 0) {
					m_isFirstPersonWeapon = true;
				}
				else if (strcmp(firstpersonAttribute->Value(), "false") == 0) {
					m_isFirstPersonWeapon = false;
				}
				else {
					Core::Error("Material::LoadXML: failed to load material. Wrong value in the \"FirstPersonWeapon\" element. Possible only true/false.");
				}
			}
		} else {
			Core::Error("Material::LoadXML: failed to load material. Doesnt exist \"RenderState\" element.");
		}

		CreateHw();
	}

	void Material::SaveXML(tinyxml2::XMLElement& element)
	{
		// Shader
		tinyxml2::XMLElement* shaderElement = element.InsertNewChildElement("Shader"); //doc.NewElement("Shader");
		shaderElement->SetText("diffuse");

		// Texture's

		// Albedo
		tinyxml2::XMLElement* albedoTexture = element.InsertNewChildElement("AlbedoTexture");
		albedoTexture->SetAttribute("filename", m_albedoTextureFileName.c_str());
		if (m_clampToEdge) {
			albedoTexture->SetAttribute("ClampToEdge", m_clampToEdge);
		}
		else if (m_skipmips) {
			albedoTexture->SetAttribute("SkipMips", m_skipmips);
		}

		// Normal map
		tinyxml2::XMLElement* normalTexture = element.InsertNewChildElement("NormalTexture");
		normalTexture->SetAttribute("filename", m_normalTextureName.c_str());
		
		// Specular map
		tinyxml2::XMLElement* specularTexture = element.InsertNewChildElement("SpecularTexture");
		specularTexture->SetAttribute("filename", m_specularTextureName.c_str());

		// Render states
		tinyxml2::XMLElement* renderStates = element.InsertNewChildElement("RenderState");

		tinyxml2::XMLElement* depthWrite = renderStates->InsertNewChildElement("DepthWrite");
		depthWrite->SetAttribute("value", m_depthWrite);

		tinyxml2::XMLElement* selfillum = renderStates->InsertNewChildElement("SelfIllumination");
		selfillum->SetAttribute("value", m_selfillum);
	}

	std::weak_ptr<TextureMap> loadMaterialTexture(const std::string& path)
	{
		/*if (!g_fileSystem->exist(path))
		{
			Core::Msg("[content]: not found %s, using notexture ...", path.c_str());
			return ContentManager::GetInstance()->LoadTexture("textures/notexture.png");
		}

		return ContentManager::GetInstance()->LoadTexture(path);*/

		return dynamicCastPtr< TextureMap >(g_contentManager->Load("textures/notexture.png", TextureMap::GetStaticTypeInfo()));
	}

	void Material::bindSamplers()
	{
		// OPTICK_EVENT("Material::bindSamplers");
		if (std::shared_ptr<TextureMap> albedoTexture = m_albedoTexture.lock())
		{
			// bind texture slot 0 sampler
			g_renderDevice->SetSamplerState(0, m_albedoSampler);

			// activate diffuse texture as 0
			g_renderDevice->SetTexture2D(0, albedoTexture->getHWTexture());

		}

		// Set up normal texture as 1
		if (std::shared_ptr<TextureMap> normalTexture = m_normalTexture.lock())
		{
			// #TODO: HACK
			g_renderDevice->SetSamplerState(1, m_albedoSampler);

			g_renderDevice->SetTexture2D(1, normalTexture->getHWTexture());
		}

		// Set up specular texture as 2
		if (std::shared_ptr<TextureMap> specularTexture = m_specularTexture.lock())
		{
			// #TODO: HACK
			g_renderDevice->SetSamplerState(2, m_albedoSampler);

			g_renderDevice->SetTexture2D(2, specularTexture->getHWTexture());
		}
	}

	void Material::bindUniformsCustom(IShaderProgram* shader)
	{
		
	}

	void Material::bind()
	{
		// OPTICK_EVENT("Material::bind");
		bindSamplers();
	}

	std::weak_ptr<TextureMap> Material::GetTexture(MAT_TEX tex)
	{
		switch (tex)
		{
		case MAT_TEX_DIFFUSE: return m_albedoTexture;
		case MAT_TEX_NORMALMAP: return m_normalTexture;
		}

		return std::weak_ptr<TextureMap>();
	}

	void Material::CreateHw()
	{
		//initializeShader();

		m_materialInstance = MaterialInstanceFactory::GetInstance()->createMaterialInstance(this);

		SamplerDesc albedoSamplerDesc;
		memset(&albedoSamplerDesc, 0, sizeof(albedoSamplerDesc));

		// if clamp to edge is enable
		if (m_clampToEdge)
		{
			albedoSamplerDesc.m_wrapS = TextureWrap::ClampToEdge;
			albedoSamplerDesc.m_wrapT = TextureWrap::ClampToEdge;
		}
		else
		{
			albedoSamplerDesc.m_wrapS = TextureWrap::Repeat;
			albedoSamplerDesc.m_wrapT = TextureWrap::Repeat;
		}

		m_skipmips = true;

		// if we skip the texture mip maps
		if (m_skipmips)
		{
			albedoSamplerDesc.m_minFilter = TextureFilter::Nearest;
			albedoSamplerDesc.m_magFilter = TextureFilter::Linear;
		}
		else
		{
			albedoSamplerDesc.m_minFilter = TextureFilter::LinearMipmapLinear;
			albedoSamplerDesc.m_magFilter = TextureFilter::Linear;
		}

		if (m_pointFilter)
		{
			albedoSamplerDesc.m_minFilter = TextureFilter::Nearest;
			albedoSamplerDesc.m_magFilter = TextureFilter::Nearest;
		}

		// set anisotropy filter based on what we have in graphics options

		int anisotropicQuality = g_graphicsOptions.m_anisotropicQuality;
		if (anisotropicQuality <= 0)
			anisotropicQuality = 1;

		if (m_pointFilter)
		{
			anisotropicQuality = 1;
		}

		albedoSamplerDesc.m_anisotropyLevel = (float)anisotropicQuality;

		albedoSamplerDesc.m_comparisonFunc = COMPARISON_ALWAYS;

		m_albedoSampler = g_stateManager->CreateSamplerState(albedoSamplerDesc); //g_renderDevice->CreateSamplerState(albedoSamplerDesc);

		//ContentManager* contentManager = ContentManager::GetInstance();

		//if (!g_fileSystem->exist(m_albedoTextureName))
		//{
		//	Core::Msg("[content]: not found %s, using notexture ...", m_albedoTextureName.c_str());
		//	m_albedoTexture = contentManager->LoadTexture("textures/notexture.png");
		//}
		//else
		//{
		//	m_albedoTexture = contentManager->LoadTexture(m_albedoTextureName);
		//}

		// TODO: Return default texture if TextureMap is not exist
		if (m_albedoTextureFileName.empty() || !g_contentManager->OpenStream(m_albedoTextureFileName))
			m_albedoTextureFileName = "textures/system/notex.bmp";

		m_albedoTexture = g_contentManager->LoadObject<TextureMap>(m_albedoTextureFileName);
		m_albedoTexture.lock()->getHWTexture()->SetDebugName(m_albedoTextureFileName.c_str());

		if (m_albedoTexture.lock())
			m_albedoTexture.lock()->getHWTexture()->SetDebugName(m_albedoTextureFileName.c_str());

		if (!m_skipmips)
		{
			D3D11Texture2D* pD3DTexture = dynamic_cast<D3D11Texture2D*>(m_albedoTexture.lock()->getHWTexture());
			if (pD3DTexture)
				pD3DTexture->GenerateMips();
		}


		// normal map loading
#if 0
		if (!m_normalTextureName.empty())
		{
			m_normalTexture = g_contentManager->LoadObject<TextureMap>(m_normalTextureName);
			m_normalTexture.lock()->getHWTexture()->SetDebugName(m_normalTextureName.c_str());
		}
#endif
		// REWRITE TO SAMPLER !!!
#if 0
		// if we dont skip mip mapping in the albedo texture when generate them
		if (!m_skipmips)
		{
			GLTexture2D* texture2DImpl = reinterpret_cast<GLTexture2D*>(m_albedoTexture->getHWTexture());
			glGenerateTextureMipmapEXT(texture2DImpl->getHandle(), GL_TEXTURE_2D);
		}

		if (!m_normalTextureName.empty())
			m_normalTexture = ContentManager::GetInstance()->LoadTexture(m_normalTextureName);

		// if we dont skip mip mapping in the albedo texture when generate them
		if (!m_skipmips && m_normalTexture)
		{
			GLTexture2D* texture2DImpl = reinterpret_cast<GLTexture2D*>(m_normalTexture->getHWTexture());
			glGenerateTextureMipmapEXT(texture2DImpl->getHandle(), GL_TEXTURE_2D);
		}

		if (!m_specularTextureName.empty())
			m_specularTexture = ContentManager::GetInstance()->LoadTexture(m_specularTextureName);

		// if we dont skip mip mapping in the albedo texture when generate them
		if (!m_skipmips && m_specularTexture)
		{
			GLTexture2D* texture2DImpl = reinterpret_cast<GLTexture2D*>(m_specularTexture->getHWTexture());
			glGenerateTextureMipmapEXT(texture2DImpl->getHandle(), GL_TEXTURE_2D);
		}
#endif
	}

	void Material::ReleaseHw()
	{
		if (m_albedoSampler)
		{
			mem_delete(m_albedoSampler);
			m_albedoSampler = nullptr;
		}
	}

	///////////////////////////////////////////////////////
	// Generic Material
	///////////////////////////////////////////////////////

	IMPLEMENT_OBJECT(GenericMaterial, Material);

	GenericMaterial::GenericMaterial()
	{
	}

	GenericMaterial::~GenericMaterial()
	{
	}

	void GenericMaterial::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<GenericMaterial>();
	}

	///////////////////////////////////////////////////////
	// Layered and masked material
	///////////////////////////////////////////////////////

	IMPLEMENT_OBJECT(LayeredMaskedMaterial, Material);

	LayeredMaskedMaterial::LayeredMaskedMaterial()
	{
	}

	LayeredMaskedMaterial::~LayeredMaskedMaterial()
	{
	}

	void LayeredMaskedMaterial::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<LayeredMaskedMaterial>();
	}

	///////////////////////////////////////////////////////
	// Null material
	///////////////////////////////////////////////////////

	IMPLEMENT_OBJECT(NullMaterial, Material);

	NullMaterial::NullMaterial()
	{
	}

	NullMaterial::~NullMaterial()
	{
	}

	void NullMaterial::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<NullMaterial>();
	}
}
