#include "graphicspch.h"

#include "core/file/datastream.h"

// Graphics core
#include "graphics/core/device.h"
#include "graphics/core/texture.h"
#include "graphics/core/buffer.h"

#include "graphics/renderer.h"
#include "graphics/material.h"
#include "graphics/animatedmodel.h"
#include "graphics/mesh.h"
#include "graphics/shaderconstantmanager.h"
#include "graphics/debugrenderer.h"
#include "graphics/screenquad.h"

#include "glm/ext.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace solunar
{

// #TODO: shit move to vertex.h\.cpp
InputLayoutDesc g_vertexInputLayout[5] =
{
	{ "POSITION", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(Vertex, m_position), INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(Vertex, m_normal), INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, ImageFormat::RG32F, 0, (UINT)offsetof(Vertex, m_texcoord0), INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(Vertex, m_tangent), INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(Vertex, m_bitangent), INPUT_PER_VERTEX_DATA, 0 }
};

InputLayoutDesc g_animatedVertexInputLayout[7] =
{
	{ "POSITION", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(AnimatedVertex, m_position), INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(AnimatedVertex, m_normal), INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, ImageFormat::RG32F, 0, (UINT)offsetof(AnimatedVertex, m_texcoord), INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(AnimatedVertex, m_tangent), INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, ImageFormat::RGB32F, 0, (UINT)offsetof(AnimatedVertex, m_bitangent), INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_weights), INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDINDICES", 0, ImageFormat::RGBA32F, 0, (UINT)offsetof(AnimatedVertex, m_boneIDs), INPUT_PER_VERTEX_DATA, 0 }
};

// UnpackValue
template <typename T>
bool gltfUnpackValues(const cgltf_primitive& primitive, int64_t index, int64_t vtxCount, std::vector<T>& data)
{
	if (index == -1) {
		Core::Msg("gltfUnpackValue: Invalid index %i", index);
		return false;
	}

	const auto& Attributes = primitive.attributes[index];
	const size_t NumComponents = cgltf_num_components(Attributes.data->type);
	if (NumComponents != (sizeof(T) / sizeof(float))) {
		Core::Msg("gltfUnpackValue: Mesh doesn't contain invalid attribute %s information.", Attributes.name);
		return false;
	}

	data.resize(vtxCount);
	cgltf_accessor_unpack_floats(Attributes.data, (float*)data.data(), vtxCount * NumComponents);
	return true;
}

inline InterpolationType getInterpolationType(cgltf_interpolation_type type)
{
	switch (type)
	{
	case cgltf_interpolation_type_linear:
		return InterpolationType_Linear;
	
	case cgltf_interpolation_type_step:
		return InterpolationType_Step;
	
	case cgltf_interpolation_type_cubic_spline:
		return InterpolationType_CubicSpline;

	default:
		break;
	}

	return InterpolationType_Unknown;
}

inline AnimationPathType getAnimationPathType(cgltf_animation_path_type type)
{
	switch (type)
	{
	case cgltf_animation_path_type_invalid:
		break;
	
	case cgltf_animation_path_type_translation:
		return AnimationPathType_Translation;
	
	case cgltf_animation_path_type_rotation:
		return AnimationPathType_Rotation;
	
	case cgltf_animation_path_type_scale:
		return AnimationPathType_Scale;
	
	case cgltf_animation_path_type_weights:
		return AnimationPathType_Weights;

	default:
		break;
	}

	return AnimationPathType_Unknown;
}

IMPLEMENT_OBJECT(AnimatedModel, ModelBase);

// Object registering
void AnimatedModel::RegisterObject()
{
	TypeManager::GetInstance()->RegisterObject<AnimatedModel>();
}

AnimatedModel::AnimatedModel() :
	m_animationId(-1),
	m_baseModel(nullptr)
{
	m_boundingBox.SetIdentity();

	for (int i = 0; i < sizeof(m_bonesMatrices) / sizeof(m_bonesMatrices[0]); i++)
	{
		m_bonesMatrices[i] = glm::mat4(1.0f);
	}
}

AnimatedModel::~AnimatedModel()
{
	ReleaseHw();
}

void AnimatedModel::Load(const std::shared_ptr<DataStream>& stream)
{
	Load_GLTF(stream);
}

void AnimatedModel::Load_GLTF(const std::shared_ptr<DataStream>& stream)
{
	stream->Seek(Seek_End, 0);
	size_t filesize = stream->Tell();
	stream->Seek(Seek_Begin, 0);

	uint8_t* fileData = (uint8_t*)malloc(filesize);
	stream->Read(fileData, filesize);

	cgltf_data* data = NULL;
	cgltf_options options = {};
	cgltf_result result = cgltf_parse(&options, fileData, filesize, &data);

	if (result != cgltf_result_success) {
		Core::Msg("AnimatedModel: Failed to parse GLTF asset.");
		return;
	}

	result = cgltf_load_buffers(&options, data, nullptr);
	if (result != cgltf_result_success) {
		cgltf_free(data);
		Core::Msg("AnimatedModel: Failed to load buffers for GLTF asset .");
		return;
	}

	for (int i = 0; i < data->meshes_count; i++) {
		const cgltf_mesh& mesh = data->meshes[i];
		
		// #TODO: more than one primitive !!!
		const cgltf_primitive& primitive = mesh.primitives[0];

		const size_t vtxCount = primitive.attributes[0].data->count;
		const size_t idxCount = primitive.indices->count;
		
		int64_t attributePosition = -1;
		int64_t attributeNormal = -1;
		int64_t attributeTexcoord = -1;
		int64_t attributeTangent = -1;
		int64_t attributeBoneIds = -1;
		int64_t attributeWeights = -1;

		for (int j = 0; j < primitive.attributes_count; j++) {
			const auto& attribute = primitive.attributes[j];
			std::string name = std::string(attribute.name);
			if (attribute.data->count != vtxCount) {
				Core::Msg("AnimatedModel: Invalid attribute count in mesh %s for attribute %i.", mesh.name, (int)attribute.type);
				return;
			}

			if (attribute.type == cgltf_attribute_type_position) {
				attributePosition = j;
			}
			
			if (attribute.type == cgltf_attribute_type_normal) {
				attributeNormal = j;
			}
			
			if (attribute.type == cgltf_attribute_type_texcoord) {
				attributeTexcoord = j;
			}

			if (attribute.type == cgltf_attribute_type_tangent) {
				attributeTangent = j;
			}

			if (attribute.type == cgltf_attribute_type_joints) {
				attributeBoneIds = j;
			}

			if (attribute.type == cgltf_attribute_type_weights) {
				attributeWeights = j;
			}
		}

		std::vector<uint32_t> indices;
		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> texcoords;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec4> tangents;
		std::vector<glm::vec4> joints;
		std::vector<glm::vec4> weights;

		indices.resize(idxCount);
		cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(uint32_t), idxCount);

		gltfUnpackValues(primitive, attributePosition, vtxCount, positions);
		gltfUnpackValues(primitive, attributeTexcoord, vtxCount, texcoords);
		gltfUnpackValues(primitive, attributeNormal, vtxCount, normals);
		gltfUnpackValues(primitive, attributeTangent, vtxCount, tangents);
		gltfUnpackValues(primitive, attributeBoneIds, vtxCount, joints);
		gltfUnpackValues(primitive, attributeWeights, vtxCount, weights);

		// flip uvs
		for (auto& it : texcoords) {
			it.y = 1.0f - it.y;
		}

		// Bounding box calculation
#if 0
		BoundingBox AABB;
		AABB.SetIdentity();

		for (size_t o = 0; o < vtxCount; o++) {
			//positions[o].z = -positions[o].z;  // Sparkle uses LH Y+
			AABB.m_min = glm::min(positions[o], AABB.m_min);
			AABB.m_max = glm::max(positions[o], AABB.m_max);
		}

		m_boundingBox = AABB;
#else
		for (size_t o = 0; o < vtxCount; o++) {
			//positions[o].z = -positions[o].z;  // Sparkle uses LH Y+
			m_boundingBox.m_min = glm::min(positions[o], m_boundingBox.m_min);
			m_boundingBox.m_max = glm::max(positions[o], m_boundingBox.m_max);
		}
#endif

		// calculate vertices
		std::vector<AnimatedVertex> vertices;
		for (size_t o = 0; o < vtxCount; o++) {
			AnimatedVertex vertex;
			vertex.m_position = positions[o];

			if (normals.size() > 0) {
				vertex.m_normal = normals[o];
			} else {
				vertex.m_normal = glm::vec3(0.0f);
			}

			if (texcoords.size() > 0) {
				vertex.m_texcoord = texcoords[o];
			} else {
				vertex.m_texcoord = glm::vec2(0.0f);
			}

			if (tangents.size() > 0) {
				vertex.m_tangent = glm::vec3(tangents[o]);
			} else {
				vertex.m_tangent = glm::vec3(0.0f);
			}

			vertex.m_bitangent = glm::cross(vertex.m_normal, vertex.m_tangent);

			if (joints.size() > 0) {
				vertex.m_boneIDs = joints[o];
			} else {
				vertex.m_boneIDs = glm::vec4(0.0f);
			}

			if (weights.size() > 0) {
				vertex.m_weights = weights[o];
			} else {
				vertex.m_weights = glm::vec4(0.0f);
			}
;
			vertices.push_back(vertex);
		}

		std::string materialname = "materials/default_material.xml";
		if (primitive.material && strlen(primitive.material->name) > 0)
			materialname = std::string("materials/models/") + std::string(primitive.material->name) + ".xml"; // #TODO: SHIT !!!!

#if 0
		if (data->materials_count != data->meshes_count)
			Core::Msg("AnimatedModel::Load_GLTF: wrong material count! using default ...");

		if (data->materials_count < data->meshes_count)
			Core::Msg("AnimatedModel::Load_GLTF: material count less than mesh count (%i < %i)! using default ...",
				data->materials_count, data->meshes_count);

		// #TODO: This shit should be rewritten !!!
		//if (data->materials_count == data->meshes_count)
		if (i < data->materials_count)
		{
			if (data->materials[i].name && strlen(data->materials[i].name) <= 0)
				Core::Msg("AnimatedModel::Load_GLTF: material name on mesh %i are empty! using default ...", i);
			else if (data->materials[i].name && strlen(data->materials[i].name) > 0)
				materialname = std::string("materials/models/") + std::string(data->materials[i].name) + ".xml"; // #TODO: SHIT !!!!
		}
#endif

		AnimatedSubMesh* submesh = mem_new<AnimatedSubMesh>();
		submesh->m_vertices = vertices;
		submesh->m_verticesCount = (uint32_t)vertices.size();
		submesh->m_indices = indices;
		submesh->m_indicesCount = (uint32_t)indices.size();
		submesh->m_materialName = materialname;
		m_subMeshes.push_back(submesh);
	}

	// loading animations
	m_animations.resize( data->animations_count);

	for (int i = 0; i < data->animations_count; i++) {
		const cgltf_animation& gltf_animation = data->animations[i];

		Animation& animation = m_animations[i];
		animation.m_name = "unknowed animation name";
		if (gltf_animation.name)
			animation.m_name = gltf_animation.name;

		animation.m_samplers.resize(gltf_animation.samplers_count);
		for (int j = 0; j < gltf_animation.samplers_count ; j++) {
			const cgltf_animation_sampler& gltf_animationSampler = gltf_animation.samplers[j];

			AnimationSampler& animationSampler = animation.m_samplers[j];

			animationSampler.m_interpolationType = getInterpolationType(gltf_animationSampler.interpolation);

			Assert(animationSampler.m_interpolationType != InterpolationType_Unknown);

			size_t inputCount = gltf_animationSampler.input->count;
			animationSampler.m_inputs.resize(inputCount);
			cgltf_accessor_unpack_floats(gltf_animationSampler.input, (float*)animationSampler.m_inputs.data(), inputCount);

			size_t outputCount = gltf_animationSampler.output->count;
			animationSampler.m_outputs.resize(outputCount);
			
			if (gltf_animationSampler.output->type == cgltf_type_vec3) {
				for (int k = 0; k < outputCount ; k++) {
					glm::vec3 outputValue = glm::vec3(0.0f);
					cgltf_accessor_read_float(gltf_animationSampler.output, k, (float*)&outputValue, sizeof(glm::vec3));
					animationSampler.m_outputs[k] = glm::vec4(outputValue, 0.0f);
				}	
			} else if (gltf_animationSampler.output->type == cgltf_type_vec4) {
				cgltf_accessor_unpack_floats(gltf_animationSampler.output, (float*)animationSampler.m_outputs.data(), outputCount * sizeof(glm::vec4));
			}
		}

		for (const auto& samplers : animation.m_samplers) {
			for (const auto& input : samplers.m_inputs) {
				if (input < animation.m_startTime) animation.m_startTime = input;
				if (input > animation.m_endTime) animation.m_endTime = input;
			}
		}

		animation.m_channels.resize(gltf_animation.channels_count);
		for (int j = 0; j < gltf_animation.channels_count; j++) {
			const cgltf_animation_channel& gltf_animationChannel = gltf_animation.channels[j];

			AnimationChannel& animationChannel = animation.m_channels[j];
			animationChannel.m_pathType = getAnimationPathType(gltf_animationChannel.target_path);
			
			Assert(animationChannel.m_pathType != AnimationPathType_Unknown);

			animationChannel.m_nodeId = (int)cgltf_node_index(data, gltf_animationChannel.target_node);
			animationChannel.m_samplerId = (int)cgltf_animation_sampler_index(&gltf_animation, gltf_animationChannel.sampler);
		}

		//Core::Msg("AnimatedModel: animation %s channels %i", gltf_animation.name, gltf_animation.channels_count);
	}

	// loading model skin
	m_skins.resize(data->skins_count);
	for (int i = 0; i < data->skins_count; i++) {
		const cgltf_skin& gltf_skin = data->skins[i];
	//	m_joints.resize(gltf_skin.joints_count);

		AnimationSkin& skin = m_skins[i];
		skin.m_name = "unknowed skin name";
		if (gltf_skin.name)
			skin.m_name = gltf_skin.name;

		skin.m_joints.resize(gltf_skin.joints_count);

		// Load joint too
		for (int j = 0; j < gltf_skin.joints_count; j++) {
			const cgltf_node* joint = gltf_skin.joints[j];

			int jointIndex = (int)cgltf_node_index(data, joint);
			skin.m_joints[j] = jointIndex;
#if 0
			if (joint->name) {
				m_joints[jointIndex].m_name = joint->name;
			} else {
				char buf[32];
				snprintf(buf, sizeof(buf), "Joint %i", j);
				m_joints[jointIndex].m_name = buf;
			}

			if (joint->has_translation)
				m_joints[jointIndex].m_translation = glm::vec3(joint->translation[0], joint->translation[1], joint->translation[2]);
			else
				m_joints[jointIndex].m_translation = glm::vec3(0.0f);

			if (joint->has_rotation)
				m_joints[jointIndex].m_rotation = glm::quat(joint->rotation[3], joint->rotation[0], joint->rotation[1], joint->rotation[2]);
			else
				m_joints[jointIndex].m_rotation = glm::identity<glm::quat>();

			if (joint->has_scale)
				m_joints[jointIndex].m_scale = glm::vec3(joint->scale[0], joint->scale[1], joint->scale[2]);
			else
				m_joints[jointIndex].m_scale = glm::vec3(0.0f);
			
			if (joint->has_matrix)
				memcpy(&m_joints[jointIndex].m_matrix[0][0], &joint->matrix[0], 16 * sizeof(float));
			else
				m_joints[jointIndex].m_matrix = glm::mat4(1.0f);
#endif	
		}

		// Load the inverse bind matrices
		size_t inverseBindMatricesCount = gltf_skin.inverse_bind_matrices->count;
		skin.m_inverseBindMatrices.resize(inverseBindMatricesCount);
		cgltf_accessor_unpack_floats(gltf_skin.inverse_bind_matrices, (float*)skin.m_inverseBindMatrices.data(), inverseBindMatricesCount * 16);
		
		if (gltf_skin.skeleton)
			skin.m_skeletonRootId = (int)cgltf_node_index(data, gltf_skin.skeleton);

		m_skins.push_back(skin);
	}

	m_nodes.resize(data->nodes_count);

	for (int i = 0; i < data->nodes_count; i++) {
		auto& node = m_nodes[i];
		node.m_name = data->nodes[i].name;

		if (data->nodes[i].has_translation)
			node.m_translation = glm::vec3(data->nodes[i].translation[0], data->nodes[i].translation[1], data->nodes[i].translation[2]);
		else
			node.m_translation = glm::vec3(0.0f);

		if (data->nodes[i].has_rotation)
			node.m_rotation = glm::quat(data->nodes[i].rotation[3], data->nodes[i].rotation[0], data->nodes[i].rotation[1], data->nodes[i].rotation[2]);
		else
			node.m_rotation = glm::identity<glm::quat>();

		if (data->nodes[i].has_scale)
			node.m_scale = glm::vec3(data->nodes[i].scale[0], data->nodes[i].scale[1], data->nodes[i].scale[2]);
		else
			node.m_scale = glm::vec3(1.0f);

		if (data->nodes[i].has_matrix)
			memcpy(&node.m_matrix[0][0], &data->nodes[i].matrix[0], 16 * sizeof(float));
		else
			node.m_matrix = glm::mat4(1.0f);

		if (data->nodes[i].parent)
			node.m_parentId = (int)cgltf_node_index(data, data->nodes[i].parent);

		if (data->nodes[i].children_count > 0) {
			node.m_children.resize(data->nodes[i].children_count);
			for (int j = 0; j < data->nodes[i].children_count; j++) {
				node.m_children[j] = (int)cgltf_node_index(data, data->nodes[i].children[j]);
			}
		}

		if (data->nodes[i].skin)
			node.m_skinId = (int)cgltf_skin_index(data, data->nodes[i].skin);
	}

#if 1
	m_rootNodeId = (int)m_nodes.size();
	m_nodes.resize(m_nodes.size() + 1);
	
	for (int i = 0; i < m_nodes.size() - 1; ++i) {
		if (m_nodes[i].m_parentId == -1) {
			m_nodes[i].m_parentId = m_rootNodeId;
			m_nodes[m_rootNodeId].m_children.push_back(i);
		}
	}
	// main rotate Y
	m_nodes[m_rootNodeId].m_name = "Root Node";
	m_nodes[m_rootNodeId].m_skinId = -1;
	m_nodes[m_rootNodeId].m_parentId = -1;
	m_nodes[m_rootNodeId].m_matrix = glm::mat4(1.0f);
	m_nodes[m_rootNodeId].m_translation = glm::vec3(0.0f);
	m_nodes[m_rootNodeId].m_scale = glm::vec3(1.0f);//glm::vec3(0.1f);
	m_nodes[m_rootNodeId].m_rotation.x = 0;
	m_nodes[m_rootNodeId].m_rotation.y = sinf(180.0 / 2.0 / 180.0 * maths::PI);
	m_nodes[m_rootNodeId].m_rotation.z = 0;
	m_nodes[m_rootNodeId].m_rotation.w = cosf(180.0 / 2.0 / 180.0 * maths::PI);

	//m_nodes[rootNode].m_rotation = glm::quat(0, 6, 1, 0);
#endif
	//for (auto it : m_joints)
	//	m_bones.emplace(it.m_name, it);

	/// todo !!!!!!!!!!!!!! cgltf_free(data);

	// #TODO: cgltf has very SLOW memory freeing, fix with fixed allocator or custom allocator
#ifdef NDEBUG
	//cgltf_free(data);
#endif // !NDEBUG
	cgltf_free(data);

	free(fileData);

	CreateHw();
}

