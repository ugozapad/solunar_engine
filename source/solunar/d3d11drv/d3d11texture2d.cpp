#include "d3d11drv_pch.h"
#include "d3d11drv/d3d11texture2d.h"

namespace solunar
{

D3D11Texture2D::D3D11Texture2D(D3D11Device* device, const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc) :
	m_textureDesc(textureDesc),
	m_texture(nullptr),
	m_textureSRV(nullptr)
{
	Create(device, textureDesc, subresourceDesc);
}

D3D11Texture2D::~D3D11Texture2D()
{
	Destroy();
}

void D3D11Texture2D::Create(D3D11Device* device, const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc)
{
	Assert2(device, "Failed to create texture without initialized device.");

	D3D11_TEXTURE2D_DESC d3dTextureDesc;
	memset(&d3dTextureDesc, 0, sizeof(d3dTextureDesc));
	d3dTextureDesc.Width = textureDesc.m_width;
	d3dTextureDesc.Height = textureDesc.m_height;
	d3dTextureDesc.MipLevels = (textureDesc.m_mipmapLevel < 1 ? 1 : textureDesc.m_mipmapLevel);
	d3dTextureDesc.ArraySize = 1;
	d3dTextureDesc.Format = GetDXGIFormat(textureDesc.m_format);
	d3dTextureDesc.SampleDesc.Count = 1;
	d3dTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	d3dTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	// #TODO: FIX !!!
	if (textureDesc.m_format == ImageFormat::DEPTH32F && textureDesc.m_renderTargetUsage)
		d3dTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	
	// #TODO: FIX !!!
	bool isDepth =	textureDesc.m_format == ImageFormat::DEPTH32F ||
					textureDesc.m_format == ImageFormat::DEPTH32 ||
					textureDesc.m_format == ImageFormat::DEPTH24;
	// #TODO: FIX !!!
	if (isDepth)
		d3dTextureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	else if (textureDesc.m_renderTargetUsage)
		d3dTextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

#if 0
	// #TODO: HACK
	if (textureDesc.m_format == ImageFormat::DEPTH32F ||
		textureDesc.m_format == ImageFormat::DEPTH32 ||
		textureDesc.m_format == ImageFormat::DEPTH24)
	{
		d3dTextureDesc.BindFlags = 0;
	}
	else
	{
		d3dTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	}
#endif


	d3dTextureDesc.CPUAccessFlags = 0;
	d3dTextureDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceData;
	memset(&subresourceData, 0, sizeof(subresourceData));
	subresourceData.pSysMem = subresourceDesc.m_memory;
	subresourceData.SysMemPitch = subresourceDesc.m_memoryPitch;
	subresourceData.SysMemSlicePitch = subresourceDesc.m_memorySlicePitch;

	D3D11_CHECK(device->GetDevice()->CreateTexture2D(&d3dTextureDesc, subresourceDesc.m_memory ? &subresourceData : NULL, &m_texture));

	if ((d3dTextureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE ) != 0)  
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		memset(&shaderResourceViewDesc, 0, sizeof(shaderResourceViewDesc));
		DXGI_FORMAT typelessFormat = ConvertToTypelessFmt(d3dTextureDesc.Format);
		DXGI_FORMAT srvFormat = ConvertToShaderResourceFmt(typelessFormat);
		shaderResourceViewDesc.Format = typelessFormat == srvFormat ? d3dTextureDesc.Format : srvFormat;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MipLevels = -1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		D3D11_CHECK(device->GetDevice()->CreateShaderResourceView(m_texture, &shaderResourceViewDesc, &m_textureSRV));
	}
}

void D3D11Texture2D::Destroy()
{
	if (m_textureSRV)
	{
		m_textureSRV->Release();
		m_textureSRV = nullptr;
	}

	if (m_texture)
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

void D3D11Texture2D::updateTexture(const void* data, int rowPitch, int depthPitch)
{
	g_d3d11Device->GetDeviceContext()->UpdateSubresource(
		m_texture,
		0,
		NULL,
		data,
		rowPitch,
		depthPitch);
}

void D3D11Texture2D::SetDebugName(const char* debugName)
{
	D3D11_CHECK(m_texture->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName));
	D3D11_CHECK(m_textureSRV->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName));
}

void D3D11Texture2D::GenerateMips()
{
	g_d3d11Device->GetDeviceContext()->GenerateMips(m_textureSRV);
}

}