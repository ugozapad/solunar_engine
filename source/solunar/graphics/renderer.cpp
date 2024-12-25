#include "graphicspch.h"
#include "graphics/renderer.h"
#include "graphics/screenquad.h"
#include "graphics/gfxcommon.h"
#include "graphics/view.h"
#include "graphics/core/texture.h"
#include "graphics/core/device.h"
#include "graphics/core/rendertarget.h"
#include "graphics/rendercontext.h"
#include "graphics/core/statemanager.h"


// shaders
#include "graphics/ShaderProgramManager.h"

// materials
#include "graphics/materials/materialinstancefactory.h"

// render stuff
#include "graphics/shaderconstantmanager.h"
#include "graphics/shadowsrenderer.h"
#include "graphics/image.h"
#include "graphics/graphicsworld.h"
#include "graphics/lightmanager.h"
#include "graphics/debugrenderer.h"
#include "graphics/ifontmanager.h"
#include "graphics/fontmanager.h"
#include "graphics/mesh.h"
#include "graphics/light.h"
#include "graphics/ui/rmlsystem.h"
#include "graphics/postfxmanager.h"
#include "graphics/imguimanager.h"
#include "graphics/animatedmodel.h"

#include "engine/camera.h"
#include "engine/engine.h"
#include "engine/console.h"
#include "engine/entity/world.h"
#include "engine/entity/entitymanager.h"
#include "engine/entity/entity.h"

namespace solunar
{
	Renderer* g_renderer = nullptr;

	static std::weak_ptr<TextureMap> g_defaultTexture;
	static std::weak_ptr<TextureMap> g_notFoundTexture;
	static std::weak_ptr<Material> g_defaultMaterial;

	ISamplerState* g_defaultSampler = nullptr;
	IDepthStencilState* g_depthStencilState_NoWrite = nullptr;

	IDepthStencilState* g_depthStencilState_Default = nullptr;
	IRasterizerState* g_rasterizerState_Default = nullptr;

	void Command_ToggleRMode()
	{
		if (!g_renderer)
			return;

		RendererViewMode currentMode = g_renderer->GetRenderMode();

		RendererViewMode modeToSet;
		if (currentMode == RendererViewMode::Wireframe)
			modeToSet = RendererViewMode::Unlit;
		else if (currentMode == RendererViewMode::Unlit)
			modeToSet = RendererViewMode::Lit;
		else if (currentMode == RendererViewMode::Lit)
			modeToSet = RendererViewMode::Wireframe;

		g_renderer->SetRenderMode(modeToSet);
	}

	void Command_ShowPoly()
	{
		g_renderer->ToggleShowingWireframe();
	}

	void DrawLoadscreen()
	{
		std::weak_ptr<TextureMap> loadscreen = g_contentManager->LoadObject<TextureMap>("textures/ui/ui_loading_screen.png");

		View* view = CameraProxy::GetInstance()->GetView();

		Viewport viewport;
		viewport.m_x = 0;
		viewport.m_y = 0;
		viewport.m_width = view->m_width;
		viewport.m_height = view->m_height;
		g_renderDevice->SetViewport(&viewport);
		g_renderDevice->SetSamplerState(0, g_defaultSampler);

		g_stateManager->SetDepthStencilState(g_depthStencilState_NoWrite, 0);
		g_stateManager->SetRasterizerState(g_rasterizerState_Default);

		g_renderer->BeginFrame();
		ScreenQuad::Render(loadscreen.lock()->getHWTexture());
		g_renderer->EndFrame();
	}

	Renderer::Renderer()
	{
		m_meshPolysWireframe	= false;
		m_showOctree			= false;
		m_screenRenderTarget	= nullptr;
		m_screenDepthTexture	= nullptr;
		m_screenColorTexture	= nullptr;
	}

	Renderer::~Renderer()
	{

	}