template<typename POD>
void Deserialize(const std::shared_ptr<DataStream>& os, std::vector<POD>& v)
{
	// this only works on built in data types (PODs)
	static_assert(std::is_trivial<POD>::value && std::is_standard_layout<POD>::value,
		"Can only serialize POD types with this function");

	uint32_t size = 0;
	os->Read((void*)&size, sizeof(size));

	v.resize(size);
	os->Read((void*)v.data(), v.size() * sizeof(POD));
}


void Deserialize(const std::shared_ptr<DataStream>& os, std::string& v)
{
	uint32_t size;
	os->Read((void*)&size, sizeof(size));
	v.resize(size + 1);
	os->Read((void*)v.data(), v.size() * sizeof(std::string::value_type));
	v[size + 1] = '\0';
}

void AnimatedModel::Load_Model(const std::shared_ptr<DataStream>& dataStream)
{
	ModelFileHeader header;
	dataStream->Read(&header);

	// animations
	uint32_t animationsCount;
	dataStream->Read(&animationsCount);

	// skins
	uint32_t skinsCount;
	dataStream->Read(&skinsCount);

	// node count
	uint32_t nodeCount;
	dataStream->Read(&nodeCount);

#if 0
	for (int i = 0; i < animationsCount; i++)
	{
		Animation animation;
		Deserialize(dataStream, animation.m_name);

		uint16_t samplersCount;
		dataStream->Read(&samplersCount);

		for (const auto& sampler : animation.m_samplers)
		{
			uint8_t interpolationType = sampler.m_interpolationType;
			dataStream->Write(&interpolationType);
			Serialize(dataStream, sampler.m_inputs);
			Serialize(dataStream, sampler.m_outputs);
		}

		for (const auto& channels : animation.m_channels)
		{
			uint8_t pathType = channels.m_pathType;
			dataStream->Write(&pathType);
			dataStream->Write(&channels.m_nodeId);
			dataStream->Write(&channels.m_samplerId);
		}

		dataStream->Write(&animation.m_startTime);
		dataStream->Write(&animation.m_endTime);
	}

	for (const auto& skin : m_skins)
	{
		Serialize(dataStream, skin.m_name);
		Serialize(dataStream, skin.m_joints);
		Serialize(dataStream, skin.m_inverseBindMatrices);
		dataStream->Write(&skin.m_skeletonRootId);
	}

	for (const auto& node : m_nodes)
	{
		Serialize(dataStream, node.m_name);
		dataStream->Write(&node.m_translation);
		dataStream->Write(&node.m_rotation);
		dataStream->Write(&node.m_scale);
		dataStream->Write(&node.m_matrix);
		Serialize(dataStream, node.m_children);
		dataStream->Write(&node.m_parentId);
		dataStream->Write(&node.m_skinId);
	}

	for (const auto& submesh : m_subMeshes)
	{
		ModelFileSubmeshData submeshData;
		memset(&submeshData, 0, sizeof(submeshData));
		strcpy(submeshData.materialInfo, submesh->m_materialName.c_str());
		submeshData.verticesCount = submesh->m_verticesCount;
		submeshData.indicesCount = submesh->m_indicesCount;
		dataStream->Write(&submeshData);

		AnimatedVertex* pVertices = (AnimatedVertex*)submesh->m_vertices.data();
		dataStream->Write(pVertices, submeshData.verticesCount * sizeof(Vertex));

		unsigned int* pIndices = (unsigned int*)submesh->m_indices.data();
		dataStream->Write(pIndices, submeshData.indicesCount * sizeof(uint32_t));
	}
#endif
}

