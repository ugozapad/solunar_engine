#include "renderer.h"
#include "d3d11drv_pch.h"
#include "d3d11drv/d3d11renderer.h"
#include "d3d11drv/d3d11device.h"
#include "d3d11drv/d3d11shaderprogrammanager.h"
#include "d3d11drv/d3d11statemanager.h"
#include "d3d11drv/d3d11rendertarget.h"

// filesystem
#include "core/file/filesystem.h"

// Base graphics classes
#include "graphics/view.h"
#include "graphics/rendercontext.h"

// Graphics objects
#include "graphics/image.h"
#include "graphics/mesh.h"
#include "graphics/animatedmodel.h"
#include "graphics/material.h"
#include "graphics/materials/materialinstance.h"
#include "graphics/materials/materialinstance_generic.h"

// Graphics managers
#include "graphics/ShaderProgramManager.h"
#include "graphics/shaderconstantmanager.h"
#include "graphics/shadowsrenderer.h"

#include "main/main.h"

#include <d3d9.h>
#include <dxgidebug.h>

#pragma comment(lib, "d3d9.lib")

namespace solunar
{

void createRenderer()
{
	g_renderer = mem_new<D3D11Renderer>();
}

void destroyRenderer()
{
	if (g_renderer)
	{
		mem_delete(g_renderer);
		g_renderer = nullptr;
	}
}

void reportRenderDevice()
{
	IDXGIDevice* dxgiDevice = nullptr;
	D3D11_CHECK(g_d3d11Device->GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = nullptr;
	D3D11_CHECK(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	DXGI_ADAPTER_DESC adapterDesc;
	dxgiAdapter->GetDesc(&adapterDesc);

	Core::Msg("D3D11RenderDevice: GPU: %S, Video Memory: %zu MB",
		adapterDesc.Description,
		adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Release stuff
	dxgiAdapter->Release();
	dxgiDevice->Release();
}

IRasterizerState* g_wireframeRasterizerState = nullptr;

void CreateWireframeRasterizerState()
{
	RasterizerStateDesc rasterizerStateDesc;
	memset(&rasterizerStateDesc, 0, sizeof(rasterizerStateDesc));
	rasterizerStateDesc.m_cullMode = CullMode::Back;
	rasterizerStateDesc.m_frontCCW = true;
	rasterizerStateDesc.m_fillMode = FillMode::Wireframe;
	g_wireframeRasterizerState = g_stateManager->CreateRasterizerState(rasterizerStateDesc);
}

D3D11Renderer::D3D11Renderer() :
	m_renderTargetView(nullptr),
	m_depthStencilTexture(nullptr),
	m_depthStencilView(nullptr),
	m_depthStencilState(nullptr),
	m_swapChain(nullptr),
	m_takeScreenshot(false)
{
}

D3D11Renderer::~D3D11Renderer()
{
	m_takeScreenshot = false;
}

void D3D11Renderer::Init()
{
	Core::Msg("D3D11Renderer: Creating rendering device");

	// Initialize render device
	g_renderDevice = (IRenderDevice*)mem_new<D3D11Device>();
	((D3D11Device*)g_renderDevice)->Create();

	// report about device
	reportRenderDevice();

	// initalize swap chain
	createSwapChain();

	// Initialize device state manager
	g_stateManager = mem_new<D3D11StateManager>();
	g_d3d11StateManager->Init();

	// Initialize shader manager with current api
	g_shaderManager = mem_new<D3D11ShaderProgramManager>();
	g_shaderManager->Init("shaders/d3d11");

	// Initialize base renderer
	Renderer::Init();

	// Initialize stuff
	CreateWireframeRasterizerState();
}

void D3D11Renderer::createSwapChain()
{
	D3D11Device* device = ((D3D11Device*)g_renderDevice);

	IDXGIDevice* dxgiDevice;
	device->GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);

	IDXGIAdapter* dxgiAdapter;
	HRESULT hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

	IDXGIFactory* dxgiFactory;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	HWND windowHandle = (HWND)appGetWindow();

	RECT rc = {};
	GetClientRect(windowHandle, &rc);
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	memset(&swapChainDesc, 0, sizeof(swapChainDesc));
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = windowHandle;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	D3D11_CHECK(dxgiFactory->CreateSwapChain(device->GetDevice(), &swapChainDesc, &m_swapChain));

	ID3D11Texture2D* backBuffer;
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	D3D11_CHECK(device->GetDevice()->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView));
	backBuffer->Release();
	backBuffer = nullptr;

	dxgiFactory->Release();
	dxgiAdapter->Release();
	dxgiDevice->Release();

	// Create depth stencil texture & view

	D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
	memset(&depthStencilTextureDesc, 0, sizeof(depthStencilTextureDesc));
	depthStencilTextureDesc.Width = width;
	depthStencilTextureDesc.Height = height;
	depthStencilTextureDesc.MipLevels = 1;
	depthStencilTextureDesc.ArraySize = 1;
	depthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilTextureDesc.SampleDesc.Count = 1;
	depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	D3D11_CHECK(device->GetDevice()->CreateTexture2D(&depthStencilTextureDesc, NULL, &m_depthStencilTexture));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	memset(&depthStencilViewDesc, 0, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	D3D11_CHECK(device->GetDevice()->CreateDepthStencilView(m_depthStencilTexture, &depthStencilViewDesc, &m_depthStencilView));

	// Create proxy render target for swap chain
	m_swapChainRenderTarget = mem_new<D3D11RenderTarget>(m_renderTargetView, m_depthStencilView);

	// Initialize target
	device->GetDeviceContext()->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	// Create depth stencil state
	// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	memset(&depthStencilDesc, 0, sizeof(depthStencilDesc));

	// Depth test parameters
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	D3D11_CHECK(device->GetDevice()->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState));

	// Set depth stencil state
	device->GetDeviceContext()->OMSetDepthStencilState(m_depthStencilState, 0);
}

void D3D11Renderer::Shutdown()
{
	Renderer::Shutdown();

	if (g_shaderManager)
	{
		mem_delete(g_shaderManager);
		g_shaderManager = nullptr;
	}

	g_d3d11StateManager->Shutdown();

	if (m_swapChainRenderTarget)
	{
		mem_delete(m_swapChainRenderTarget);
		m_swapChainRenderTarget = nullptr;
	}

	if (m_depthStencilState)
	{
		m_depthStencilState->Release();
		m_depthStencilState = nullptr;
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = nullptr;
	}

	if (m_depthStencilTexture)
	{
		m_depthStencilTexture->Release();
		m_depthStencilTexture = nullptr;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	mem_delete(g_stateManager);
	g_stateManager = nullptr;

	mem_delete(g_renderDevice);
	g_renderDevice = nullptr;
}

void D3D11Renderer::EndFrame()
{
	//OPTICK_EVENT("D3D11Renderer::EndFrame");

	if (m_takeScreenshot)
		takeScreenshotInternal();

	Renderer::EndFrame();

	m_swapChain->Present(0, 0);
}

void D3D11Renderer::BindMaterialForMesh(MeshComponent* mesh, Material* material, IMaterialInstance* materialInstance)
{
	// OPTICK_EVENT("D3D11Renderer::BindMaterialForMesh");

	Assert(mesh);
	Assert(material);
	Assert(materialInstance);

#if 1
	// bind material samplers
	material->bind();

	// Initialize shader
	uint32_t pixelVariation = 0;
	if (material->m_selfillum || g_renderer->GetRenderMode() == RendererViewMode::Unlit)
		pixelVariation |= PixelVariation_Unlit;
	else
	{
		pixelVariation |= PixelVariation_Lit;

		if (!material->m_normalTexture.expired())
			pixelVariation |= PixelVariation_Normalmap;
	}
	
	if (GetRenderMode() == RendererViewMode::Wireframe)
		pixelVariation = PixelVariation_Wireframe;

	IShaderProgram* shaderProgram = nullptr;

	if (mesh->IsA<AnimatedMeshComponent>() && dynamicCast<AnimatedMeshComponent>(mesh)->HasSkin())
		shaderProgram = materialInstance->getShaderProgramVariation(VertexFactory_SkinnedMesh, pixelVariation);
	else
		shaderProgram = materialInstance->getShaderProgramVariation(VertexFactory_StaticMesh, pixelVariation);

	Assert2(shaderProgram, "Unknowed mesh component type!");

	// bind material instance shader and material uniforms
	g_shaderManager->SetShaderProgram(shaderProgram);
#endif

#if 0
	// bind material samplers
	material->bind();

	// Initialize shader
	IShaderProgram* shaderProgram = nullptr;
	
	//if (mesh->IsA<StaticMeshComponent>())
		shaderProgram = materialInstance->getStaticMeshShaderProgram();

	Assert2(shaderProgram, "Unknowed mesh component type!");

	// bind material instance shader and material uniforms
	g_shaderManager->SetShaderProgram(shaderProgram);

	material->bindUniformsCustom(shaderProgram);

	// bind point lights
//	if (StaticMeshComponent* staticMesh = dynamicCast<StaticMeshComponent>(mesh))
//		ShaderConstantManager::GetInstance()->SetGlobalData(staticMesh, view, )

	//shaderProgram->setInteger("u_lightsCount", mesh->m_world->getWorldComponent<GraphicsWorld>()->getLightManager()->getLights().size());
	//ShaderConstantManager::GetInstance()->setPointLightConstantBuffer();

#endif
}

void D3D11Renderer::RenderMesh(GraphicsWorld* graphicsWorld, View* view, MeshComponent* mesh)
{
	// OPTICK_EVENT("D3D11Renderer::RenderMesh");

	// Set depth stencil state
	g_d3d11Device->GetDeviceContext()->OMSetDepthStencilState(m_depthStencilState, 0);
	
	SetDefaultRenderState();
	
	// set shadowmap

	const int kShadowMapIndex = 5;
	g_renderDevice->SetTexture2D(kShadowMapIndex, ShadowsRenderer::GetInstance()->GetTexture());
	g_renderDevice->SetSamplerState(kShadowMapIndex, ShadowsRenderer::GetInstance()->GetSamplerState());

	//g_stateManager->SetRasterizerState(m_rasterizerState);

	// setup lights
	SetupLights(graphicsWorld);

	if (mesh->IsA<AnimatedMeshComponent>())
		RenderAnimatedMesh(graphicsWorld, view, mesh);
	else
		RenderStaticMesh(graphicsWorld, view, mesh);
}

void D3D11Renderer::RenderStaticMesh(GraphicsWorld* graphicsWorld, View* view, MeshComponent* mesh)
{
	// OPTICK_EVENT("D3D11Renderer::RenderStaticMesh");

	D3DPERF_BeginEvent(D3DCOLOR_XRGB(255, 0, 0), L"RenderStaticMesh");

	std::shared_ptr<ModelBase> modelBase = mesh->LockModel();
	
	std::shared_ptr<Model> model = dynamicCastPtr<Model, ModelBase>(modelBase);
	if (model)
	{
		for (const auto& submesh : model->GetModelSubmeshes())
		{
			// create saved render ctx as previous model.
			RenderContext savedCtx = RenderContext::GetContext();

			// create local copy of render context
			RenderContext localCtx = RenderContext::GetContext();

			// transpose matrices for D3D11
			//localCtx.model = glm::transpose(localCtx.model);

			// set our local render ctx
			RenderContext::SetContext(localCtx);

			g_renderDevice->SetVertexBuffer(submesh->GetVerticesBuffer(), sizeof(Vertex), 0);

			//g_renderDevice->SetIndexBuffer(it->getIndexBuffer());

			//it->GetMaterial()->bind();

			std::shared_ptr<Material> material = submesh->LockMaterial();
			BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

			ShaderConstantManager::GetInstance()->SetGlobalData(mesh, view, localCtx, graphicsWorld);

			// install polygon fill mode based on which mode set now

			// if we showing polys
			if (m_meshPolysWireframe && m_currentViewMode != RendererViewMode::Wireframe)
			{
				// render mesh normaly
				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// set polygon fill to lines
				g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

				// hack view
				RenderContext hackHackHack = localCtx;
				hackHackHack.proj[2][3] -= 0.0001f;
				RenderContext::SetContext(hackHackHack);

				// hack the view
				RendererViewMode savedViewMode = m_currentViewMode;
				m_currentViewMode = RendererViewMode::Wireframe;

				// bind material again
				BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

				// draw with lines
				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// reset view mode
				m_currentViewMode = savedViewMode;

				// reset mode
				SetDefaultRenderState();
			}
			else
			{
				if (GetRenderMode() == RendererViewMode::Wireframe)
					g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// reset
				if (GetRenderMode() == RendererViewMode::Wireframe)
					SetDefaultRenderState();
			}

			// return what have been
			RenderContext::SetContext(savedCtx);
		}
	}
	else
	{
		for (const auto& submesh : modelBase->GetSubmehes())
		{
			// create saved render ctx as previous model.
			RenderContext savedCtx = RenderContext::GetContext();

			// create local copy of render context
			RenderContext localCtx = RenderContext::GetContext();

			// and overwrite model matrix
			localCtx.model = savedCtx.model * submesh->GetTransform();

			// transpose matrices for D3D11
			//localCtx.model = glm::transpose(localCtx.model);

			// set our local render ctx
			RenderContext::SetContext(localCtx);

			g_renderDevice->SetVertexBuffer(submesh->GetVertexBuffer(), sizeof(Vertex), 0);

			//g_renderDevice->SetIndexBuffer(it->getIndexBuffer());

			//it->GetMaterial()->bind();

			std::shared_ptr<Material> material = submesh->LockMaterial();
			BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

			ShaderConstantManager::GetInstance()->SetGlobalData(mesh, view, localCtx, graphicsWorld);

			// install polygon fill mode based on which mode set now

			// if we showing polys
			if (m_meshPolysWireframe && m_currentViewMode != RendererViewMode::Wireframe)
			{
				// render mesh normaly
				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// set polygon fill to lines
				g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

				// hack view
				RenderContext hackHackHack = localCtx;
				hackHackHack.proj[2][3] -= 0.0001f;
				RenderContext::SetContext(hackHackHack);

				// hack the view
				RendererViewMode savedViewMode = m_currentViewMode;
				m_currentViewMode = RendererViewMode::Wireframe;

				// bind material again
				BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

				// draw with lines
				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// reset view mode
				m_currentViewMode = savedViewMode;

				// reset mode
				SetDefaultRenderState();
			}
			else
			{
				if (GetRenderMode() == RendererViewMode::Wireframe)
					g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

				g_renderDevice->Draw(PM_TriangleList, 0, submesh->GetVerticesCount());

				// reset
				if (GetRenderMode() == RendererViewMode::Wireframe)
					SetDefaultRenderState();
			}

			// return what have been
			RenderContext::SetContext(savedCtx);
		}
	}

	D3DPERF_EndEvent();
}

void D3D11Renderer::RenderAnimatedMesh(GraphicsWorld* graphicsWorld, View* view, MeshComponent* mesh)
{
	// OPTICK_EVENT("D3D11Renderer::RenderAnimatedMesh");

	if (!mesh->IsActive())
		return;

	std::shared_ptr<ModelBase> model = mesh->LockModel();
	if (!model)
		return;

	D3DPERF_BeginEvent(D3DCOLOR_XRGB(255, 0, 0), L"RenderAnimatedMesh");

	AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(model.get());

	for (const auto& submesh : animatedModel->GetAnimatedSubmehes())
	{
		// create saved render ctx as previous model.
		RenderContext savedCtx = RenderContext::GetContext();

		// create local copy of render context
		RenderContext localCtx = RenderContext::GetContext();

		// transpose matrices for D3D11
		//localCtx.model = glm::transpose(localCtx.model);

		// set our local render ctx
		RenderContext::SetContext(localCtx);

		g_renderDevice->SetVertexBuffer(submesh->m_vertexBuffer, sizeof(AnimatedVertex), 0);
		g_renderDevice->SetIndexBuffer(submesh->m_indexBuffer, false);

		//g_renderDevice->SetIndexBuffer(it->getIndexBuffer());

		//it->GetMaterial()->bind();

		std::shared_ptr<Material> material = submesh->m_material.lock();
		BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

		ShaderConstantManager::GetInstance()->SetGlobalData(mesh, view, localCtx, graphicsWorld);

		// install polygon fill mode based on which mode set now

		// if we showing polys
		if (m_meshPolysWireframe && m_currentViewMode != RendererViewMode::Wireframe)
		{
			// render mesh normaly
			g_renderDevice->DrawIndexed(PM_TriangleList, 0, submesh->m_indicesCount, 0);

			// set polygon fill to lines
			g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

			// hack view
			RenderContext hackHackHack = localCtx;
			hackHackHack.proj[2][3] -= 0.0001f;
			RenderContext::SetContext(hackHackHack);

			// hack the view
			RendererViewMode savedViewMode = m_currentViewMode;
			m_currentViewMode = RendererViewMode::Wireframe;

			// bind material again
			BindMaterialForMesh(mesh, material.get(), material->GetMaterialInstance());

			// draw with lines
			g_renderDevice->DrawIndexed(PM_TriangleList, 0, submesh->m_indicesCount, 0);

			// reset view mode
			m_currentViewMode = savedViewMode;

			// reset mode
			SetDefaultRenderState();
		}
		else
		{
			if (GetRenderMode() == RendererViewMode::Wireframe)
				g_stateManager->SetRasterizerState(g_wireframeRasterizerState);

			g_renderDevice->DrawIndexed(PM_TriangleList, 0, submesh->m_indicesCount, 0);
			//g_renderDevice->Draw(PM_TriangleList, 0, submesh->m_verticesCount);
			//glDrawArrays(GL_TRIANGLES, 0, it->GetVerticesCount());
			//glDrawElements(GL_TRIANGLES, it->getIndeciesCount(), GL_UNSIGNED_BYTE, NULL);

			// reset
			if (GetRenderMode() == RendererViewMode::Wireframe)
				SetDefaultRenderState();
		}

		// return what have been
		RenderContext::SetContext(savedCtx);
	}

	D3DPERF_EndEvent();
}

void D3D11Renderer::renderShadows(View* view)
{
}

void D3D11Renderer::TakeScreenshot()
{
	m_takeScreenshot = true;
}

#define SHOT_JPEG

void D3D11Renderer::takeScreenshotInternal()
{
	int width = GetView()->m_width;
	int height = GetView()->m_height;
	int slicePitch = width * height;

	size_t bufferSize = 4 * width * height;
	uint8_t* screenBuffer = new uint8_t[bufferSize];

	D3D11Device* device = reinterpret_cast<D3D11Device*>(g_renderDevice);

	ID3D11Texture2D* screenTexture = nullptr;
	D3D11_CHECK(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&screenTexture));

	D3D11_TEXTURE2D_DESC textureDesc;
	screenTexture->GetDesc(&textureDesc);
	textureDesc.BindFlags = 0;
	textureDesc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	textureDesc.Usage = D3D11_USAGE_STAGING;

	ID3D11Texture2D* copyOfScreenTexture = nullptr;
	D3D11_CHECK(device->GetDevice()->CreateTexture2D(&textureDesc, NULL, &copyOfScreenTexture));

	device->GetDeviceContext()->CopyResource(copyOfScreenTexture, screenTexture);

	screenTexture->Release();
	screenTexture = nullptr;

	// map resource
	D3D11_MAPPED_SUBRESOURCE mapped;
	D3D11_CHECK(device->GetDeviceContext()->Map(copyOfScreenTexture, 0, D3D11_MAP_READ, 0, &mapped));

	uint64_t imageSize = uint64_t(mapped.RowPitch) * uint64_t(textureDesc.Height);
	memcpy(screenBuffer, mapped.pData, imageSize);

	device->GetDeviceContext()->Unmap(copyOfScreenTexture, 0);

	copyOfScreenTexture->Release();
	copyOfScreenTexture = nullptr;

	Image image;
	image.createRaw((void*)screenBuffer, width, height, 4);
	image.setFlip(false);

	char buffer[256];
	for (int i = 0;; i++)
	{
#ifdef SHOT_JPEG
		snprintf(buffer, sizeof(buffer), "sshot_%i.jpg", i);
#else
		snprintf(buffer, sizeof(buffer), "sshot_%i.png", i);
#endif // SHOT_JPEG

		FileHandle fh = g_fileSystem->Open(buffer);
		if (!fh)
		{
			break;
		}
		else
		{
			g_fileSystem->Close(fh);
		}
	}

	image.Save(buffer);

	delete[] screenBuffer;

	m_takeScreenshot = false;
}

void D3D11Renderer::clearScreen()
{
	if (m_renderTargetView)
	{
		static float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		((D3D11Device*)g_renderDevice)->GetDeviceContext()->ClearRenderTargetView(m_renderTargetView, color);
		((D3D11Device*)g_renderDevice)->GetDeviceContext()->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
}

void D3D11Renderer::ClearRenderTarget(IRenderTarget* renderTarget)
{
	D3D11RenderTarget* d3dRenderTarget = (D3D11RenderTarget*)renderTarget;
	if (d3dRenderTarget)
	{
		for (auto it : d3dRenderTarget->m_renderTargetViews)
		{
			if (it)
			{
				static float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				g_d3d11Device->GetDeviceContext()->ClearRenderTargetView(it, color);
			}
		}

		if (d3dRenderTarget->m_depthStencilView)
		{
			g_d3d11Device->GetDeviceContext()->ClearDepthStencilView(d3dRenderTarget->m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		}
	}
}

void D3D11Renderer::setSwapChainRenderTarget()
{
	// Initialize target
	g_d3d11Device->GetDeviceContext()->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

}

}