	void Renderer::initInplaceResources()
	{
#if 0
		// check for shaders
		// #TODO: rewrite to content manager (should return nullptr on stream open failure)
		if (!g_fileSystem->Exist("data/shaders/dx11/quad.vsh") ||
			!g_fileSystem->Exist("data/shaders/dx11/quad.psh"))
		{
			Core::Error("Game files is corrupted or incomplete.");
		}
#endif

		SamplerDesc samplerDesc = {};
		samplerDesc.m_wrapS = TextureWrap::Repeat;
		samplerDesc.m_wrapT = TextureWrap::Repeat;
		samplerDesc.m_minFilter = TextureFilter::Nearest;
		samplerDesc.m_magFilter = TextureFilter::Nearest;
		samplerDesc.m_comparisonFunc = COMPARISON_ALWAYS;
		g_defaultSampler = g_renderDevice->CreateSamplerState(samplerDesc);

		DepthStencilDesc desc = {};
		desc.m_depthEnable = false;
		desc.m_depthWriteMask = DEPTH_WRITE_MASK_ALL;
		desc.m_depthFunc = COMPARISON_ALWAYS;
		desc.m_stencilEnable = false;
		desc.m_frontFace.m_stencilFailOp = desc.m_frontFace.m_stencilDepthFailOp = desc.m_frontFace.m_stencilPassOp = STENCIL_OP_KEEP;
		desc.m_frontFace.m_stencilFunc = COMPARISON_ALWAYS;
		desc.m_backFace = desc.m_frontFace;
		g_depthStencilState_NoWrite = g_stateManager->CreateDepthStencilState(desc);

		g_defaultMaterial = g_contentManager->LoadObject<Material>("materials/default_material.xml");







		RasterizerStateDesc rasterizerState;
		memset(&rasterizerState, 0, sizeof(rasterizerState));
		rasterizerState.m_cullMode = CullMode::Back;
		rasterizerState.m_frontCCW = true;
		rasterizerState.m_fillMode = FillMode::Solid;
		g_rasterizerState_Default = g_stateManager->CreateRasterizerState(rasterizerState);



		DepthStencilDesc depthStencilDesc;
		memset(&depthStencilDesc, 0, sizeof(depthStencilDesc));

		// Depth test parameters
		depthStencilDesc.m_depthEnable = true;
		depthStencilDesc.m_depthWriteMask = DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.m_depthFunc = COMPARISON_LESS;

		// Stencil test parameters
		depthStencilDesc.m_stencilEnable = true;
		depthStencilDesc.m_stencilReadMask = 0xFF;
		depthStencilDesc.m_stencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing
		depthStencilDesc.m_frontFace.m_stencilFailOp = STENCIL_OP_KEEP;
		depthStencilDesc.m_frontFace.m_stencilDepthFailOp = STENCIL_OP_INCR;
		depthStencilDesc.m_frontFace.m_stencilPassOp = STENCIL_OP_KEEP;
		depthStencilDesc.m_frontFace.m_stencilFunc = COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing
		depthStencilDesc.m_backFace.m_stencilFailOp = STENCIL_OP_KEEP;
		depthStencilDesc.m_backFace.m_stencilDepthFailOp = STENCIL_OP_DECR;
		depthStencilDesc.m_backFace.m_stencilPassOp = STENCIL_OP_KEEP;
		depthStencilDesc.m_backFace.m_stencilFunc = COMPARISON_ALWAYS;

		g_depthStencilState_Default = g_stateManager->CreateDepthStencilState(depthStencilDesc);

		// Load textures
		//g_defaultTexture = g_contentManager->LoadObject<TextureMap>("textures/default.png");
		//g_notFoundTexture = g_contentManager->LoadObject<TextureMap>("textures/notexture.png");
	}

