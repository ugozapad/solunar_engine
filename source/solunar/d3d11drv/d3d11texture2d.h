#ifndef GRAPHICS_D3D11_D3D11TEXTURE2D_H
#define GRAPHICS_D3D11_D3D11TEXTURE2D_H

#include "graphics/core/texture.h"
#include "d3d11drv/d3d11device.h"

namespace solunar
{

class D3D11Texture2D : public ITexture2D
{
public:
	D3D11Texture2D(D3D11Device* device, const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc);
	~D3D11Texture2D();

	void Create(D3D11Device* device, const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc);
	void Destroy();

	void updateTexture(const void* data, int rowPitch, int depthPitch) override;

	void SetDebugName(const char* debugName) override;

	SurfaceDesc getSurfaceDesc() override { return m_surfaceDesc; }
	TextureDesc getTextureDesc() { return m_textureDesc; }

	ID3D11Texture2D* GetTexture() { return m_texture; }
	ID3D11ShaderResourceView* getTextureSRV() { return m_textureSRV; }

	void GenerateMips();

private:
	TextureDesc m_textureDesc;
	SurfaceDesc m_surfaceDesc;

	ID3D11Texture2D* m_texture;
	ID3D11ShaderResourceView* m_textureSRV;
};

}

#endif