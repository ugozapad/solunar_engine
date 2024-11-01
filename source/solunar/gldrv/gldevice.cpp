#include "gldrv_pch.h"

#include "gldrv/gldevice.h"
#include "gldrv/glrendertarget.h"
#include "gldrv/gltexture2d.h"
#include "gldrv/glbufferimpl.h"
#include "gldrv/glsamplerstate.h"

namespace solunar {

GLDevice::GLDevice()
{
	Create();
}

GLDevice::~GLDevice()
{
	Destroy();
}

void GLDevice::Create()
{

}

void GLDevice::Destroy()
{

}

IRenderTarget* GLDevice::CreateRenderTarget(const RenderTargetCreationDesc& renderTargetDesc)
{
	return (IRenderTarget*)mem_new<GLRenderTarget>(renderTargetDesc);
}

IBufferBase* GLDevice::CreateBuffer(const BufferDesc& bufferDesc, const SubresourceDesc& subresourceDesc)
{
	return (IBufferBase*)mem_new<GLBufferImpl>(bufferDesc, subresourceDesc);
}

ITexture2D* GLDevice::CreateTexture2D(const TextureDesc& textureDesc, const SubresourceDesc& subresourceDesc)
{
	return (ITexture2D*)mem_new<GLTexture2D>(textureDesc, subresourceDesc);
}

ISamplerState* GLDevice::CreateSamplerState(const SamplerDesc& samplerDesc)
{
	return (ISamplerState*)mem_new<GLSamplerState>(samplerDesc);
}

void GLDevice::SetRenderTarget(IRenderTarget* rt)
{
	GLRenderTarget* glrt = (GLRenderTarget*)rt;
	glBindFramebuffer(GL_FRAMEBUFFER, glrt ? glrt->getHandle() : 0);
}

void GLDevice::SetVertexBuffer(IBufferBase* buffer, uint32_t stride, uint32_t offset)
{
	//ASSERT(buffer->getBufferDesc().m_bufferType != BufferType::VertexBuffer);

	GLBufferImpl* bufferImpl = reinterpret_cast<GLBufferImpl*>(buffer);
	glBindBuffer(GL_ARRAY_BUFFER, bufferImpl ? bufferImpl->getHandle() : 0);
}

void GLDevice::SetIndexBuffer(IBufferBase* buffer, bool use16bitsIndices)
{
	//ASSERT(buffer->getBufferDesc().m_bufferType != BufferType::IndexBuffer);

	GLBufferImpl* bufferImpl = reinterpret_cast<GLBufferImpl*>(buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferImpl ? bufferImpl->getHandle() : 0);
}

void GLDevice::SetTexture2D(int slot, ITexture2D* texture)
{
	GLTexture2D* nativeTexture = reinterpret_cast<GLTexture2D*>(texture);

	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, nativeTexture ? nativeTexture->getHandle() : 0);

	//glBindMultiTextureEXT(GL_TEXTURE0 + slot, GL_TEXTURE_2D, nativeTexture ? nativeTexture->getHandle() : 0);
}

void GLDevice::SetSamplerState(int slot, ISamplerState* sampler)
{
	GLSamplerState* samplerImpl = reinterpret_cast<GLSamplerState*>(sampler);
	glBindSampler(slot, samplerImpl ? samplerImpl->getHandle() : 0);
}

void GLDevice::SetConstantBuffer(IBufferBase* cb)
{
	Assert2(0, "GLDevice::SetConstantBuffer is obsolote. Please use SetConstantBufferIndex");

	//ASSERT(cb->getBufferDesc().m_bufferType != BufferType::ConstantBuffer);

	GLBufferImpl* bufferImpl = reinterpret_cast<GLBufferImpl*>(cb);
	glBindBuffer(GL_UNIFORM_BUFFER, bufferImpl ? bufferImpl->getHandle() : 0);
}

void GLDevice::SetConstantBufferIndex(int slot, IBufferBase* cb)
{
	//ASSERT(cb->getBufferDesc().m_bufferType != BufferType::ConstantBuffer);

	GLBufferImpl* bufferImpl = reinterpret_cast<GLBufferImpl*>(cb);
	glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferImpl ? bufferImpl->getHandle() : 0);
}

//void GLDevice::setVertexFormat(VertexFormat* vertex_format)
//{
//	Assert(vertex_format);
//	if (vertex_format->count() == 0) {
//		Core::error("GLDevice::setVertexFormat: failed to set empty vertex format");
//	}
//
//	size_t appliedOffset = 0;
//	for (VertexAttribute* it = vertex_format->begin(); it != vertex_format->end(); ++it) {
//	
//		if (appliedOffset > 0)
//			glVertexAttribPointer(it->m_offset, it->m_size, GL_FLOAT, GL_FALSE, vertex_format->size() * sizeof(float), (void*)(appliedOffset * sizeof(float)));
//		else
//			glVertexAttribPointer(it->m_offset, it->m_size, GL_FLOAT, GL_FALSE, vertex_format->size() * sizeof(float), (void*)0);
//	
//		//glVertexAttribPointer(it->m_offset, it->m_size, GL_FLOAT, GL_FALSE, format->size() * sizeof(float), (void*)appliedOffset);
//		glEnableVertexAttribArray(it->m_offset);
//
//		appliedOffset += it->m_size;
//	}
//}

void GLDevice::SetViewport(Viewport* viewport)
{
	Assert(viewport);
	m_viewport = *viewport;
	glViewport(m_viewport.m_x, m_viewport.m_y, m_viewport.m_width, m_viewport.m_height);
}

Viewport GLDevice::GetViewport()
{
	return m_viewport;
}

void GLDevice::SetScissors(float x, float y, float w, float h)
{
	glScissor((int)x, (int)y, (int)w, (int)h);
}

static GLenum s_glPrimiviteMode[PM_UNUSED_COUNT] =
{
	GL_LINES,			// PM_LineList
	GL_LINE_STRIP,		// PM_LineStrip
	GL_TRIANGLES,		// PM_TriangleList
	GL_TRIANGLE_STRIP,	// PM_TriangleStrip
};

void GLDevice::Draw(PrimitiveMode primitiveMode, size_t verticesStart, size_t verticesCount)
{
	glDrawArrays(s_glPrimiviteMode[primitiveMode], verticesStart, verticesCount);
}

void GLDevice::DrawIndexed(PrimitiveMode primitiveMode, size_t indexStart, size_t indexCount, int baseVertexLocation)
{
	glDrawElements(s_glPrimiviteMode[primitiveMode], indexCount, GL_UNSIGNED_INT, NULL);
}

}