	void Renderer::initFramebuffer(View* view)
	{
		TextureDesc colorTextureDesc;
		memset(&colorTextureDesc, 0, sizeof(colorTextureDesc));
		colorTextureDesc.m_width = view->m_width;
		colorTextureDesc.m_height = view->m_height;
		colorTextureDesc.m_format = ImageFormat::RGBA16F;
		colorTextureDesc.m_renderTargetUsage = true;

		SubresourceDesc colorTextureSubresourceDesc;
		memset(&colorTextureSubresourceDesc, 0, sizeof(colorTextureSubresourceDesc));

		m_screenColorTexture = g_renderDevice->CreateTexture2D(colorTextureDesc, colorTextureSubresourceDesc);
		m_screenColorTexture->SetDebugName("Screen Color Texture");

		TextureDesc depthTextureDesc;
		memset(&depthTextureDesc, 0, sizeof(depthTextureDesc));
		depthTextureDesc.m_width = view->m_width;
		depthTextureDesc.m_height = view->m_height;
		depthTextureDesc.m_format = ImageFormat::DEPTH24;
		depthTextureDesc.m_renderTargetUsage = true;

		SubresourceDesc depthTextureSubresourceDesc;
		memset(&depthTextureSubresourceDesc, 0, sizeof(depthTextureSubresourceDesc));

		m_screenDepthTexture = g_renderDevice->CreateTexture2D(depthTextureDesc, depthTextureSubresourceDesc);
		m_screenDepthTexture->SetDebugName("Screen Depth Texture");

		RenderTargetCreationDesc renderTargetDesc;
		memset(&renderTargetDesc, 0, sizeof(renderTargetDesc));
		renderTargetDesc.m_depthTexture2D = m_screenDepthTexture;
		renderTargetDesc.m_textures2D[0] = m_screenColorTexture;
		renderTargetDesc.m_textures2DCount = 1;
		m_screenRenderTarget = g_renderDevice->CreateRenderTarget(renderTargetDesc);
		m_screenRenderTarget->SetDebugName("Screen Render Target");
	}

	void Renderer::Init()
	{
		ConsoleCommandManager::GetInstance()->RegisterCommand("togglermode", &Command_ToggleRMode);
		ConsoleCommandManager::GetInstance()->RegisterCommand("showpoly", &Command_ShowPoly);

		View* view = CameraProxy::GetInstance()->GetView();

		// Initialize default framebuffer
		initFramebuffer(view);

		// shader manager is inited
		initInplaceResources();

		ImGuiManager::GetInstance()->Init();

		// set as lit
		SetRenderMode(RendererViewMode::Lit);

		RenderContext::Init();

		ShaderConstantManager::GetInstance()->Init();

		ScreenQuad::Init();

		// Draw loading screen
		DrawLoadscreen();

		// Initialize font manager
		g_fontManager = mem_new<FontManager>();
		g_fontManager->Initialize();

		// Initialize material factory
		MaterialInstanceFactory::CreateInstance();

		g_debugRender.Initialize();

		// Initialize ui renderer
		// RmlSystem::GetInstance()->Init();
		// RmlSystem::GetInstance()->createContext(view->getWidth(), view->getHeight());

		//initFramebuffer(CameraProxy::GetInstance()->GetView());

		g_postFxManager.Init(CameraProxy::GetInstance()->GetView());

		AnimatedModelRenderer::GetInstance()->Init();

		ShadowsRenderer::GetInstance()->Init();
	}