template<typename POD>
void Serialize(const std::shared_ptr<DataStream>& os, std::vector<POD> const& v)
{
	// this only works on built in data types (PODs)
	static_assert(std::is_trivial<POD>::value && std::is_standard_layout<POD>::value,
		"Can only serialize POD types with this function");

	uint32_t size = v.size();
	os->Write((void*)&size, sizeof(size));
	os->Write((void*)v.data(), v.size() * sizeof(POD));
}

void Serialize(const std::shared_ptr<DataStream>& os, std::string const& v)
{
	uint32_t size = v.size();
	os->Write((void*)&size, sizeof(size));
	os->Write((void*)v.data(), v.size() * sizeof(std::string::value_type));
}

//template<typename POD>
//std::istream& deserialize(const std::shared_ptr<DataStream>& is, std::vector<POD>& v)
//{
//	static_assert(std::is_trivial<POD>::value && std::is_standard_layout<POD>::value,
//		"Can only deserialize POD types with this function");
//
//	decltype(v.size()) size;
//	is.read(reinterpret_cast<char*>(&size), sizeof(size));
//	v.resize(size);
//	is.read(reinterpret_cast<char*>(v.data()), v.size() * sizeof(POD));
//	return is;
//}

void AnimatedModel::Save(const std::shared_ptr<DataStream>& dataStream)
{
	ModelFileHeader header;
	header.magic = MODELFILE_MAGIC;
	header.version = 2;
	header.submeshCount = m_subMeshes.size();
	dataStream->Write(&header);

	// animations
	uint32_t animationsCount = m_animations.size();
	dataStream->Write(&animationsCount);

	// skins
	uint32_t skinsCount = m_skins.size();
	dataStream->Write(&skinsCount);

	// node count
	uint32_t nodeCount = m_nodes.size();
	dataStream->Write(&nodeCount);

	for (const auto& animation : m_animations)
	{
		Serialize(dataStream, animation.m_name);

		uint16_t samplersCount = animation.m_samplers.size();
		dataStream->Write(&samplersCount);

		for (const auto& sampler : animation.m_samplers)
		{
			uint8_t interpolationType = sampler.m_interpolationType;
			dataStream->Write(&interpolationType);
			Serialize(dataStream, sampler.m_inputs);
			Serialize(dataStream, sampler.m_outputs);
		}

		for (const auto& channels : animation.m_channels)
		{
			uint8_t pathType = channels.m_pathType;
			dataStream->Write(&pathType);
			dataStream->Write(&channels.m_nodeId);
			dataStream->Write(&channels.m_samplerId);
		}

		dataStream->Write(&animation.m_startTime);
		dataStream->Write(&animation.m_endTime);
	}

	for (const auto& skin : m_skins)
	{
		Serialize(dataStream, skin.m_name);
		Serialize(dataStream, skin.m_joints);
		Serialize(dataStream, skin.m_inverseBindMatrices);
		dataStream->Write(&skin.m_skeletonRootId);
	}

	for (const auto& node : m_nodes)
	{
		Serialize(dataStream, node.m_name);
		dataStream->Write(&node.m_translation);
		dataStream->Write(&node.m_rotation);
		dataStream->Write(&node.m_scale);
		dataStream->Write(&node.m_matrix);
		Serialize(dataStream, node.m_children);
		dataStream->Write(&node.m_parentId);
		dataStream->Write(&node.m_skinId);
	}

	for (const auto& submesh : m_subMeshes)
	{
		ModelFileSubmeshData submeshData;
		memset(&submeshData, 0, sizeof(submeshData));
		strcpy(submeshData.materialInfo, submesh->m_materialName.c_str());
		submeshData.verticesCount = submesh->m_verticesCount;
		submeshData.indicesCount = submesh->m_indicesCount;
		dataStream->Write(&submeshData);

		AnimatedVertex* pVertices = (AnimatedVertex*)submesh->m_vertices.data();
		dataStream->Write(pVertices, submeshData.verticesCount * sizeof(Vertex));

		unsigned int* pIndices = (unsigned int*)submesh->m_indices.data();
		dataStream->Write(pIndices, submeshData.indicesCount * sizeof(uint32_t));
	}
}

