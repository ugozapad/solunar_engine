#ifndef ENGINE_ENTITY_ENTITY_H
#define ENGINE_ENTITY_ENTITY_H

#include "core/math/boundingbox.h"

namespace solunar
{

class World;
class Component;

class Entity : public SerializableObject
{
	friend class World;
public:
	DECLARE_OBJECT(Entity);
	DECLARE_PROPERTY_REGISTER(Entity);

public:
	Entity();
	~Entity();

	virtual void LoadXML(tinyxml2::XMLElement& element);
	virtual void SaveXML(tinyxml2::XMLElement& element);

	virtual void OnWorldSet(World* world);

	void SetRootEntity(Entity* node);
	Entity* GetRootEntity() { return m_rootEntity; }

	void SetName(const std::string& name)	{ m_name = name; }
	std::string GetName() const				{ return m_name; }

	glm::vec3 GetPosition()	const	{ return m_position; }
	glm::quat GetRotation()	const	{ return m_rotation; }
	glm::vec3 GetScale()	const	{ return m_scale; }
	
	glm::vec3 GetWorldPosition() const;

	void SetPosition(const glm::vec3& pos) { m_position = pos; }
	void SetRotation(const glm::quat& rot) { m_rotation = rot; }
	void SetEulerRotation(const glm::vec3& rot);
	void SetScale(const glm::vec3& scale) { m_scale = scale; }

	void QuaternionRotate(const glm::vec3& axis, float angle);

	glm::mat4 GetWorldTranslation();
	glm::mat4 GetLocalTranslation();

	void SetBoundingBox(const BoundingBox& boundingBox);
	const BoundingBox& GetLocalBoundingBox();
	const BoundingBox& GetBoundingBox();

	Entity* CreateChild();
	Entity* CreateChildEx(const TypeInfo* typeInfo);

	Component* CreateComponentByTypeInfo(const TypeInfo* typeinfo);
	Component* GetComponentByTypeInfo(const TypeInfo* typeinfo);

	const std::vector<Component*>& GetAllComponents();

	void AddComponent(Component* component);
	void UpdateWorldTransform();

	template <typename T>
	T* CreateComponent();

	template <typename T>
	T* GetComponent();

	template <typename T>
	inline std::vector<T*> GetComponents();

private:
	void TransformBBox();

private:
	typedef std::vector<Component*>::iterator ComponentIt;
	std::vector<Component*> m_components;

	typedef std::vector<Entity*>::iterator EntityIt;
	std::vector<Entity*> m_children;

	World* m_world;

	Entity* m_rootEntity;

	std::string m_name;

	glm::mat4 m_worldTransform;
	BoundingBox m_boundingBox;
	BoundingBox m_worldBoundingBox;
	glm::quat m_rotation;
	glm::vec3 m_position;
	glm::vec3 m_scale;
};

}

#include "engine/entity/entity.inl"

#endif