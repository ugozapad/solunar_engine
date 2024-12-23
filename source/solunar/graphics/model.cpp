#include "graphicspch.h"
#include "model.h"

#include "core/file/filesystem.h"
#include "core/file/datastream.h"
#include "core/file/contentmanager.h"

#include "graphics/shaderprogram.h"
#include "graphics/material.h"

#include "graphics/renderer.h"
#include "graphics/rendercontext.h"

#include "graphics/core/device.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include "core/file/filesystem.h"

//#define DISABLE_ASSIMP

#ifndef DISABLE_ASSIMP
#ifdef _MSC_VER
#	ifdef _DEBUG
#		pragma comment(lib, "assimp-vc141-mtd.lib")
#	else
#		pragma comment(lib, "assimp-vc141-mt.lib")
#	endif
#endif // _MSC_VER
#endif // !DISABLE_ASSIMP

namespace solunar
{

	inline static glm::mat4 Assimp2Glm(const aiMatrix4x4& from)
	{
		return glm::mat4(
			(double)from.a1, (double)from.b1, (double)from.c1, (double)from.d1,
			(double)from.a2, (double)from.b2, (double)from.c2, (double)from.d2,
			(double)from.a3, (double)from.b3, (double)from.c3, (double)from.d3,
			(double)from.a4, (double)from.b4, (double)from.c4, (double)from.d4
		);
	}

	std::string getDirectoryPath(const std::string& filepath)
	{
		std::string directory = "";
		size_t last_slash_idx = filepath.rfind('/');
		if (std::string::npos == last_slash_idx)
			size_t last_slash_idx = filepath.rfind('\\');

		if (std::string::npos != last_slash_idx)
		{
			directory = filepath.substr(0, last_slash_idx);
		}
		return directory;
	}

	void osConvertPath(std::string& path)
	{
#ifdef WIN32
		for (int i = 0; i < path.size(); i++)
			if (path[i] == '/')
				path[i] = '\\';
#endif
	}

	void osConvertStandartPath(std::string& path)
	{
#ifdef WIN32
		for (int i = 0; i < path.size(); i++)
			if (path[i] == '\\')
				path[i] = '/';
#endif
	}

#ifndef DISABLE_ASSIMP
	SubMesh* ProccessSubMesh(aiMesh* mesh, aiNode* node, const aiScene* scene)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indecies;

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.m_position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex.m_normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

			if (mesh->mTextureCoords[0])
			{
				vertex.m_texcoord0 = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

				if (mesh->mTangents)
					vertex.m_tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
				if (mesh->mBitangents)
					vertex.m_bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			}
			else
			{
				vertex.m_texcoord0 = glm::vec2(0.0f, 0.0f);
			}

			//if (mesh->mTextureCoords[1])
			//	vertex.m_texcoord1 = glm::vec2(mesh->mTextureCoords[1][i].x, mesh->mTextureCoords[1][i].y);
			//else
			//	vertex.m_texcoord1 = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			for (uint32_t j = 0; j < face.mNumIndices; j++)
				indecies.push_back(face.mIndices[j]);
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "data/materials/%s.xml", material->GetName().C_Str());

		std::string bufferString = buffer;
		osConvertStandartPath(bufferString);

		if (!g_fileSystem->Exist(bufferString.c_str()))
		{
			std::string directoryPath = getDirectoryPath(bufferString);
			osConvertPath(directoryPath);

	/*		if (!isDirectoryExist(directoryPath.c_str()))
				createDirectory(directoryPath.c_str());*/

			aiString diffusePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);

			if (diffusePath.length == 0)
			{
				material->GetTexture(aiTextureType_DIFFUSE, 1, &diffusePath);
				if (diffusePath.length == 0)
				{
					material->GetTexture(aiTextureType_BASE_COLOR, 0, &diffusePath);
				}
			}


			aiString normalPath;
			material->GetTexture(aiTextureType_NORMALS, 0, &normalPath);

			Material::createMaterialFromImport(material->GetName().C_Str(), diffusePath.C_Str(), normalPath.C_Str());
		}

		aiMatrix4x4 nodePosition = node->mTransformation;
		glm::mat4 transform = glm::mat4(1.0f);
		transform = Assimp2Glm(nodePosition);

		return mem_new<SubMesh>(vertices, indecies, transform, material->GetName().C_Str());
	}

	void ProccessNode(std::vector<SubMesh*>& submeshes, aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			SubMesh* submesh = ProccessSubMesh(mesh, node, scene);
			submeshes.push_back(submesh);
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProccessNode(submeshes, node->mChildren[i], scene);
		}
	}
