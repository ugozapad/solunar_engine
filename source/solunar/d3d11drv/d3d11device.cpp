#include "d3d11drv_pch.h"
#include "d3d11drv/d3d11device.h"
#include "d3d11drv/d3d11bufferimpl.h"
#include "d3d11drv/d3d11rendertarget.h"
#include "d3d11drv/d3d11texture2d.h"
#include "d3d11drv/d3d11samplerstate.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace solunar
{

D3D11Device::D3D11Device() :
	m_device(nullptr),
	m_deviceContext(nullptr)
{
}

D3D11Device::~D3D11Device()
{
	Destroy();
}

void D3D11Device::Create()
{
	// lock feature level at shader model 4.0 (NVidia Geforce 8400+ hardware)
	D3D_FEATURE_LEVEL needFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	D3D_FEATURE_LEVEL featureLevel = (D3D_FEATURE_LEVEL)0;

	UINT deviceCreationFlags = 0;
#if 0//def _DEBUG
	deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, 0, deviceCreationFlags, &needFeatureLevel, 1, D3D11_SDK_VERSION,
		&m_device, &featureLevel, &m_deviceContext);

	if (FAILED(hr))
	{
		// #TODO: Move get win api error string to something outside device creation.
		const DWORD kStringBufferSize = 512;
		char stringBuffer[kStringBufferSize];

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
			// ONLY US-US
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			// ONLY US-US

			stringBuffer, kStringBufferSize, NULL);

#ifdef FINAL_BUILD
		Core::Error("Failed to create DirectX 11 Device. Error: %s\nMake sure you has install DirectX 11 Runtime already\nand you video card drivers\nare up to date.",
			stringBuffer);
#else
		Core::Error("Failed to create D3D11 Device. 0x%08X %s", hr, stringBuffer);
#endif
	}

#ifdef FINAL_BUILD
	// Ok, device created but feature level is low to us
	if (featureLevel < D3D_FEATURE_LEVEL_10_0)
		Core::Error("Missing DirectX 11 features level.\nPlease sure you'r video card drivers is up to date\nor support DirectX 10.");
#else
	if (featureLevel < D3D_FEATURE_LEVEL_10_0)
		Core::Error("Missing D3D_FEATURE_LEVEL_10_0 support.");
#endif

}

void D3D11Device::Destroy()
{
	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = nullptr;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = nullptr;
	}
}

IRenderTarget* D3D11Device::CreateRenderTarget(const RenderTargetCreationDesc& renderTargetDesc)
{
	return mem_new<D3D11RenderTarget>(this, renderTargetDesc);
}

IBufferBase* D3D11Device::CreateBuffer(const BufferDesc& bufferDesc, const SubresourceDesc& subresourceDesc)
{
	return mem_new<D3D11BufferImpl>(this, bufferDesc, subresourceDesc);
}

ITexture2D* D3D11Device::CreateTexture2D(const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc)
{
	return mem_new<D3D11Texture2D>(this, textureDesc, subresourceDesc);
}

ISamplerState* D3D11Device::CreateSamplerState(const SamplerDesc& samplerDesc)
{
	return mem_new<D3D11SamplerState>(this, samplerDesc);
}

void D3D11Device::SetRenderTarget(IRenderTarget* rt)
{
	if (rt)
	{
		D3D11RenderTarget* renderTarget = (D3D11RenderTarget*)rt;
		renderTarget->bind(this);
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			// #TODO: Hack with depth stencil installation
			m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
			m_deviceContext->PSGetShaderResources(i, 0, nullptr);
		}
	}

	//if (rt)
	//{
	//	D3D11RenderTarget* renderTarget = (D3D11RenderTarget*)rt;
	//	//ID3D11RenderTargetView* pD3DRenderTargetView = renderTarget->getRenderTargetView();

	//	//m_deviceContext->OMSetRenderTargets(1, &pD3DRenderTargetView, NULL);
	//}
	//else
	//{
	//	m_deviceContext->OMSetRenderTargets(0, NULL, NULL);
	//}
}