	void Renderer::Shutdown()
	{
		// Release inplace resources

		ShadowsRenderer::GetInstance()->Shutdown();

		AnimatedModelRenderer::GetInstance()->Shutdown();

		g_postFxManager.Shutdown();

		g_debugRender.Shutdown();

//		RmlSystem::GetInstance()->Shutdown();

		if (m_screenRenderTarget)
		{
			mem_delete(m_screenRenderTarget);
			m_screenRenderTarget = nullptr;
		}

		if (m_screenDepthTexture)
		{
			mem_delete(m_screenDepthTexture);
			m_screenDepthTexture = nullptr;
		}

		if (m_screenColorTexture)
		{
			mem_delete(m_screenColorTexture);
			m_screenColorTexture = nullptr;
		}

		//D3D11: DeviceChild reference counter underflow.Release should not be called on objects with zero reference count.
		//Exception thrown at 0x00007FFE0F6EAB89 (KernelBase.dll) in solunar_main_d.exe : 0x0000087D (parameters : 0x0000000000000000, 0x000000F2E33CD930, 0x000000F2E33CD950).
		//Unhandled exception at 0x00007FFE0F6EAB89 (KernelBase.dll) in solunar_main_d.exe : 0x0000087D (parameters : 0x0000000000000000, 0x000000F2E33CD930, 0x000000F2E33CD950).

		//g_stateManager->DestroyDepthStencilState(g_depthStencilState_NoWrite);
		//g_depthStencilState_NoWrite = nullptr;

		mem_delete(g_defaultSampler);
		g_defaultSampler = nullptr;

		//m_postProcessingRenderer->Shutdown();
		//m_postProcessingRenderer = nullptr;

		MaterialInstanceFactory::DestroyInstance();

		// shutdown font manager
		g_fontManager->Shutdown();
		mem_delete(g_fontManager);

		ScreenQuad::Shutdown();

	//	ContentManager::GetInstance()->ReleaseGraphicsObjects();

		ShaderConstantManager::GetInstance()->Shutdown();

		RenderContext::Shutdown();

		ImGuiManager::GetInstance()->Shutdown();
	}

	void Renderer::BeginFrame()
	{
		//OPTICK_EVENT("Renderer::BeginFrame");

		setSwapChainRenderTarget();

		clearScreen();
	}

	void Renderer::RenderWorld(View* view)
	{
		//OPTICK_EVENT("Renderer::RenderWorld");

		RenderShadows(view);
	
		g_renderDevice->SetRenderTarget(m_screenRenderTarget);
		ClearRenderTarget(m_screenRenderTarget);

		// get camera
		Camera* camera = CameraProxy::GetInstance();

		World* world = Engine::ms_world;
		if (world)
		{
			const std::vector<MeshComponent*>& drawableEntities = world->GetGraphicsWorld()->GetMeshes();

			// color pass
			for (auto meshComponent : drawableEntities)
			{
				Entity* entity = meshComponent->GetEntity();
				if (entity)
				{
					// setup render context
					RenderContext& renderCtx = RenderContext::GetContext();
					renderCtx.model = entity->GetWorldTranslation();
					RenderContext::SetContext(renderCtx);

					if (meshComponent->IsA(AnimatedMeshComponent::GetStaticTypeInfo()))
						AnimatedModelRenderer::GetInstance()->Render((AnimatedMeshComponent*)meshComponent);

					// call render function
					RenderMesh(world->GetGraphicsWorld(), view, meshComponent);

					meshComponent->Render();
				}
			}
		}

		g_debugRender.RenderFrame(view);
	}

	void Renderer::RenderShadows(View* view)
	{
		//OPTICK_EVENT("Renderer::RenderShadows");

		return;

		ShadowsRenderer::GetInstance()->BeginRender();

		// get camera
		Camera* camera = CameraProxy::GetInstance();

		World* world = Engine::ms_world;
		if (world)
		{
			const std::vector<MeshComponent*>& drawableEntities = world->GetGraphicsWorld()->GetMeshes();

			// color pass
			for (auto meshComponent : drawableEntities)
			{
				if (!meshComponent->IsCastingShadow())
					continue;

				Entity* entity = meshComponent->GetEntity();
				if (entity)
				{
					ShadowsRenderer::GetInstance()->RenderMesh(
						world->GetGraphicsWorld(),
						view,
						meshComponent);
				}
			}
		}

		ShadowsRenderer::GetInstance()->EndRender();
	}