#endif // !DISABLE_ASSIMP

	IMPLEMENT_OBJECT(ModelBase, GraphicsObject);

	ModelBase::ModelBase()
	{
	}

	ModelBase::~ModelBase()
	{
		Destroy();
	}

	void ModelBase::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<ModelBase>();
	}

#ifndef DISABLE_ASSIMP
	unsigned int kAssimpFlags =                      // some optimizations and safety checks
		aiProcess_OptimizeMeshes |                                   // minimize number of meshes
		aiProcess_PreTransformVertices |                             // apply node matrices
		aiProcess_Triangulate |
		aiProcess_TransformUVCoords /*|*/ // apply UV transformations
		/*aiProcess_FlipUVs*/
		;
#endif // !DISABLE_ASSIMP

	void ModelBase::Load(const std::shared_ptr<DataStream>& dataStream)
	{
#ifndef DISABLE_ASSIMP
		dataStream->Seek(Seek_End, 0);
		size_t filesize = dataStream->Tell();
		dataStream->Seek(Seek_Begin, 0);

		uint8_t* fileData = mem_array<uint8_t>(filesize);
		dataStream->Read(fileData, filesize);

		Assimp::Importer importer;
		//const aiScene* scene = importer.ReadFile(m_filename.c_str(), aiProcess_Triangulate /*| aiProcess_TransformUVCoords | aiProcess_FlipUVs*/);
		const aiScene* scene = importer.ReadFileFromMemory(fileData, filesize, kAssimpFlags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			Core::Error("Failed to load model. %s", importer.GetErrorString());
		}

		Assert(scene);
		ProccessNode(m_subMeshes, scene->mRootNode, scene);

		mem_free_array(fileData);

		CreateHw();
#else
		Core::Error("ModelBase::Load: Assimp import is disabled");
#endif // !DISABLE_ASSIMP
	}

	void ModelBase::Destroy()
	{
		for (int i = 0; i < m_subMeshes.size(); i++)
		{
			m_subMeshes[i]->Destroy();
			mem_delete(m_subMeshes[i]);
		}

		m_subMeshes.clear();
	}

	void ModelBase::renderObjects()
	{
		for (int i = 0; i < m_subMeshes.size(); i++)
			m_subMeshes[i]->Render();
	}

	void ModelBase::SaveBinary(const std::string& filename)
	{
		//return;

		std::string oldFilename = filename;
		size_t extensionPos = oldFilename.find_last_of('.');
		if (extensionPos == std::string::npos)
			Core::Error("ModelBase::saveBinary: invalid filename.");

		//std::string modelfilename = oldFilename.substr(0, extensionPos);

#if 0
		std::string modelfilename = "data/";
		modelfilename += oldFilename.substr(0, extensionPos);
		modelfilename += ".model";
#else
		std::string modelfilename = oldFilename.substr(0, extensionPos);
		if (modelfilename.find("data/") != std::string::npos)
		{
			modelfilename.clear();
			modelfilename = "data/";
			modelfilename += oldFilename.substr(0, extensionPos);
			modelfilename += ".model";
		}
		else
		{
			modelfilename += ".model";
		}
#endif

#define DELETE_OLD_MODEL

#ifndef DELETE_OLD_MODEL
		if (g_fileSystem->Exist(modelfilename.c_str()))
			return;
#else
		if (g_fileSystem->Exist(modelfilename.c_str()))
		{
			DeleteFileA(modelfilename.c_str());
		}
#endif // !DELETE_OLD_MODEL

		FileHandle file = g_fileSystem->Create(modelfilename.c_str());

		ModelFileHeader header = { 0 };
		header.magic = MODELFILE_MAGIC;
		header.version = kModelFileVersion;
		header.submeshCount = (uint32_t)m_subMeshes.size();
		g_fileSystem->Write(file, &header, sizeof(header));

		for (SubMesh* pSubMesh : m_subMeshes)
		{
			ModelFileSubmeshData submeshData;
			memset(&submeshData, 0, sizeof(submeshData));
			strcpy(submeshData.materialInfo, pSubMesh->m_materialName.c_str());
			submeshData.verticesCount = pSubMesh->GetVerticesCount();
			submeshData.indicesCount = pSubMesh->getIndeciesCount();
			g_fileSystem->Write(file, &submeshData, sizeof(submeshData));

			Vertex* pBufferVertices = (Vertex*)pSubMesh->m_vertices.data();

			// Avoid transform in vertex buffer
			// so we copy vertices to temp buffer and write them to file.
			Vertex* pVertices = (Vertex*)malloc(pSubMesh->GetVerticesCount() * sizeof(Vertex));
			memcpy(pVertices, (const void*)pBufferVertices, pSubMesh->GetVerticesCount() * sizeof(Vertex));

			pBufferVertices = nullptr;

			// Transform vertices to OpenGL space.
			for (uint32_t i = 0; i < pSubMesh->GetVerticesCount(); i++)
			{
				pVertices[i].m_position = pSubMesh->GetTransform() * glm::vec4(pVertices[i].m_position, 0.0f);
			}

			g_fileSystem->Write(file, pVertices, pSubMesh->GetVerticesCount() * sizeof(Vertex));

			// free temp buffer
			free(pBufferVertices);

			unsigned int* pIndices = (unsigned int*)pSubMesh->m_indecies.data();
			g_fileSystem->Write(file, pIndices, pSubMesh->getIndeciesCount() * sizeof(uint32_t));

			// Unmap buffer
			pIndices = nullptr;
		}

		g_fileSystem->Close(file);
	}

	void ModelBase::CreateHw()
	{
		for (int i = 0; i < m_subMeshes.size(); i++)
		{
			SubMesh* submesh = m_subMeshes[i];

			// Create vertex buffer

			BufferDesc vertexBufferDesc;
			memset(&vertexBufferDesc, 0, sizeof(vertexBufferDesc));
			vertexBufferDesc.m_bufferType = BufferType::VertexBuffer;
			vertexBufferDesc.m_bufferAccess = BufferAccess::Static;
			vertexBufferDesc.m_bufferMemorySize = (uint32_t)submesh->m_vertices.size() * sizeof(Vertex);

			SubresourceDesc vertexBufferSubresourceDesc;
			memset(&vertexBufferSubresourceDesc, 0, sizeof(vertexBufferSubresourceDesc));
			vertexBufferSubresourceDesc.m_memory = submesh->m_vertices.data();

			submesh->m_vertexBuffer = g_renderDevice->CreateBuffer(vertexBufferDesc, vertexBufferSubresourceDesc);

			// Create index buffer

			BufferDesc indexBufferDesc;
			memset(&indexBufferDesc, 0, sizeof(indexBufferDesc));
			indexBufferDesc.m_bufferType = BufferType::IndexBuffer;
			indexBufferDesc.m_bufferAccess = BufferAccess::Static;
			indexBufferDesc.m_bufferMemorySize = (uint32_t)submesh->m_indecies.size() * sizeof(uint32_t);

			SubresourceDesc indexBufferSubresourceDesc;
			memset(&indexBufferSubresourceDesc, 0, sizeof(indexBufferSubresourceDesc));
			indexBufferSubresourceDesc.m_memory = submesh->m_indecies.data();

			submesh->m_indexBuffer = g_renderDevice->CreateBuffer(indexBufferDesc, indexBufferSubresourceDesc);

			// Load material
			submesh->m_material = g_contentManager->LoadObject<Material>(submesh->m_materialName);
			if (submesh->m_material.expired())
			{
				Core::Msg("ERROR: ModelBase: Missing material \"%s\"", submesh->m_materialName.c_str());
				submesh->m_material = GetDefaultMaterial();
			}
		}
	}

	void ModelBase::ReleaseHw()
	{
		Destroy();
	}

	SubMesh::SubMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indecies, const glm::mat4& position, const char* materialname) :
		m_vertexBuffer(0),
		m_indexBuffer(0)
	{
		load(vertices, indecies, position, materialname);
	}

	SubMesh::SubMesh() :
		m_vertexBuffer(0),
		m_indexBuffer(0)
	{

	}

	void SubMesh::load(std::vector<Vertex>& vertices, std::vector<uint32_t>& indecies, const glm::mat4& position, const char* materialname)
	{
		//m_materialName = materialname;
		m_materialName += "materials/";
		m_materialName += materialname;
		m_materialName += ".xml";

		m_transform = position;

		m_vertices = vertices;
		m_indecies = indecies;
		m_verticesCount = (uint32_t)vertices.size();
		m_indeciesCount = (uint32_t)indecies.size();
	}

	SubMesh::~SubMesh()
	{
		Destroy();
	}

	void SubMesh::Destroy()
	{
		m_material.reset();

		if (m_indexBuffer)
		{
			mem_delete(m_indexBuffer);
			m_indexBuffer = nullptr;
		}

		if (m_vertexBuffer)
		{
			mem_delete(m_vertexBuffer);
			m_vertexBuffer = nullptr;
		}
	}

	void SubMesh::Render()
	{
		//create saved render ctx as previous model.
		//RenderContext savedCtx = RenderContext::GetContext();

		//// create local copy of render context
		//RenderContext localCtx = RenderContext::GetContext();

		//// and overwrite model matrix
		//localCtx.model = savedCtx.model * m_transform;

		//// set our local render ctx
		//RenderContext::SetContext(localCtx);

		//GraphicsDevice::GetInstance()->SetVertexBuffer(m_vertexBuffer);
		//g_renderDevice->setVertexFormat(&s_vfVertex);
		//
		//m_material->bind();

		//GraphicsDevice::GetInstance()->SetIndexBuffer(m_indexBuffer);

		////GraphicsDevice::GetInstance()->drawArray(PrimitiveMode::Triangles, 0, m_verticesCount);
		//GraphicsDevice::GetInstance()->drawElements(PrimitiveMode::Triangles, m_indeciesCount);

		//// return what have been
		//RenderContext::SetContext(savedCtx);

		//// reset material bindings
		//m_material->resetAllStates();
	}

	std::shared_ptr<Material> SubMesh::LockMaterial()
	{
		return m_material.lock();
	}

	////////////////////////////////////////////////////////////////////
	// Model Submesh

	IMPLEMENT_OBJECT(ModelSubmesh, GraphicsObject);

	void ModelSubmesh::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<ModelSubmesh>();
	}

	ModelSubmesh::ModelSubmesh() :
		m_verticesBuffer(nullptr),
		m_indicesBuffer(nullptr),
		m_verticesCount(0),
		m_indicesCount(0)
	{
	}

	ModelSubmesh::~ModelSubmesh()
	{
		ReleaseHw();
	}

	void ModelSubmesh::Load(DataStreamPtr stream)
	{
		// read submesh data
		ModelFileSubmeshData submeshData = { 0 };
		stream->Read(&submeshData);

		m_materialName = submeshData.materialInfo;
		m_material = dynamicCastPtr<Material>(g_contentManager->Load(m_materialName, Material::GetStaticTypeInfo()));

		Assert2(submeshData.verticesCount != 0, "Mesh has zero vertices");
		Assert2(submeshData.indicesCount != 0, "Mesh has zero indices");

		m_verticesCount = submeshData.verticesCount;
		m_vertices.resize(m_verticesCount);
		stream->Read(m_vertices.data(), m_verticesCount * sizeof(Vertex));

		m_indicesCount = submeshData.indicesCount;
		m_indices.resize(m_indicesCount);
		stream->Read(m_indices.data(), m_indicesCount * sizeof(unsigned int));

		CreateHw();
	}

	void ModelSubmesh::Save(DataStreamPtr stream)
	{
		ModelFileSubmeshData submeshData;
		memset(&submeshData, 0, sizeof(submeshData));
		strcpy(submeshData.materialInfo, m_materialName.c_str());
		submeshData.verticesCount = GetVerticesCount();
		submeshData.indicesCount = GetIndicesCount();
		stream->Write(&submeshData);

		Vertex* pVertices = (Vertex*)m_vertices.data();
		stream->Write(pVertices, GetVerticesCount() * sizeof(Vertex));

		unsigned int* pIndices = (unsigned int*)m_indices.data();
		stream->Write(pIndices, GetIndicesCount() * sizeof(uint32_t));
	}

	void ModelSubmesh::CreateHw()
	{
		BufferDesc verticesBufferDesc;
		memset(&verticesBufferDesc, 0, sizeof(verticesBufferDesc));
		verticesBufferDesc.m_bufferType = BufferType::VertexBuffer;
		verticesBufferDesc.m_bufferAccess = BufferAccess::Static;
		verticesBufferDesc.m_bufferMemorySize = (uint32_t)m_vertices.size() * sizeof(Vertex);

		SubresourceDesc verticesSubresourceDesc;
		memset(&verticesSubresourceDesc, 0, sizeof(verticesSubresourceDesc));
		verticesSubresourceDesc.m_memory = m_vertices.data();

		m_verticesBuffer = g_renderDevice->CreateBuffer(verticesBufferDesc, verticesSubresourceDesc);

		BufferDesc indicesBufferDesc;
		memset(&indicesBufferDesc, 0, sizeof(indicesBufferDesc));
		indicesBufferDesc.m_bufferType = BufferType::IndexBuffer;
		indicesBufferDesc.m_bufferAccess = BufferAccess::Static;
		indicesBufferDesc.m_bufferMemorySize = (uint32_t)m_indices.size() * sizeof(unsigned int);

		SubresourceDesc indicesSubresourceDesc;
		memset(&indicesSubresourceDesc, 0, sizeof(indicesSubresourceDesc));
		indicesSubresourceDesc.m_memory = m_indices.data();

		m_indicesBuffer = g_renderDevice->CreateBuffer(indicesBufferDesc, indicesSubresourceDesc);

#ifdef FINAL_BUILD
		// clear temp data
		m_indices.clear();
		m_vertices.clear();
#endif // !FINAL_BUILD
	}

	void ModelSubmesh::ReleaseHw()
	{
		mem_delete(m_indicesBuffer);
		m_indicesBuffer = nullptr;

		mem_delete(m_verticesBuffer);
		m_verticesBuffer = nullptr;
	}

	std::shared_ptr<Material> ModelSubmesh::LockMaterial()
	{
		return m_material.lock();
	}

	IMPLEMENT_OBJECT(Model, ModelBase);

	void Model::RegisterObject()
	{
		TypeManager::GetInstance()->RegisterObject<Model>();
	}

	Model::Model()
	{
	}

	Model::~Model()
	{
	}

	void Model::Load(const std::shared_ptr<DataStream>& stream)
	{
		// read header
		ModelFileHeader header;
		stream->Read(&header);

		if (header.magic != MODELFILE_MAGIC)
			Core::Error("Model file has unknowed magic!");

		if (header.version > kModelFileVersion)
			Core::Msg("Model has older version format");
		else if (header.version < kModelFileVersion)
			Core::Msg("Model has newer version than engine support. (model %i, engine %i)", header.version, kModelFileVersion);

		if (header.submeshCount == 0)
			Core::Msg("Model has zero sub meshes, cannot load");

		if (header.version > kModelFileVersion ||
			header.version < kModelFileVersion ||
			header.submeshCount == 0)
			return;

		for (uint32_t i = 0; i < header.submeshCount; i++)
		{
			// Create submesh
			ModelSubmesh* submeshPtr = (ModelSubmesh*)TypeManager::GetInstance()->CreateObjectByTypeInfo(ModelSubmesh::GetStaticTypeInfo());
		
			// Load from current stream
			submeshPtr->Load(stream);

			m_submeshes.push_back(std::shared_ptr<ModelSubmesh >(submeshPtr, ObjectDeleter));
		}
	}

	void Model::Save(const std::shared_ptr<DataStream>& stream)
	{
#ifdef FINAL_BUILD
		Core::Error("Model::Save: Canno't save in the final build.");
#endif // FINAL_BUILD

		ModelFileHeader header = { 0 };
		header.magic = MODELFILE_MAGIC;
		header.version = kModelFileVersion;
		header.submeshCount = (uint32_t)m_submeshes.size();
		stream->Write(&header);

		for (auto pSubMesh : m_submeshes)
		{
			pSubMesh->Save(stream);
		}
	}

	void Model::CreateHw()
	{
		return;
			
		for (auto pSubMesh : m_submeshes)
		{
			pSubMesh->CreateHw();
		}
	}

	void Model::ReleaseHw()
	{
		return;

		for (auto pSubMesh : m_submeshes)
		{
			pSubMesh->ReleaseHw();
		}
	}

	const std::vector<std::shared_ptr<ModelSubmesh>>& Model::GetModelSubmeshes()
	{
		return m_submeshes;
	}

}
