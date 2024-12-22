#ifndef ENGINE_ENTITY_WORLD_H
#define ENGINE_ENTITY_WORLD_H

#include "engine/entity/entitymanager.h"
#include "graphics/graphicsworld.h"

namespace solunar
{
	class IEditorWindow;
}

namespace solunar
{
	class PhysicsWorld;

	struct RayCastResult
	{
		Entity* m_entity;
		glm::vec3 m_hitPosition;
		glm::vec3 m_hitNormal;
	};

	class World : public SerializableObject
	{
	public:
		DECLARE_OBJECT(World);

	public:
		World();
		~World();

		void LoadXML(tinyxml2::XMLElement& element) override;
		void SaveXML(tinyxml2::XMLElement& element) override;

		void PostInitializeEntity(Entity* entity);

		void Update_PreEntityUpdate();
		void Update_LogicEntity();
		void Update_PhysicsEntity();

		Entity* CreateEntity();
		Entity* CreateEntityEx(const TypeInfo* typeInfo);

		EntityManager& GetEntityManager() { return m_entityManager; }
		GraphicsWorld* GetGraphicsWorld() { return m_graphicsWorld; }
		PhysicsWorld* GetPhysicsWorld() { return m_physicsWorld; }

		void TogglePhysicsDebugDraw();

		void Initialize();
		bool IsWorldInitialized() { return m_isWorldInitialized; }

		// Casting functions

		bool RayCast(RayCastResult& rayResult, const glm::vec3& rayStart, const glm::vec3& rayEnd, const int collisionFilter = -1);
		std::vector<Entity*> BoxCast(const glm::vec3& boxPos, const glm::vec3& boxSize);

	private:
		void LoadAI(tinyxml2::XMLElement& tagWorld);

	private:
		EntityManager m_entityManager;

		GraphicsWorld* m_graphicsWorld;
		PhysicsWorld* m_physicsWorld;
		bool m_isWorldInitialized;
	};

}

#endif // !ENGINECORE_ENTITY_LEVEL_H