void AnimatedModel::CreateHw()
{	
	// do not create hardware data when animated model is clone
	if (m_instance)
		return;
	
	for (int i = 0; i < m_subMeshes.size(); i++)
	{
		m_subMeshes[i]->Create();
		//AnimatedSubMesh* submesh = m_subMeshes[i];
		
	}
}

void AnimatedModel::ReleaseHw()
{
	// do not delete hardware data when animated model is clone
	if (m_instance)
		return;

	for (int i = 0; i < m_subMeshes.size(); i++)
	{
		AnimatedSubMesh* submesh = m_subMeshes[i];
		submesh->m_material.reset();

		mem_delete(submesh->m_indexBuffer);
		mem_delete(submesh->m_vertexBuffer);

		mem_delete(submesh);
	}

	m_subMeshes.clear();
}

int AnimatedModel::GetAnimationByName(const std::string& name)
{
	if (m_instance)
		return m_baseModel->GetAnimationByName(name);

	for (int i = 0; i < m_animations.size(); i++)
		if (m_animations[i].m_name == name)
			return i;

	return -1;
}

Animation* AnimatedModel::GetAnimationByIndex(int index)
{
	return &m_animations[index];
}

int AnimatedModel::GetCurrentAnimationId()
{
	return m_animationId;
}

