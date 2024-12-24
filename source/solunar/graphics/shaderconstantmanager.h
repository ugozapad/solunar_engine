#ifndef SHADERCONSTANTMANAGER_H
#define SHADERCONSTANTMANAGER_H

#include "graphics/view.h"
#include "graphics/rendercontext.h"
//#include "graphics/light.h"
//#include "graphics/staticmeshcomponent.h"
#include "graphics/rendercontext.h"
#include "graphics/graphicsworld.h"

namespace solunar
{
	enum class ShaderConstantType
	{
		Int,
		Float,
		Vector2,
		Vector3,
		Vector4,
		Matrix3x3,
//		Matrix3x4,
		Matrix4x4,
	};

	enum ConstantBufferBindings
	{
		CBBindings_Scene,
		CBBindings_Skinning,
		CBBindings_DirectionalLight,
		CBBindings_PointLights,
		CBBindings_SpotLights,
		CBBindings_LightData,
	};

	struct StaticMeshGlobalData
	{
		glm::mat4 m_modelMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_modelViewProjection;
		glm::mat4 m_inverseModelMatrix;
		glm::mat4 m_LightView;
		glm::mat4 m_LightViewProjection;
		glm::mat4 m_inverseViewProjection;

		glm::vec4 m_viewPos;
		glm::vec4 m_viewDir;
	};

	class IShaderProgram;
	class MeshComponent;

	class ConstantBufferProxy
	{
	public:
		ConstantBufferProxy() = default;
		ConstantBufferProxy(IBufferBase* buffer);
		ConstantBufferProxy(IBufferBase* buffer, const std::string& name);
		ConstantBufferProxy(const ConstantBufferProxy& proxy);
		
		~ConstantBufferProxy();

		IBufferBase* get();
		IBufferBase* operator->();

		std::string name();

	private:
		std::string m_name;
		IBufferBase* m_buffer = nullptr;
	};

	extern ConstantBufferProxy g_staticMeshConstantBuffer;
	extern ConstantBufferProxy g_directionalLightConstantBuffer;
	extern ConstantBufferProxy g_pointLightConstantBuffer;
	extern ConstantBufferProxy g_spotLightConstantBuffer;
	extern ConstantBufferProxy g_lightDataConstantBuffer;

	class ShaderConstantManager : public Singleton<ShaderConstantManager>
	{
	public:
		ShaderConstantManager();
		~ShaderConstantManager();

		void Init();
		void Shutdown();

		template <typename BufferStructureData>
		ConstantBufferProxy Create(const std::string& name);
	
		ConstantBufferProxy get(const std::string& name);

		void SetGlobalData(MeshComponent* meshComponent, View* view, RenderContext& renderContext, GraphicsWorld* graphicsWorld);

		void getConstantBuffers(std::unordered_map<std::string, IBufferBase*>& map);

	private:
		std::unordered_map<std::string, IBufferBase*> m_constantBuffers;
	};

	template<typename BufferStructureData>
	inline ConstantBufferProxy ShaderConstantManager::Create(const std::string& name)
	{
		// Create constant buffer

		BufferDesc bufferDesc = {};
		bufferDesc.m_bufferType = BufferType::ConstantBuffer;
		bufferDesc.m_bufferAccess = BufferAccess::Stream;
		bufferDesc.m_bufferMemorySize = sizeof(BufferStructureData);

		SubresourceDesc subresourceDesc = {};

		IBufferBase* buffer = g_renderDevice->CreateBuffer(bufferDesc, subresourceDesc);

		m_constantBuffers.emplace(name, buffer);

		Core::Msg("ShaderConstantManager: created constant buffer %s (%i bytes)", name.c_str(), bufferDesc.m_bufferMemorySize);

		return ConstantBufferProxy(buffer);
	}

	void graphicsShowConstantBuffers(bool* open);

}

#endif // !SHADERCONSTANTMANAGER_H
