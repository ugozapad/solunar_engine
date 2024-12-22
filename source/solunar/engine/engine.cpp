#include "enginepch.h"
#include "core/timer.h"
#include "core/file/contentmanager.h"
#include "core/timer.h"

#include "engine/engine.h"
#include "engine/inputmanager.h"
#include "engine/console.h"
#include "engine/entity/entity.h"
#include "engine/entity/component.h"
#include "engine/entity/world.h"
#include "engine/entity/cameracomponent.h"
#include "engine/entity/logiccomponent.h"

#include "engine/physics/shapescomponent.h"
#include "engine/physics/rigidbodycomponent.h"
#include "engine/physics/trianglemeshshape.h"
#include "engine/physics/triggercomponent.h"

#include "graphics/renderer.h"

// #HACK: move to game interface
#include "shockgame/demogame.h"

#include "engine/editor/editor_manager.h"

#include "ai/pathfinding_manager.h"

namespace solunar
{
	EngineData		g_engineData;
	World* Engine::ms_world = nullptr;
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

		g_console->Init();

		if (g_engineData.m_editor)
			g_editorManager = mem_new<EditorManager>();

		g_aiPathfindingManager = mem_new<PathfindingManager>();
	}

	void Engine::Shutdown()
	{
		if (ms_world)
		{
			Core::Msg("Engine: world is present on engine shutdown, deleting ...");

			mem_delete(ms_world);
			ms_world = nullptr;
		}

		if (g_editorManager)
		{
			g_editorManager->Shutdown();

			mem_delete(g_editorManager);
			g_editorManager = nullptr;
		}

		if (g_aiPathfindingManager)
		{
			g_aiPathfindingManager->Shutdown();

			mem_delete(g_aiPathfindingManager);
			g_aiPathfindingManager = nullptr;
		}
		
		g_console->Shutdown();
	}

	void Engine::LoadWorld(const std::string& filename)
	{
		g_worldName = filename;

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

		if (g_engineData.m_editor)
		{
			g_editorManager->SetWorldXML(data, length);
		}

		ms_world = world;

		delete[] data;

		// #TODO: RESET TIMER AND RUN ONE FRAME INSTEAD
		Timer::GetInstance()->Update();
		Timer::GetInstance()->Update();
	}

	void Engine::LoadEmptyWorld()
	{
		Core::Msg("Engine: Creating empty world");

		World* world = g_typeManager->CreateObject<World>();
		ms_world = world;

		g_GameManager->OnWorldLoad("", world);

		// #TODO: RESET TIMER AND RUN ONE FRAME INSTEAD
		Timer::GetInstance()->Update();
		Timer::GetInstance()->Update();
	}

	void Engine::InitEditor(World* pLoadedWorld)
	{
		Assert(g_editorManager && "early calling or something is broken!");

		if (g_editorManager)
		{
			g_editorManager->Shutdown();
			g_editorManager->Init(pLoadedWorld);
		}
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
			bool simulating = true;
			if (g_editorManager)
			{
				simulating = g_editorManager->IsSimulate();
			}

			if (!world->IsWorldInitialized())
				world->Initialize();

			world->Update_PreEntityUpdate();
			if (simulating)
			{
				world->Update_PhysicsEntity();
				world->Update_LogicEntity();
			}
		}

		if (g_engineData.m_editor)
		{
			if (g_editorManager)
			{
				g_editorManager->Update();
			}
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

	void EngineStateManager::OnCloseApplication()
	{
		if (g_engineData.m_editor)
		{
			if (g_editorManager)
			{
				m_nextState = EngineState::CloseApplication;
			}
		}
	}

	void EngineStateManager::RestoreState()
	{
		m_nextState = EngineState::Running;
	}

	const std::string& EngineStateManager::GetWorldName() const
	{
		return this->m_worldName;
	}

	void EngineStateManager::OnStateSwitch()
	{
		static const char* s_stateNames[(int)EngineState::Count] =
		{
			"None",
			"Running",
			"LoadWorld",
			"CloseApplication"
		};

		Core::Msg("Engine: Switching to state %s", s_stateNames[(int)m_nextState]);

		switch (m_currentState)
		{
		case EngineState::Running:
			break;
		case EngineState::LoadWorld:

			Assert(g_renderer);
			g_renderer->RenderLoadscreen();

			if (Engine::ms_world)
			{
				mem_delete(Engine::ms_world);
				Engine::ms_world = nullptr;
			}

			if (m_worldName.empty())
				Engine::LoadEmptyWorld();
			else
				Engine::LoadWorld(m_worldName);

			if (g_engineData.m_editor)
				Engine::InitEditor(Engine::ms_world);

			g_GameManager->OnWorldLoad(m_worldName, Engine::ms_world);

			if (g_engineData.m_editor)
			{
				g_editorManager->PostInit();
			}

			m_nextState = EngineState::Running;
		//	m_worldName.clear();

			break;
		case EngineState::CloseApplication:
		{
			if (g_engineData.m_editor)
			{
				if (g_editorManager)
				{
					g_editorManager->OnCloseApplication();
				}
			}
			break;
		}
		default:
			break;
		}
	}
}