void AnimatedModel::PlayAnimation(int index, bool looped)
{
	Assert(index != -1);

	m_currentAnimation = m_instance ? m_baseModel->GetAnimationByIndex(index) : GetAnimationByIndex(index); //&m_animations[m_animationId];
	m_animationId = index;
	m_playLooped = looped;
	m_currentTime = 0.0f;
	m_play = true;
}

void AnimatedModel::PauseAnimation()
{
	m_speed = 0.0f;
}

Animation* AnimatedModel::GetCurrentAnimation()
{
	return m_currentAnimation;
}

float AnimatedModel::GetCurrentTime()
{
	return m_currentTime;
}

bool AnimatedModel::IsStoped()
{
	return !m_play;
}

inline glm::quat samplerRotationToGlm(const glm::vec4& v)
{
	return glm::quat(v.w, v.x, v.y, v.z);
}

void AnimatedModel::Update(float dt)
{
	if (!m_play)
		return;

	m_currentTime += m_speed * dt;
	
	bool updated = false;
	Animation& animation = *m_currentAnimation;

	if (animation.m_startTime >= m_currentTime)
		m_currentTime = animation.m_startTime;

	if (m_currentTime >= animation.m_endTime && m_playLooped) {
		m_currentTime = animation.m_startTime; // #TODO: handle animation end
	} else if (m_currentTime >= animation.m_endTime) {
		m_currentTime = 0.0f;
		m_play = false;
	}

	float time = std::fmod(static_cast<float>(m_currentTime), animation.m_endTime - animation.m_startTime);
	
	for (auto& channel : animation.m_channels)
	{
		AnimationSampler& sampler = animation.m_samplers[channel.m_samplerId];
		if (sampler.m_inputs.size() > sampler.m_outputs.size())
			continue;

		for (std::size_t i = 0; i < sampler.m_inputs.size() - 1; ++i)
		{
			if ((time >= sampler.m_inputs[i]) && (time <= sampler.m_inputs[i + 1]))
			{
				float u = std::max(0.0f, time - sampler.m_inputs[i]) / (sampler.m_inputs[i + 1] - sampler.m_inputs[i]);
				if (u <= 1.0f)
				{
					if (channel.m_pathType == AnimationPathType_Translation)
					{
						auto A = sampler.m_outputs[i];
						auto B = sampler.m_outputs[i + 1];
						auto translation = glm::lerp(A, B, u);
						m_nodes[channel.m_nodeId].m_translation = translation;
					}
					else if (channel.m_pathType == AnimationPathType_Scale)
					{
						auto A = sampler.m_outputs[i];
						auto B = sampler.m_outputs[i + 1];
						auto scale = glm::lerp(A, B, u);
						m_nodes[channel.m_nodeId].m_scale = scale;
					}
					else if (channel.m_pathType == AnimationPathType_Rotation)
					{
						auto A = samplerRotationToGlm(sampler.m_outputs[i]);
						auto B = samplerRotationToGlm(sampler.m_outputs[i + 1]);
						auto rotate = glm::slerp(A, B, u);
						m_nodes[channel.m_nodeId].m_rotation = glm::normalize(rotate);
					}

					updated = true;
				}
			}
		}
	}
	
	if (updated == true)
	{
		UpdateNodeTransform(m_rootNodeId);

		for (int i = 0; i < (int)m_nodes.size() - 1; i++)
		{
			UpdateNode(i);
		}
	}
}

