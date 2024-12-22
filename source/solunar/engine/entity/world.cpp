#include "enginepch.h"
#include "core/timer.h"
#include "core/object/typemanager.h"
#include "engine/entity/entity.h"
#include "engine/entity/entitymanager.h"
#include "engine/entity/world.h"
#include "engine/entity/logiccomponent.h"
#include "engine/physics/physicsworld.h"
#include "engine/physics/rigidbodycomponent.h"
#include "engine/physics/shapescomponent.h"
#include "editor\editor_window.h"
#include "engine\engine.h"
#include "editor/editor_manager.h"
#include "ai/pathfinding_navigation_graph.h"
#include "ai/pathfinding_manager.h"
#include "ai/pathfinding_solver_astar.h"
#include "ai/pathfinding_solver_dijkstra.h"
#include <imgui.h>

namespace solunar
{
	IMPLEMENT_OBJECT(World, SerializableObject);

	World::World() :
		m_physicsWorld(nullptr), m_graphicsWorld(nullptr), m_isWorldInitialized(false)
	{
		m_physicsWorld = mem_new<PhysicsWorld>();
		m_graphicsWorld = mem_new<GraphicsWorld>();
	}

	World::~World()
	{
		// Delete all entities before physics world destruction
		m_entityManager.Destroy();

		if (m_physicsWorld)
		{
			mem_delete(m_physicsWorld); m_physicsWorld = nullptr;
		}

		if (m_graphicsWorld)
		{
			mem_delete(m_graphicsWorld); m_graphicsWorld = nullptr;
		}
	}

	void World::LoadXML(tinyxml2::XMLElement& element)
	{
		if (strcmp(element.Name(), "World") != 0)
			Core::Error("World::LoadXML: '%s' is not a world element.", element.Name());

		tinyxml2::XMLElement* entityElement = element.FirstChildElement("Entity");
		while (entityElement)
		{
			const TypeInfo* entityTypeInfo = Entity::GetStaticTypeInfo();

			// if classid
			if (const tinyxml2::XMLAttribute* nodeClassId = entityElement->FindAttribute("classId"))
			{
				entityTypeInfo = g_typeManager->GetTypeInfoById(nodeClassId->IntValue());
			}
			// if classname
			else if (const tinyxml2::XMLAttribute* nodeClassName = entityElement->FindAttribute("className"))
			{
				entityTypeInfo = g_typeManager->GetTypeInfoByName(nodeClassName->Value());
			}

			Entity* entity = CreateEntityEx(entityTypeInfo);
			entity->LoadXML(*entityElement);

			PostInitializeEntity(entity);

			entityElement = entityElement->NextSiblingElement("Entity");
		}


		this->LoadAI(element);
	}

	void World::SaveXML(tinyxml2::XMLElement& element)
	{
		const std::vector<Entity*>& entities = m_entityManager.GetEntities();
		for (auto it : entities)
		{
			if (it->GetRootEntity())
				continue;

			tinyxml2::XMLElement* entityElement = element.InsertNewChildElement("Entity");
			entityElement->SetAttribute("className", it->GetTypeInfo()->GetClassName());
			it->SaveXML(*entityElement);
		}
	}

	void World::PostInitializeEntity(Entity* entity)
	{
		// Initialize physics shapes
		std::vector<ShapeComponent*> shapes = entity->GetComponents<ShapeComponent>();
		if (!shapes.empty())
		{
			for (auto it : shapes)
				it->InitializeShape();
		}

		std::vector<RigidBodyComponent*> bodies = entity->GetComponents<RigidBodyComponent>();
		if (!bodies.empty())
		{
			for (auto it : bodies)
				it->UpdateBodyTranslationDirty();
		}
	}

	void World::Update_PreEntityUpdate()
	{
		m_entityManager.Update();

		const std::vector<Entity*>& entities = m_entityManager.GetEntities();
		for (auto it : entities)
		{
			it->UpdateWorldTransform();
			it->TransformBBox();
		}
	}

	void World::Update_LogicEntity()
	{
		Timer* timerInstance = Timer::GetInstance();

		const std::vector<Entity*>& entities = m_entityManager.GetEntities();
		for (auto entity : entities)
		{
			LogicComponent* logicComponent = entity->GetComponent<LogicComponent>();
			if (logicComponent)
				logicComponent->Update(timerInstance->GetDelta());
		}

#ifdef _DEBUG
		if (g_aiPathfindingManager)
		{
			g_aiPathfindingManager->DebugDraw();
		}
#endif
	}

	void World::Update_PhysicsEntity()
	{
		float delta = Timer::GetInstance()->GetDelta();
		m_physicsWorld->Step(delta);

		const std::vector<RigidBodyComponent*>& physicsEntities = m_physicsWorld->GetRigidBodies();
		for (auto it : physicsEntities)
		{
			it->UpdateEntityTranslationDirty();
			it->UpdateBodyTranslationDirty();
		}
	}

	Entity* World::CreateEntity()
	{
		Entity* entity = m_entityManager.CreateEntity();
		entity->OnWorldSet(this);
		return entity;
	}

	Entity* World::CreateEntityEx(const TypeInfo* typeInfo)
	{
		Entity* entity = m_entityManager.CreateEntityEx(typeInfo);
		entity->OnWorldSet(this);
		return entity;
	}

	void World::TogglePhysicsDebugDraw()
	{
		m_physicsWorld->ToggleDebugDraw();
		g_aiPathfindingManager->ToggleDebugDraw();
	}

