#include "shockgamepch.h"
#include "shock_component_ai_round_system.h"
#include "engine/physics/rigidbodycomponent.h"

namespace solunar
{
	static tinyxml2::XMLDocument s_zombieDocument;

	void InitPrefabDOC()
	{
		static bool s_Initialized = false;
		if (!s_Initialized)
		{
			const char* prefabString = "<Entity><Position x=\"0\" y=\"0\" z=\"0\"/><Rotation x=\"0\" y=\"0\" z=\"0\"/><Scale x=\"1\" y=\"1\" z=\"1\"/><AnimatedMeshComponent><Model filename=\"models/zombie.glb\"/><CastShadow value=\"true\"/></AnimatedMeshComponent><RigidBodyComponent><Static value=\"false\"/><Mass value=\"80\"/><Filter value=\"PhysicsFilter_Bolvan\"/></RigidBodyComponent><BoxShapeComponent><Position x=\"0.0\" y=\"0.8\" z=\"0.0\"/><Size x=\"0.2\" y=\"0.8\" z=\"0.2\"/></BoxShapeComponent> <ShockAIComponent><AIType value=\"ShockAIType_Zombie\"/></ShockAIComponent><ShockAIBehaviourTree><BehaviourTreeType value=\"Zombie\"/></ShockAIBehaviourTree></Entity>";
			tinyxml2::XMLError error = s_zombieDocument.Parse(prefabString);
			if (error != tinyxml2::XMLError::XML_SUCCESS)
			{
				Core::Error("ZOMBIE PREFAB ROUTINE IS SADLY FUCKING BROKEN I FUCK THIS STUPID PIECE OF SHIT WHAT I CALLED \"ENGINE\"");
			}

			s_Initialized = true;
		}
	}

	IMPLEMENT_OBJECT(ShockAIRoundSystem, LogicComponent);

	ShockAIRoundSystem* g_ShockAIRoundSystem = nullptr;

	ShockAIRoundSystem::ShockAIRoundSystem() : 
		m_current_round(0), 
		m_max_rounds(4), 
		m_zombies_to_kill(12),
		m_numZombieSpawnedInCurrentRound(0),
		m_timer(0.0f)
	{
	}

	ShockAIRoundSystem::~ShockAIRoundSystem()
	{
		g_ShockAIRoundSystem = nullptr;
	}

	void ShockAIRoundSystem::OnInit()
	{
		// find spawn points
		m_spawnPoints[0] = GetWorld()->GetEntityManager().GetEntityByName("zombie_spawn_1");
		m_spawnPoints[1] = GetWorld()->GetEntityManager().GetEntityByName("zombie_spawn_2");
		m_spawnPoints[2] = GetWorld()->GetEntityManager().GetEntityByName("zombie_spawn_3");

		if (!m_spawnPoints[0] || !m_spawnPoints[1] || !m_spawnPoints[2])
		{
			Core::Msg("ERROR: THIS IS SHIT DONT HAVE ANY SPAWN POINT FOR ZOMBIE :(((((((");
			return;
		}
	}

	void ShockAIRoundSystem::Update(float dt)
	{
		m_timer += dt;

		const float kZombieSpawnDelay = 12.0f;

		if (m_timer > kZombieSpawnDelay)
		{
			SpawnZombie();
			m_timer = 0.0f;
		}
	}

	void ShockAIRoundSystem::LoadXML(tinyxml2::XMLElement& element)
	{
	}

	void ShockAIRoundSystem::SaveXML(tinyxml2::XMLElement& element)
	{
	}
	bool ShockAIRoundSystem::IsPlayerKilledEnoughZombies()
	{
		return false;
	}
	void ShockAIRoundSystem::NextRound()
	{
	}
	void ShockAIRoundSystem::FinishRound()
	{
	}
	void ShockAIRoundSystem::CompleteGame()
	{
	}
	void ShockAIRoundSystem::CompleteLevel()
	{
	}

	void ShockAIRoundSystem::SpawnZombie()
	{
#if 1
		InitPrefabDOC();

		Entity* entity = GetWorld()->CreateEntity();
		tinyxml2::XMLElement* pRE = s_zombieDocument.RootElement();
		entity->LoadXML(*pRE);
		
		GetWorld()->PostInitializeEntity(entity);

		// select spawner

		static int s_SpawnPointId = 0;
		if (s_SpawnPointId >= kZombieSpawnerCount)
			s_SpawnPointId = 0;

		Entity* spawner = m_spawnPoints[s_SpawnPointId++];
		entity->SetPosition(spawner->GetWorldPosition());

		entity->GetComponent<RigidBodyComponent>()->UpdateBodyTranslationDirty();
#endif

#if 0
		// select spawner
		int spawnerId = rand() / RAND_MAX * kZombieSpawnerCount;
		Entity* spawner = m_spawnPoints[spawnerId];
		entity->SetRotation(spawner->GetWorldPosition());

		Entity* entity = GetWorld()->CreateEntity();
#endif


		m_numZombieSpawnedInCurrentRound++;
	}
}