void AnimatedModel::UpdateNode(int node_id)
{
	auto& node = m_nodes[node_id];
	if (node.m_skinId != -1) {
		auto& skin = m_skins[node.m_skinId];
		size_t numJoints = skin.m_joints.size();
		for (size_t i = 0; i < numJoints; ++i)
		{
			m_bonesMatrices[i] = GetNodeMatrix(skin.m_joints[i]) *  skin.m_inverseBindMatrices[i] ;
		}
	}

	for (int i = 0; i < node.m_children.size(); i++)
		UpdateNode(node.m_children[i]);

}

void AnimatedModel::UpdateNodeTransform(int node_id) {
	Assert(node_id != -1);
	AnimationNode& node = m_nodes[node_id];

	glm::mat4 translation = glm::translate(
		glm::mat4(1.0f), node.m_translation) * 
		glm::toMat4(node.m_rotation) * 
		glm::scale(glm::mat4(1.0f), node.m_scale);

	if (node.m_parentId != -1) {
		auto& nodeParent = m_nodes[node.m_parentId];
		
		translation = nodeParent.m_matrix * translation;
	}

	node.m_matrix = translation;

	for (int i = 0; i < node.m_children.size(); i++)
		UpdateNodeTransform(node.m_children[i]);
}

const glm::mat4& AnimatedModel::GetNodeMatrix(int nodeId)
{
	Assert(nodeId != -1);
	AnimationNode& node = m_nodes[nodeId];
	return node.m_matrix;
}

