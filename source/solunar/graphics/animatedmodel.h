#ifndef ANIMATEDMODEL_H
#define ANIMATEDMODEL_H

#include "graphics/model.h"
#include "graphics/material.h"

namespace solunar
{

// the upper limit for joint palette size is 256 (because the mesh joint indices
// are stored in packed byte-size vertex formats), but the example mesh only needs less than 64
#define MAX_JOINTS 256 //(64) #TODO ::!!!!!!

// this defines the size of the instance-buffer and height of the joint-texture
#define MAX_INSTANCES (512)

// this defines the size of the vertex weight
#define MAX_BONE_WEIGHT 4

// Forward declaration
class ITexture2D;
class ISamplerState;
struct IBufferBase;

enum InterpolationType
{
	InterpolationType_Unknown,
	InterpolationType_Linear,
	InterpolationType_Step,
	InterpolationType_CubicSpline,
	InterpolationType_Count
};

enum AnimationPathType {
	AnimationPathType_Unknown,
	AnimationPathType_Translation,
	AnimationPathType_Rotation,
	AnimationPathType_Scale,
	AnimationPathType_Weights,
	AnimationPathType_Count
};

struct AnimatedVertex
{
	glm::vec3 m_position;
	glm::vec3 m_normal;
	glm::vec2 m_texcoord;
	glm::vec3 m_tangent;
	glm::vec3 m_bitangent;
	glm::vec4 m_weights;
	glm::vec4 m_boneIDs;
};

struct AnimatedSubMesh
{
	std::vector<AnimatedVertex> m_vertices;
	std::vector<uint32_t> m_indices;
	IBufferBase* m_vertexBuffer;
	IBufferBase* m_indexBuffer;
	std::weak_ptr<Material> m_material;
	std::string m_materialName;
	uint32_t m_verticesCount;
	uint32_t m_indicesCount;

	void Create();
};

struct AnimationSampler
{
	InterpolationType m_interpolationType = InterpolationType_Unknown;
	std::vector<float> m_inputs = {};
	std::vector<glm::vec4> m_outputs = {};
};

struct AnimationChannel {
	AnimationPathType m_pathType = AnimationPathType_Unknown;
	int m_nodeId = -1;
	int m_samplerId = -1;
};

struct Animation
{
	std::string m_name;
	std::vector<AnimationSampler> m_samplers;
	std::vector<AnimationChannel> m_channels;
	float m_startTime = FLT_MAX; //std::numeric_limits<float>::max();
	float m_endTime = -FLT_MAX;//std::numeric_limits<float>::min();
};

struct AnimationSkin
{
	std::string m_name = "";
	std::vector<int> m_joints = {};
	std::vector<glm::mat4> m_inverseBindMatrices = {};
	int m_skeletonRootId = -1;
};

struct AnimationNode
{
	std::string m_name;
	glm::vec3 m_translation;
	glm::quat m_rotation;
	glm::vec3 m_scale;
	glm::mat4 m_matrix;
	std::vector<int> m_children;
	int m_parentId = -1;
	int m_skinId = -1;
};

class AnimatedModel : public ModelBase
{
	DECLARE_OBJECT(AnimatedModel);
public:
	AnimatedModel();
	~AnimatedModel();

	static void RegisterObject();

	void Load(const std::shared_ptr<DataStream>& dataStream) override;
	void Load_GLTF(const std::shared_ptr<DataStream>& dataStream);
	void Load_Model(const std::shared_ptr<DataStream>& dataStream);

	void Save(const std::shared_ptr<DataStream>& dataStream) override;

	void CreateHw() override;
	void ReleaseHw() override;

	std::vector<AnimatedSubMesh*>& GetAnimatedSubmehes() { return m_subMeshes; }

	/** Animation accessing */
	int GetAnimationByName(const std::string& name);
	Animation* GetAnimationByIndex(int index);
	void PlayAnimation(int index, bool looped = false);
	void PauseAnimation();
	int GetCurrentAnimationId();
	Animation* GetCurrentAnimation();
	float GetCurrentTime();
	bool IsStoped();

	void Update(float dt);

	void UpdateNode(int node_id);
	void UpdateNodeTransform(int node_id);

	const glm::mat4& GetNodeMatrix(int nodeId);

	const BoundingBox& GetBoundingBox();

	/** Node accessing */
	int GetNodeByName(const std::string& name);
	int GetRootNodeId() { return m_rootNodeId; }
	void SetNodeScale(int nodeid, const glm::vec3& scale);

	void DebugRender(const glm::mat4& modelMatrix);

	bool HasSkin() { return m_skins.size(); }

	size_t GetAnimationCount() { return m_animations.size(); }

	std::shared_ptr< AnimatedModel > Clone();

private:
	std::vector<AnimatedSubMesh*> m_subMeshes;
	std::vector<Animation> m_animations;
	std::vector<AnimationSkin> m_skins;
	//std::vector<AnimationNode> m_joints;
	std::vector<AnimationNode> m_nodes;
	Animation* m_currentAnimation;
	bool m_playLooped = false;
	bool m_play = false;
	float m_currentTime = 0.0f;
	float m_speed = 1.0f;
	
	int m_rootNodeId;
	int m_animationId;

	BoundingBox m_boundingBox;

	AnimatedModel* m_baseModel;
	bool m_instance = false;

public:
	glm::mat4 m_bonesMatrices[MAX_JOINTS];
};

class AnimatedMeshComponent;

class AnimatedModelRenderer : public Singleton<AnimatedModelRenderer>
{
private:
	static AnimatedModelRenderer ms_instance;

public:
	AnimatedModelRenderer();
	~AnimatedModelRenderer();

	void Init();
	void Shutdown();

	void Render(AnimatedMeshComponent* model);

private:
	ITexture2D* m_jointTexture;
	ISamplerState* m_jointSampler;

};

extern InputLayoutDesc g_animatedVertexInputLayout[7];

}

#endif // !ANIMATEDMODEL_H