	void World::Initialize()
	{
		if (m_isWorldInitialized)
			Core::Error("World::Initialize: Calling function on already initialized world is unacceptable");

		const std::vector<Entity*>& entities = m_entityManager.GetEntities();
		for (auto entity : entities)
		{
			const std::vector<Component*>& components = entity->GetAllComponents();
			for (auto component : components)
				component->OnInit();
		}

		m_isWorldInitialized = true;

		m_entityManager.OnWorldLoaded();
	}

	bool World::RayCast(RayCastResult& rayResult, const glm::vec3& rayStart, const glm::vec3& rayEnd, const int collisionFilter /* =-1*/)
	{
		Assert2(m_physicsWorld, "Physics world is not initialized for ray casting");

		btDynamicsWorld::ClosestRayResultCallback rayCallback(glmVectorToBt(rayStart), glmVectorToBt(rayEnd));
		
		if (collisionFilter == -1)
			rayCallback.m_collisionFilterMask = kCollisionFilterAllMask;
		else
			rayCallback.m_collisionFilterMask = collisionFilter;

		if (g_engineData.m_editor && !g_editorManager->GetInstance()->IsSimulate())
			m_physicsWorld->GetWorld()->updateAabbs();

		m_physicsWorld->GetWorld()->rayTest(glmVectorToBt(rayStart), glmVectorToBt(rayEnd), rayCallback);
		if (rayCallback.hasHit())
		{
			// Query entity
			
			RigidBodyComponent* rigidbody = reinterpret_cast<RigidBodyComponent*>(rayCallback.m_collisionObject->getUserPointer());
			Assert2(rigidbody, "Critital skill issue, rigid body component is nullptr!");

			Entity* entity = rigidbody->GetEntity();
			Assert2(entity, "Critital skill issue, rigid body component assigned to nullptr entity!");
			
			rayResult.m_entity = entity;
			rayResult.m_hitPosition = btVectorToGlm(rayCallback.m_hitPointWorld);
			rayResult.m_hitNormal = btVectorToGlm(rayCallback.m_hitNormalWorld);
			return true;
		}

		return false;
	}

	std::vector<Entity*> World::BoxCast(const glm::vec3& boxPos, const glm::vec3& boxSize)
	{
		std::vector<Entity*> entities;
		
		glm::vec3 halfSize = boxSize / glm::vec3(2.0f);
		
		btBoxShape convexShape(glmVectorToBt(halfSize));

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(glmVectorToBt(boxPos));

		btDynamicsWorld::ClosestConvexResultCallback convexCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f));
		//m_physicsWorld->GetWorld()->convexSweepTest(&convexShape, transform,  )

		
		return entities;
	}

	void World::LoadAI(tinyxml2::XMLElement& tagWorld)
	{
		auto* tag_ai = tagWorld.FirstChildElement(_kSerializationTag_AI);

		if (tag_ai)
		{
			auto* tag_ai_backend = tag_ai->FirstChildElement(_kSerializationTag_AIBackend);

			if (tag_ai_backend)
			{
				auto* attr_navigation_type = tag_ai_backend->FindAttribute(_kSerializationAttribute_NavigationType);

				if (attr_navigation_type)
				{
					if (!strcmp(attr_navigation_type->Value(), _kManualGraphName))
					{
						auto* attr_ai_data_storage = tag_ai_backend->FindAttribute(_kSerializationAttribute_AIDataStorageTagName);

						if (attr_ai_data_storage)
						{
							const char* pTagAIDataStorageName = attr_ai_data_storage->Value();

							auto* tag_ai_data_storage = tag_ai->FirstChildElement(pTagAIDataStorageName);

							if (tag_ai_data_storage)
							{
								auto* attr_ai_solver = tag_ai_backend->FindAttribute(_kPathfindingSerializationAttribute_AISolver);

								if (attr_ai_solver)
								{
									if (!strcmp(attr_ai_solver->Value(), _kPathfindingSolver_Astar))
									{
										g_aiPathfindingManager->Shutdown();

										PathfindingNavigationStaticGraph* pNavigationData = mem_new<PathfindingNavigationStaticGraph>();
										PathfindingSolverDijkstra* pSolver = mem_new<PathfindingSolverDijkstra>();

										pNavigationData->Init(*tag_ai_data_storage);
										pSolver->Init(pNavigationData);

										g_aiPathfindingManager->Init(pNavigationData, pSolver);

#ifdef _DEBUG
										Core::Msg("AI: loaded successfully!");
#endif
									}
									else
									{
										Assert(!"not implemented");
										Core::Error("Unknown ai solver (unsupported by engine)");
									}
								}
								else
								{
									Core::Warning("FAILED to obtain ai solver!");
								}
							}
							else
							{
								Core::Warning("FAILED to obtain xml-tag, the info is presented but the tag is not!");
							}
						}
						else
						{
							Core::Warning("FAILED to obtain tag where ai data is stored!");
						}
					}
					else if (!strcmp(attr_navigation_type->Value(), _kAutoGridName))
					{
						Assert(!"not implemented!");
						Core::Error("not implemented yet");
					}
					else if (!strcmp(attr_navigation_type->Value(), _kNavMeshName))
					{
						Assert(!"not implemented!");
						Core::Error("not implemented yet");
					}
					else
					{
						Assert(!"unknown navigation type, register it");
						Core::Error("Unknown navigation type (unsupported by engine)");
					}
				}
				else
				{
					Core::Warning("FAILED to obtain navigation type!");
				}
			}
		}
	}

}