const BoundingBox& AnimatedModel::GetBoundingBox()
{
	return m_boundingBox;
}

int AnimatedModel::GetNodeByName(const std::string& name)
{
	for (int i = 0; i < m_nodes.size(); i++)
		if (m_nodes[i].m_name == name)
			return i;

	return -1;
}

void AnimatedModel::SetNodeScale(int nodeid, const glm::vec3& scale)
{
	Assert(nodeid != -1);
	m_nodes[nodeid].m_scale = scale;
}

void AnimatedModel::DebugRender(const glm::mat4& modelMatrix)
{
	for (int i = 0; i < (int)m_nodes.size(); i++)
	{
		auto& node = m_nodes[i];
		if (node.m_skinId != -1) {
			auto& skin = m_skins[node.m_skinId];
			size_t numJoints = skin.m_joints.size();
			for (size_t i = 0; i < numJoints; ++i)
			{
				const glm::mat4 trans = modelMatrix * GetNodeMatrix(skin.m_joints[i]);

				g_debugRender.PushModelMatrix(trans);
				g_debugRender.drawAxis(glm::vec3(0.0f));
				g_debugRender.PopModelMatrix();
			}
		}
	}

#if 0
	for (int i = 0; i < (int)m_nodes.size() - 1; i++)
	{
		const glm::mat4 trans = modelMatrix * m_bonesMatrices[i];

		g_debugRender.PushModelMatrix(trans);
		g_debugRender.drawAxis(glm::vec3(0.0f));
		g_debugRender.PopModelMatrix();
	}
#endif
}

std::shared_ptr<AnimatedModel> AnimatedModel::Clone()
{
	// create object instance
	AnimatedModel* pObjectInstance = (AnimatedModel*)TypeManager::GetInstance()->CreateObjectByTypeInfo(AnimatedModel::GetStaticTypeInfo());

	// create from instance the smart pointer
	std::shared_ptr<AnimatedModel> object = std::shared_ptr<AnimatedModel>(pObjectInstance, ObjectDeleter);

	// set flag
	object->m_instance = true;
	object->m_baseModel = this;

	// clone everything
	object->m_subMeshes = m_subMeshes;
	//object->m_animations = m_animations;
	object->m_skins = m_skins;
	object->m_nodes = m_nodes;
	
	// animation data
	//object->m_currentAnimation = m_currentAnimation;
	//object->m_playLooped = m_playLooped;
	//object->m_play = m_play;
	//object->m_currentTime = m_currentTime;
	//object->m_speed = m_speed;

	object->m_rootNodeId = m_rootNodeId;
	
	// animation data x2
	//object->m_animationId = m_animationId;
	
	object->m_boundingBox = m_boundingBox;

	// memcpy for bone matrices
	memcpy(object->m_bonesMatrices, m_bonesMatrices, sizeof(m_bonesMatrices));

	return object;
}