	void Renderer::RenderView(View* view)
	{
		// OPTICK_EVENT("Renderer::RenderView");

		// make context current
	/*	glfwMakeContextCurrent(GraphicsDevice::GetInstance()->getWindow());*/

		// set viewport
		Viewport vp = {};
		vp.m_width = view->m_width;
		vp.m_height = view->m_height;
		g_renderDevice->SetViewport(&vp);

		// Initialize render context
		RenderContext renderContext = RenderContext::GetContext();
		renderContext.width = view->m_width;
		renderContext.height = view->m_height;
		renderContext.proj = view->m_projection;
		renderContext.view = view->m_view;
		renderContext.model = glm::mat4(1.0f);

		RenderContext::SetContext(renderContext);

		RenderWorld(view);

		// post processing
		g_postFxManager.HDRPass(m_screenColorTexture);

		// reset to swap chain
		setSwapChainRenderTarget();

		g_stateManager->SetDepthStencilState(g_depthStencilState_NoWrite, 0);

		// draw
		//ScreenQuad::Render( m_screenColorTexture );

		//ScreenQuad::Render(ShadowsRenderer::GetInstance()->GetTexture());

		g_fontManager->FlushPrimitives();

		// draw debug renderer
	//	g_debugRender.RenderFrame(view);
	}

	void Renderer::RenderSky(View* view, SkyMeshComponent* skyMesh)
	{
#if 0
		RenderContext& renderCtx = RenderContext::GetContext();
		//renderCtx.model = CameraProxy::GetInstance()->GetCameraComponent();
		RenderContext::SetContext(renderCtx);
		skyMesh->Render();
#endif
	}

	void UpdateDirectionalLight(DirectionalLightComponent* directionalLight)
	{
		if (!directionalLight)
			return;

		DirectionalLightCB* data = (DirectionalLightCB*)g_directionalLightConstantBuffer->Map(BufferMapping::WriteOnly);
		data->m_ambientColor = glm::vec4(directionalLight->m_ambientColor, 1.0f);
		data->m_color = glm::vec4(directionalLight->m_color, 1.0f);

		Assert(directionalLight->GetEntity());

		// convert to euler and reorder direction of the light
#if 0
		glm::quat quaternion = directionalLight->GetEntity()->GetRotation();
		glm::vec3 euler = glm::eulerAngles(quaternion) * 3.14159f / 180.f;
		data->m_direction = glm::vec4(euler.y, euler.x, euler.z, 1.0f);
#else
		glm::quat o = directionalLight->GetEntity()->GetRotation();
		glm::vec3 V;
		V[0] = 2 * (o.x * o.z - o.w * o.y);
		V[1] = 2 * (o.y * o.z + o.w * o.x);
		V[2] = 1 - 2 * (o.x * o.x + o.y * o.y);
		data->m_direction = glm::vec4(-V, 1.0f);

#if 0//ndef FINAL_BUILD
		g_debugRender.DrawLine(directionalLight->GetEntity()->GetPosition(),
			directionalLight->GetEntity()->GetPosition() + V, glm::vec3(1.0f, 1.0f, 0.0f));
#endif // !FINAL_BUILD

#endif
		//	data->m_direction = glm::vec4(glm::radians(directionalLight->m_direction), 1.0f);

		g_directionalLightConstantBuffer->Unmap();

		g_renderDevice->SetConstantBufferIndex(CBBindings_DirectionalLight, g_directionalLightConstantBuffer.get());
	}

	void UpdatePointLightCB(LightManager* lightMgr)
	{
		const std::vector<LightComponent*>& lights = lightMgr->GetLights();
		const std::vector<PointLightComponent*>& pointLights = lightMgr->GetPointLights();

		if (!pointLights.empty())
		{
			PointLightCB* pointLightData = (PointLightCB*)g_pointLightConstantBuffer->Map(BufferMapping::WriteOnly);
			for (int i = 0; i < pointLights.size(); i++)
			{
				auto it = pointLights[i];
				pointLightData->pointLights[i].color = glm::vec4(it->m_color, 1.0f);
				pointLightData->pointLights[i].position = glm::vec4(it->GetEntity()->GetPosition(), 1.0f);
				pointLightData->pointLights[i].specular = glm::vec4(it->m_specularColor, 1.0f);
				pointLightData->pointLights[i].lightData.r = it->m_radius;
			}

			g_pointLightConstantBuffer->Unmap();
			pointLightData = nullptr;
		}

		g_renderDevice->SetConstantBufferIndex(CBBindings_PointLights, g_pointLightConstantBuffer.get());	
	}

