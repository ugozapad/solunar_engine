#include "graphicspch.h"
#include "graphics/core/device.h"
#include "graphics/debugrenderer.h"
#include "graphics/rendercontext.h"
#include "graphics/shaderprogram.h"
#include "graphics/ShaderProgramManager.h"
#include "graphics/view.h"

#include <glm/gtx/rotate_vector.hpp>

#define MAX_LINES_IN_VB 2048 * 8

namespace solunar {

DebugRender g_debugRender;

DebugRender::DebugRender()
{
	m_verticesBuffer = nullptr;
	m_matrixBuffer = nullptr;
	m_shaderProgram = nullptr;
	m_modelMatrix = glm::identity<glm::mat4>();
	this->m_lines.reserve(10000);
}

DebugRender::~DebugRender()
{
	m_verticesBuffer = nullptr;
	m_matrixBuffer = nullptr;
	m_shaderProgram = nullptr;
}

void DebugRender::Initialize()
{
	// Vertex buffer

	BufferDesc linesBufferDesc;
	memset(&linesBufferDesc, 0, sizeof(linesBufferDesc));
	linesBufferDesc.m_bufferType = BufferType::VertexBuffer;
	linesBufferDesc.m_bufferAccess = BufferAccess::Dynamic; // D3D11_USAGE_DEFAULT and UpdateSubresource
	linesBufferDesc.m_bufferMemorySize = sizeof(Line) * MAX_LINES_IN_VB;

	SubresourceDesc linesSubresourceDesc;
	memset(&linesSubresourceDesc, 0, sizeof(linesSubresourceDesc));

	m_verticesBuffer = g_renderDevice->CreateBuffer(linesBufferDesc, linesSubresourceDesc);

	// Matrix buffer

	BufferDesc matrixBufferDesc;
	memset(&matrixBufferDesc, 0, sizeof(matrixBufferDesc));
	matrixBufferDesc.m_bufferType = BufferType::ConstantBuffer;
	matrixBufferDesc.m_bufferAccess = BufferAccess::Static; // D3D11_USAGE_DEFAULT and UpdateSubresource
	matrixBufferDesc.m_bufferMemorySize = sizeof(glm::mat4);

	SubresourceDesc matrixSubresourceDesc;
	memset(&matrixSubresourceDesc, 0, sizeof(matrixSubresourceDesc));

	m_matrixBuffer = g_renderDevice->CreateBuffer(matrixBufferDesc, matrixSubresourceDesc);

	// create input layout
	InputLayoutDesc layoutDesc[] =
	{
		{ "POSITION", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(LineVertex, position), INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(LineVertex, color), INPUT_PER_VERTEX_DATA, 0 }
	};

	// create shader
	m_shaderProgram = g_shaderManager->CreateShaderProgram(
		"debug_draw.vsh", 
		"debug_draw.psh",
		nullptr,
		layoutDesc,
		sizeof(layoutDesc) / sizeof(layoutDesc[0]));
}

void DebugRender::Shutdown()
{
	//glDeleteVertexArrays(1, &m_vao);

	if (m_matrixBuffer)
	{
		mem_delete(m_matrixBuffer);
		m_matrixBuffer = nullptr;
	}

	if (m_verticesBuffer)
	{
		mem_delete(m_verticesBuffer);
		m_verticesBuffer = nullptr;
	}

	//g_varDebugDraw = nullptr;
}

void DebugRender::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
	Line line;
	line.from = glm::vec3(m_modelMatrix * glm::vec4(from, 1.0f));
	line.color0 = color;
	line.to = glm::vec3(m_modelMatrix * glm::vec4(to, 1.0f));
	line.color1 = color;
	m_lines.push_back(line);
}

void DebugRender::drawBoundingBox(const BoundingBox& box, const glm::vec3& color)
{
	glm::vec3 from = glm::vec3(box.m_min.x, box.m_min.y, box.m_min.z);
	glm::vec3 to = glm::vec3(box.m_max.x, box.m_max.y, box.m_max.z);

	glm::vec3 halfExtents = (to - from) * 0.5f;
	glm::vec3 center = (to + from) * 0.5f;
	int i, j;

	glm::vec3 edgecoord(1.f, 1.f, 1.f), pa, pb;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 3; j++)
		{
			pa = glm::vec3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
				edgecoord[2] * halfExtents[2]);
			pa += center;

			int othercoord = j % 3;
			edgecoord[othercoord] *= -1.f;
			pb = glm::vec3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
				edgecoord[2] * halfExtents[2]);
			pb += center;

			DrawLine(pa, pb, color);
		}

		edgecoord = glm::vec3(-1.f, -1.f, -1.f);
		if (i < 3)
			edgecoord[i] *= -1.f;
	}
}

#define SIMD_PI float(3.1415926535897932384626433832795029)
#define SIMD_2_PI (float(2.0) * SIMD_PI)
#define SIMD_HALF_PI (SIMD_PI * float(0.5))
#define SIMD_RADS_PER_DEG (SIMD_2_PI / float(360.0))
#define SIMD_DEGS_PER_RAD (float(360.0) / SIMD_2_PI)
#define SIMDSQRT12 float(0.7071067811865475244008443621048490)