///////////////////////////////////////////////////////////
// Animated Model Renderer

struct BonesCB
{
	glm::mat4 matrices[256];
};

ConstantBufferProxy g_bonesConstantBuffer;

AnimatedModelRenderer AnimatedModelRenderer::ms_instance;

AnimatedModelRenderer::AnimatedModelRenderer()
{
	m_jointTexture = nullptr;
	m_jointSampler = nullptr;
}

AnimatedModelRenderer::~AnimatedModelRenderer()
{
}

void AnimatedModelRenderer::Init()
{
	g_bonesConstantBuffer = ShaderConstantManager::GetInstance()->Create<BonesCB>("BonesCB");

#if 0
	// create a dynamic joint-palette texture and sampler

	TextureDesc textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.m_textureType = TextureType::Texture2D;
	textureDesc.m_width = MAX_JOINTS * 4;
	textureDesc.m_height = MAX_INSTANCES;
	textureDesc.m_mipmapLevel = 1;
	textureDesc.m_format = ImageFormat::RGBA32F;
	textureDesc.m_isStreamTexture = true;

	SubresourceDesc subresourceDesc;
	memset(&subresourceDesc, 0, sizeof(subresourceDesc));
	subresourceDesc.m_memoryPitch = textureDesc.m_width * 4;

	m_jointTexture = g_renderDevice->CreateTexture2D(textureDesc, subresourceDesc);

	SamplerDesc samplerDesc;
	memset(&samplerDesc, 0, sizeof(samplerDesc));
	samplerDesc.m_minFilter = TextureFilter::Nearest;
	samplerDesc.m_magFilter = TextureFilter::Nearest;
	samplerDesc.m_wrapS = TextureWrap::ClampToEdge;
	samplerDesc.m_wrapT = TextureWrap::ClampToEdge;
	samplerDesc.m_anisotropyLevel = 1.0f;

	m_jointSampler = g_renderDevice->CreateSamplerState(samplerDesc);
#endif
}

void AnimatedModelRenderer::Shutdown()
{
#if 0
	if (m_jointSampler)
	{
		mem_delete(m_jointSampler);
		m_jointSampler = nullptr;
	}

	if (m_jointTexture)
	{
		mem_delete(m_jointTexture);
		m_jointTexture = nullptr;
	}
#endif
}

void AnimatedModelRenderer::Render(AnimatedMeshComponent* model)
{
	std::shared_ptr<ModelBase> modelBase = model->LockModel();
	if (!modelBase)
		return;

	AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(modelBase.get());

	glm::mat4* data = (glm::mat4*)g_bonesConstantBuffer->Map(BufferMapping::WriteOnly);
	memcpy(data, animatedModel->m_bonesMatrices, sizeof(animatedModel->m_bonesMatrices));
	g_bonesConstantBuffer->Unmap();

	g_renderDevice->SetConstantBufferIndex(CBBindings_Skinning, g_bonesConstantBuffer.get());

#if 0
	glm::mat4* data = (glm::mat4*)g_bonesConstantBuffer->Map(BufferMapping::WriteOnly);
	memcpy(data, s_boneInfoTest, sizeof(s_boneInfoTest));
	g_bonesConstantBuffer->Unmap();

	g_renderDevice->SetConstantBufferIndex(CBBindings_Skinning, g_bonesConstantBuffer.get());
#endif

#if 0
	//std::shared_ptr<ModelBase> modelBase = model->LockModel();
	//AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(modelBase.get());

	// compute skinning matrices and write to joint texture upload buffer
	m_jointTexture->updateTexture(s_boneInfoTest, 0/*sizeof(s_boneInfoTest)*/, 0);

	// draw texture
	ScreenQuad::Render(m_jointTexture);
#endif
}

void AnimatedSubMesh::Create()
{
	BufferDesc bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.m_bufferType = BufferType::VertexBuffer;
	bufferDesc.m_bufferAccess = BufferAccess::Static;
	bufferDesc.m_bufferMemorySize = (uint32_t)m_vertices.size() * sizeof(AnimatedVertex);

	SubresourceDesc subresourceDesc;
	memset(&subresourceDesc, 0, sizeof(subresourceDesc));
	subresourceDesc.m_memory = m_vertices.data();
	subresourceDesc.m_memoryPitch = sizeof(AnimatedVertex);

	m_vertexBuffer = g_renderDevice->CreateBuffer(bufferDesc, subresourceDesc);
	//m_vertices.clear();

	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.m_bufferType = BufferType::IndexBuffer;
	bufferDesc.m_bufferAccess = BufferAccess::Static;
	bufferDesc.m_bufferMemorySize = (uint32_t)m_indices.size() * sizeof(uint32_t);

	memset(&subresourceDesc, 0, sizeof(subresourceDesc));
	subresourceDesc.m_memory = m_indices.data();
	subresourceDesc.m_memoryPitch = sizeof(uint32_t);

	m_indexBuffer = g_renderDevice->CreateBuffer(bufferDesc, subresourceDesc);
	//m_indices.clear();

	m_material = g_contentManager->LoadObject<Material>(m_materialName);
	if (m_material.expired())
		m_material = GetDefaultMaterial();
}

}