void D3D11Device::SetConstantBuffer(IBufferBase* cb)
{
	Assert2(0, "D3D11Device::SetConstantBuffer is obsolote. Please use SetConstantBufferIndex");
	SetConstantBufferIndex(0, cb);
}

void D3D11Device::SetConstantBufferIndex(int slot, IBufferBase* cb)
{
	D3D11BufferImpl* bufferImpl = (D3D11BufferImpl*)cb;
	ID3D11Buffer* pD3DBuffer = bufferImpl->getBuffer();

	m_deviceContext->VSSetConstantBuffers(slot, 1, &pD3DBuffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &pD3DBuffer);
}

void D3D11Device::SetVertexBuffer(IBufferBase* buffer, uint32_t stride, uint32_t offset)
{
	D3D11BufferImpl* bufferImpl = (D3D11BufferImpl*)buffer;
	ID3D11Buffer* pD3DBuffer = bufferImpl->getBuffer();
	m_deviceContext->IASetVertexBuffers(0, 1, &pD3DBuffer, &stride, &offset);
}

void D3D11Device::SetIndexBuffer(IBufferBase* buffer, bool use16bitsIndices)
{
	D3D11BufferImpl* bufferImpl = (D3D11BufferImpl*)buffer;
	ID3D11Buffer* pD3DBuffer = bufferImpl->getBuffer();

	m_deviceContext->IASetIndexBuffer(pD3DBuffer, use16bitsIndices ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
}

void D3D11Device::SetTexture2D(int slot, ITexture2D* texture)
{
	D3D11Texture2D* nativeTexture = (D3D11Texture2D*)texture;
	if (nativeTexture)
	{
		ID3D11ShaderResourceView* textureSRV = nativeTexture->getTextureSRV();
		if (textureSRV)
		{
			m_deviceContext->PSSetShaderResources(slot, 1, &textureSRV);
		}
	}
	else
	{
		m_deviceContext->PSSetShaderResources(slot, 0, nullptr);
	}
}

void D3D11Device::SetSamplerState(int slot, ISamplerState* sampler)
{
	D3D11SamplerState* samplerImpl = (D3D11SamplerState*)sampler;
	if (samplerImpl)
	{
		ID3D11SamplerState* pD3DSamplerState = samplerImpl->getSamplerState();
		m_deviceContext->PSSetSamplers(slot, 1, &pD3DSamplerState);
	}
	else
	{
		m_deviceContext->PSSetSamplers(slot, 0, nullptr);
	}
}

void D3D11Device::SetViewport(Viewport* viewport)
{
	Assert(viewport);

	// fill d3d11 viewport
	D3D11_VIEWPORT vp;
	memset(&vp, 0, sizeof(vp));
	vp.TopLeftX = viewport->m_x;
	vp.TopLeftY= viewport->m_y;
	vp.Width = viewport->m_width;
	vp.Height = viewport->m_height;
	vp.MinDepth = 0.1f;
	vp.MaxDepth = 1.0000f;
	m_deviceContext->RSSetViewports(1, &vp);

	// copy viewport to device
	m_viewport = *viewport;
}

Viewport D3D11Device::GetViewport()
{
	return m_viewport;
}

void D3D11Device::SetScissors(float x, float y, float w, float h)
{
	D3D11_RECT rect = { static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(w), static_cast<LONG>(h) };
	m_deviceContext->RSSetScissorRects(1, &rect);
}

void D3D11Device::Draw(PrimitiveMode primitiveMode, size_t verticesStart, size_t verticesCount)
{
	m_deviceContext->IASetPrimitiveTopology(GetD3D11PrimitiveTopology(primitiveMode));
	m_deviceContext->Draw(verticesCount, verticesStart);
}

void D3D11Device::DrawIndexed(PrimitiveMode primitiveMode, size_t indexStart, size_t indexCount, int baseVertexLocation)
{
	m_deviceContext->IASetPrimitiveTopology(GetD3D11PrimitiveTopology(primitiveMode));
	m_deviceContext->DrawIndexed(indexCount, indexStart, baseVertexLocation);
}

}