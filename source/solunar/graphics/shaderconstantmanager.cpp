#include "graphicspch.h"
#include "graphics/shaderconstantmanager.h"

#include "core/timer.h"

#include "engine/engine.h"
#include "engine/camera.h"
#include "engine/entity/world.h"

#include "graphics/core/device.h"
#include "graphics/rendercontext.h"
#include "graphics/shaderprogram.h"
#include "graphics/imguimanager.h"
#include "graphics/graphicsworld.h"
#include "graphics/light.h"
#include "graphics/shadowsrenderer.h"

namespace solunar
{
	ConstantBufferProxy g_staticMeshConstantBuffer;
	ConstantBufferProxy g_directionalLightConstantBuffer;
	ConstantBufferProxy g_pointLightConstantBuffer;
	ConstantBufferProxy g_spotLightConstantBuffer;
	ConstantBufferProxy g_lightDataConstantBuffer;

	ConstantBufferProxy::ConstantBufferProxy(IBufferBase* buffer)
	{
		Assert(buffer);
		m_buffer = buffer;
	}

	ConstantBufferProxy::ConstantBufferProxy(IBufferBase* buffer, const std::string& name)
	{
		Assert(buffer);
		Assert(name.empty());
		m_buffer = buffer;
		m_name = name;
	}

	ConstantBufferProxy::ConstantBufferProxy(const ConstantBufferProxy& proxy)
	{
		Assert(proxy.m_buffer);
		Assert(proxy.m_name.empty());
		m_buffer = proxy.m_buffer;
		m_name = proxy.m_name;
	}

	ConstantBufferProxy::~ConstantBufferProxy()
	{
		m_buffer = nullptr;
	}

	IBufferBase* ConstantBufferProxy::get()
	{
		return m_buffer;
	}

	IBufferBase* ConstantBufferProxy::operator->()
	{
		return m_buffer;
	}

	std::string ConstantBufferProxy::name()
	{
		return m_name;
	}

	// ShaderConstantManager implementation

	static ShaderConstantManager s_shaderConstantManager;

	ShaderConstantManager::ShaderConstantManager()
	{
	}

	ShaderConstantManager::~ShaderConstantManager()
	{
	}

	void ShaderConstantManager::Init()
	{
		Core::Msg("ShaderConstantManager: Initialize constant buffers ...");

		g_staticMeshConstantBuffer = Create<StaticMeshGlobalData>("StaticMeshGlobalData");
		g_directionalLightConstantBuffer = Create<DirectionalLightCB>("DirectionalLightCB");
		g_pointLightConstantBuffer = Create<PointLightCB>("PointLightCB");
		g_spotLightConstantBuffer = Create<SpotLightCB>("SpotLightCB");
		g_lightDataConstantBuffer = Create<LightGlobalDataCB>("LightGlobalDataCB");
	}

	void ShaderConstantManager::Shutdown()
	{
		for (auto it : m_constantBuffers)
		{
			Core::Msg("ShaderConstantManager: releasing constant buffer %s", it.first.c_str());
			mem_delete(it.second);
		}

		m_constantBuffers.clear();
	}

	ConstantBufferProxy ShaderConstantManager::get(const std::string& name)
	{
		auto it = m_constantBuffers.find(name);
		if (it != m_constantBuffers.end())
		{
			return ConstantBufferProxy(it->second);
		}

		return ConstantBufferProxy(nullptr);
	}


	void ShaderConstantManager::SetGlobalData(MeshComponent* meshComponent, View* view, RenderContext& renderContext, GraphicsWorld* graphicsWorld)
	{
		// OPTICK_EVENT("ShaderConstantManager::SetGlobalData");

		StaticMeshGlobalData* globalData = (StaticMeshGlobalData*)g_staticMeshConstantBuffer->Map(BufferMapping::WriteOnly);
		globalData->m_modelMatrix = renderContext.model;
		globalData->m_viewMatrix = renderContext.view;
		globalData->m_projectionMatrix = renderContext.proj;

		glm::mat4 modelViewProjection		= glm::mat4(1.0f);
		modelViewProjection					= renderContext.proj * renderContext.view * renderContext.model;

		globalData->m_modelViewProjection	= modelViewProjection;

		globalData->m_inverseModelMatrix	= glm::transpose(glm::inverse(glm::mat3(renderContext.model)));

		globalData->m_LightView				= ShadowsRenderer::GetInstance()->GetLightViewMatrix();
		globalData->m_LightViewProjection	= ShadowsRenderer::GetInstance()->GetLightViewProjection();
		globalData->m_inverseViewProjection = glm::inverse(renderContext.proj * renderContext.view);

		Camera* camera = CameraProxy::GetInstance();
		globalData->m_viewPos = glm::vec4(camera->GetPosition(), 0.0f);
		globalData->m_viewDir = glm::vec4(camera->GetDirection(), 0.0f);

		g_staticMeshConstantBuffer->Unmap();
		g_renderDevice->SetConstantBufferIndex(0, g_staticMeshConstantBuffer.get());
	}

	void ShaderConstantManager::getConstantBuffers(std::unordered_map<std::string, IBufferBase*>& map)
	{
		map = m_constantBuffers;
	}

	void graphicsShowConstantBuffers(bool* open)
	{
		if (ImGui::Begin("Constant Buffer Tracker", open))
		{
			std::unordered_map<std::string, IBufferBase*> constantBufferTable;
			ShaderConstantManager::GetInstance()->getConstantBuffers(constantBufferTable);
	
				//HelpMarker(
				//	"Here we activate ScrollY, which will create a child window container to allow hosting scrollable contents.\n\n"
				//	"We also demonstrate using ImGuiListClipper to virtualize the submission of many items.");
				static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

			//	ImGui::CheckboxFlags("ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
		
				// When using ScrollX or ScrollY we need to specify a size for our table container!
				// Otherwise by default the table will fit all available space, like a BeginChild() call.
			//	ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 8);
				if (ImGui::BeginTable("table_scrolly", 2, flags))
				{
					ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None);
					ImGui::TableHeadersRow();

					for (auto& it : constantBufferTable)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%s", it.first.c_str());
						ImGui::TableSetColumnIndex(1);

						BufferDesc desc = it.second->getBufferDesc();
						ImGui::Text("%i", desc.m_bufferMemorySize);
					}

					ImGui::EndTable();
				}	
		}

		ImGui::End();
	}

}