	void UpdateSpotLightCB(LightManager* lightMgr)
	{
		const std::vector<LightComponent*>& lights = lightMgr->GetLights();
		const std::vector<SpotLightComponent*>& spotLights = lightMgr->GetSpotLights();

		if (!spotLights.empty())
		{
			SpotLightCB* spotLightData = (SpotLightCB*)g_spotLightConstantBuffer->Map(BufferMapping::WriteOnly);
			for (int i = 0; i < spotLights.size(); i++)
			{
				auto it = spotLights[i];
				spotLightData->spotLights[i].color = glm::vec4(it->m_color, 1.0f);
				spotLightData->spotLights[i].position = glm::vec4(it->GetEntity()->GetWorldPosition(), 1.0f);
				spotLightData->spotLights[i].specular = glm::vec4(it->m_specularColor, 1.0f);
				spotLightData->spotLights[i].lightData.r = glm::radians(it->m_cutoff);

				// #TODO STUPID REFACTOR
				glm::quat o = it->GetEntity()->GetRotation();
				glm::vec3 V;
				V[0] = 2 * (o.x * o.z - o.w * o.y);
				V[1] = 2 * (o.y * o.z + o.w * o.x);
				V[2] = 1 - 2 * (o.x * o.x + o.y * o.y);

				spotLightData->spotLights[i].lightData.g = V.x;
				spotLightData->spotLights[i].lightData.b = V.y;
				spotLightData->spotLights[i].lightData.a = V.z;
			}

			g_spotLightConstantBuffer->Unmap();
			spotLightData = nullptr;
		}

		g_renderDevice->SetConstantBufferIndex(CBBindings_SpotLights, g_spotLightConstantBuffer.get());
	}

	void UpdateLightDataCB(LightManager* lightMgr)
	{
		const std::vector<PointLightComponent*>& pointLights = lightMgr->GetPointLights();
		const std::vector<SpotLightComponent*>& spotLights = lightMgr->GetSpotLights();

		LightGlobalDataCB* lightGlobalData = (LightGlobalDataCB*)g_lightDataConstantBuffer->Map(BufferMapping::WriteOnly);
		lightGlobalData->m_pointLightCount = (uint32_t)pointLights.size();
		lightGlobalData->m_spotLightCount = (uint32_t)spotLights.size();
		g_lightDataConstantBuffer->Unmap();

		g_renderDevice->SetConstantBufferIndex(CBBindings_LightData, g_lightDataConstantBuffer.get());
	}

	void Renderer::SetupLights(GraphicsWorld* graphicsWorld)
	{
		LightManager* lightManager = graphicsWorld->GetLightManager();
		if (!lightManager)
			return;

		UpdateDirectionalLight(lightManager->GetDirectionalLight());
		UpdatePointLightCB(lightManager);
		UpdateSpotLightCB(lightManager);
		UpdateLightDataCB(lightManager);
	}

	void Renderer::EndFrame()
	{
//		OPTICK_EVENT("Renderer::EndFrame");

		// resetDeviceState();
	}

	void Renderer::ToggleShowingWireframe()
	{
		if (m_currentViewMode == RendererViewMode::Wireframe) {
			m_meshPolysWireframe = false;
		} else {
			m_meshPolysWireframe = !m_meshPolysWireframe;
		}
	}

	void Renderer::ToggleShowOctree()
	{
		m_showOctree = !m_showOctree;
	}

	void Renderer::RenderLoadscreen()
	{
		// Draw loading screen
		DrawLoadscreen();
	}

	void Renderer::SetDefaultRenderState()
	{
		g_stateManager->SetDepthStencilState(g_depthStencilState_Default, 0);
		g_stateManager->SetRasterizerState(g_rasterizerState_Default);
	}

	std::weak_ptr<Material> GetDefaultMaterial()
	{
		return g_defaultMaterial;
	}

}