void DebugRender::drawArc(const glm::vec3& center, const glm::vec3& normal, const glm::vec3& axis, float radiusA, float radiusB, float minAngle, float maxAngle, const glm::vec3& color, bool drawSect, float stepDegrees)
{
	const glm::vec3& vx = axis;
	glm::vec3 vy = glm::cross(normal, axis);  //normal.cross(axis);
	float step = stepDegrees * SIMD_RADS_PER_DEG;
	int nSteps = (int)fabs((maxAngle - minAngle) / step);
	if (!nSteps) nSteps = 1;
	glm::vec3 prev = center + radiusA * vx * cos(minAngle) + radiusB * vy * sin(minAngle);
	if (drawSect)
	{
		DrawLine(center, prev, color);
	}
	for (int i = 1; i <= nSteps; i++)
	{
		float angle = minAngle + (maxAngle - minAngle) * float(i) / float(nSteps);
		glm::vec3 next = center + radiusA * vx * cos(angle) + radiusB * vy * sin(angle);
		DrawLine(prev, next, color);
		prev = next;
	}
	if (drawSect)
	{
		DrawLine(center, prev, color);
	}
}

void DebugRender::drawCone(float radius, float height, int upAxis, const glm::vec3& transform, const glm::vec3& color)
{
#if 0
	int stepDegrees = 30;
	btVector3 start = transform.getOrigin();

	btVector3 offsetHeight(0, 0, 0);
	btScalar halfHeight = height * btScalar(0.5);
	offsetHeight[upAxis] = halfHeight;
	btVector3 offsetRadius(0, 0, 0);
	offsetRadius[(upAxis + 1) % 3] = radius;
	btVector3 offset2Radius(0, 0, 0);
	offset2Radius[(upAxis + 2) % 3] = radius;

	btVector3 capEnd(0.f, 0.f, 0.f);
	capEnd[upAxis] = -halfHeight;

	for (int i = 0; i < 360; i += stepDegrees)
	{
		capEnd[(upAxis + 1) % 3] = btSin(btScalar(i) * SIMD_RADS_PER_DEG) * radius;
		capEnd[(upAxis + 2) % 3] = btCos(btScalar(i) * SIMD_RADS_PER_DEG) * radius;
		DrawLine(start + transform.getBasis() * (offsetHeight), start + transform.getBasis() * capEnd, color);
	}

	DrawLine(start + transform.getBasis() * (offsetHeight), start + transform.getBasis() * (-offsetHeight + offsetRadius), color);
	DrawLine(start + transform.getBasis() * (offsetHeight), start + transform.getBasis() * (-offsetHeight - offsetRadius), color);
	DrawLine(start + transform.getBasis() * (offsetHeight), start + transform.getBasis() * (-offsetHeight + offset2Radius), color);
	DrawLine(start + transform.getBasis() * (offsetHeight), start + transform.getBasis() * (-offsetHeight - offset2Radius), color);

	// Drawing the base of the cone
	btVector3 yaxis(0, 0, 0);
	yaxis[upAxis] = btScalar(1.0);
	btVector3 xaxis(0, 0, 0);
	xaxis[(upAxis + 1) % 3] = btScalar(1.0);
	drawArc(start - transform.getBasis() * (offsetHeight), transform.getBasis() * yaxis, transform.getBasis() * xaxis, radius, radius, 0, SIMD_2_PI, color, false, 10.0);
#endif
}

void DebugRender::DrawPoly(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color)
{
	// a-b
	DrawLine(a, b, color);

	// b - c
	DrawLine(b, c, color);

	// c - a
	DrawLine(c, a, color);
}

void DebugRender::RenderFrame(View* view)
{
	beginDraw();

	// draw lines
	drawLinesInternal(view);

	// and clear them
	m_lines.clear();

	endDraw();
}

void DebugRender::PushModelMatrix(const glm::mat4& model)
{
	m_modelMatrix = model;
}

void DebugRender::PopModelMatrix()
{
	m_modelMatrix = glm::identity<glm::mat4>();
}

void DebugRender::drawAxis(const glm::vec3& vec)
{
	const float length = 0.1f;
	DrawLine(vec, glm::vec3(vec.x + length, vec.y, vec.z), glm::vec3(1.0f, 0.0, 0.0f));
	DrawLine(vec, glm::vec3(vec.x, vec.y + length, vec.z), glm::vec3(0.0f, 0.0f, 1.0f));
	DrawLine(vec, glm::vec3(vec.x, vec.y, vec.z + length), glm::vec3(0.0f, 1.0f, 0.0f));
}

void DebugRender::beginDraw()
{
	//glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (int*)&m_savedVao);

	//glBindVertexArray(m_vao);
}

void DebugRender::endDraw()
{
	//glBindVertexArray(m_savedVao);
}

void updateMatrixBuffer(IBufferBase* buffer, const glm::mat4& viewProjection)
{
	glm::mat4* matrix = (glm::mat4*)buffer->Map(BufferMapping::WriteOnly);
	memcpy(matrix, &viewProjection[0], sizeof(viewProjection));
	buffer->Unmap();
}

void DebugRender::drawLinesInternal(View* view)
{
	if (m_lines.size() < 1)
		return;

	g_renderDevice->SetVertexBuffer(m_verticesBuffer, sizeof(LineVertex), 0);
	
	//m_verticesBuffer->UpdateSubresource(m_lines.data(), m_lines.size() * sizeof(Line));

	LineVertex* vertices = (LineVertex*)m_verticesBuffer->Map(BufferMapping::WriteOnly);
	memcpy(vertices, m_lines.data(), m_lines.size() * sizeof(Line));
	m_verticesBuffer->Unmap();

	g_shaderManager->SetShaderProgram(m_shaderProgram);

	glm::mat4 mVP = glm::mat4(1.0f);
	mVP = view->m_projection * view->m_view;

	//updateMatrixBuffer(m_matrixBuffer, mVP);
	m_matrixBuffer->UpdateSubresource(&mVP[0], sizeof(mVP));
	g_renderDevice->SetConstantBufferIndex(0, m_matrixBuffer);

	g_renderDevice->Draw(PM_LineList, 0, m_lines.size() * 2);
}

}