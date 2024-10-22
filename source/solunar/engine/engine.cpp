#include "enginepch.h"
#include "core/timer.h"
#include "core/file/contentmanager.h"
#include "core/timer.h"

#include "engine/engine.h"
#include "engine/inputmanager.h"
#include "engine/entity/entity.h"
#include "engine/entity/component.h"
#include "engine/entity/world.h"
#include "engine/entity/cameracomponent.h"
#include "engine/entity/logiccomponent.h"

#include "engine/physics/shapescomponent.h"
#include "engine/physics/rigidbodycomponent.h"
#include "engine/physics/trianglemeshshape.h"
#include "engine/physics/triggercomponent.h"

// #HACK: move to game interface
#include "shockgame/demogame.h"

namespace solunar
{
	EngineData		g_engineData;
	World*			Engine::ms_world = nullptr;
	std::string		g_worldName;

	// There is more nice looking object registration
	void registerEngineObjects()
	{
		const TypeInfo* engineClasses[] = 
		{
			// base types
			Entity::GetStaticTypeInfo(),
			Component::GetStaticTypeInfo(),
			World::GetStaticTypeInfo(),

			// camera
			CameraComponent::GetStaticTypeInfo(),
			CameraYawPitchRollComponent::GetStaticTypeInfo(),
			CameraFirstPersonComponent::GetStaticTypeInfo(),

			// logic
			LogicComponent::GetStaticTypeInfo(),

			// physics
			ShapeComponent::GetStaticTypeInfo(),
			BoxShapeComponent::GetStaticTypeInfo(),
			SphereShapeComponent::GetStaticTypeInfo(),
			CylinderShapeComponent::GetStaticTypeInfo(),
			CapsuleShapeComponent::GetStaticTypeInfo(),
			TriangleMeshShapeComponent::GetStaticTypeInfo(),
			RigidBodyComponent::GetStaticTypeInfo(),
			RigidBodyProxyComponent::GetStaticTypeInfo(),
			TriggerComponent::GetStaticTypeInfo()
		};

		// register types
		for (int i = 0; i < sizeof(engineClasses) / sizeof(engineClasses[0]); i++)
			TypeManager::GetInstance()->RegisterType(engineClasses[i]);
	}

	void Engine::Init()
	{
		Core::Msg("Initializing engine");

		registerEngineObjects();
	}

	void Engine::Shutdown()
	{
		if (ms_world)
		{
			Core::Msg("Engine: world is present on engine shutdown, deleting ...");

			mem_delete(ms_world);
			ms_world = nullptr;
		}
	}

	void Engine::LoadWorld(const std::string& filename)
	{
		g_worldName = filename;

		if (ms_world)
		{
			mem_delete(ms_world);
			ms_world = nullptr;
		}

		Core::Msg("Engine: Loading world %s", filename.c_str());

		std::shared_ptr<DataStream> stream = g_contentManager->OpenStream(filename);
		if (!stream)
		{
			Core::Msg("Engine::loadWorld: Failed to load world %s. File not exist!", filename.c_str());
			return;
		}

		stream->Seek(Seek_End, 0);
		size_t length = stream->Tell();
		stream->Seek(Seek_Begin, 0);

		char* data = new char[length + 1];
		stream->Read((void*)data, length);
		data[length] = '\0';

		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError error = doc.Parse(data, length);
		
		if (error != tinyxml2::XML_SUCCESS)
		{
			Core::Error("Engine::loadWorld: Failed to parse world %s. %s", filename.c_str(), doc.ErrorStr());
		}
		
		tinyxml2::XMLElement* worldElement = doc.FirstChildElement("World");;

		World* world = g_typeManager->CreateObject<World>();
		world->LoadXML(*worldElement);
		
		ms_world = world;
		
		delete[] data;

		g_GameManager->OnWorldLoad(filename);

		// #TODO: RESET TIMER AND RUN ONE FRAME INSTEAD
		Timer::GetInstance()->Update();
		Timer::GetInstance()->Update();
	}

	void Engine::LoadEmptyWorld()
	{
		if (ms_world)
		{
			mem_delete(ms_world);
			ms_world = nullptr;
		}

		Core::Msg("Engine: Creating empty world");

		World* world = g_typeManager->CreateObject<World>();
		ms_world = world;

		g_GameManager->OnWorldLoad("");

		// #TODO: RESET TIMER AND RUN ONE FRAME INSTEAD
		Timer::GetInstance()->Update();
		Timer::GetInstance()->Update();
	}

	void Engine::Update()
	{
		//OPTICK_EVENT("Engine::update");

		if (InputManager::GetInstance()->IsPressedWithReset(KEY_F8))
			EngineStateManager::GetInstance()->LoadWorld(g_worldName);

		EngineStateManager::GetInstance()->Update();

		//g_eventManager.Update();

		World* world = Engine::ms_world;
		if (world)
		{
			world->Update_PreEntityUpdate();
			world->Update_PhysicsEntity();
			world->Update_LogicEntity();
		}
	}

	void Engine::SetGameInterface(IGameInterface* gameInterface)
	{
	}

	IGameInterface* Engine::GetGameInterface()
	{
		return nullptr;
	}

	static EngineStateManager s_engineStateManager;

	EngineStateManager::EngineStateManager()
	{
		m_currentState = EngineState::None;
		m_nextState = EngineState::None;
	}

	EngineStateManager::~EngineStateManager()
	{
	}

	void EngineStateManager::LoadWorld(const std::string& filename)
	{
		m_nextState = EngineState::LoadWorld;
		m_worldName = filename;
	}

	void EngineStateManager::LoadEmptyWorld()
	{
		m_nextState = EngineState::LoadWorld;
		m_worldName.clear();
	}

	void EngineStateManager::Update()
	{
		if (m_currentState == m_nextState)
			return;

		m_currentState = m_nextState;
		OnStateSwitch();
	}

	void EngineStateManager::OnStateSwitch()
	{
		static const char* s_stateNames[(int)EngineState::Count] =
		{
			"None",
			"Running",
			"LoadWorld"
		};

		Core::Msg("Engine: Switching to state %s", s_stateNames[(int)m_nextState]);

		switch (m_currentState)
		{
		case EngineState::Running:
			break;
		case EngineState::LoadWorld:
			if (m_worldName.empty())
				Engine::LoadEmptyWorld();
			else
				Engine::LoadWorld(m_worldName);

			m_nextState = EngineState::Running;
			m_worldName.clear();

			break;

		default:
			break;
		}
	}
}
