#include "shockgamepch.h"

// Base game includes
#include "game/gamelogic/camera/cameramanager.h"
#include "game/gamelogic/mainmenu/mainmenuworldcomponent.h"
#include "game/gamelogic/player/playercontrollercomponent.h"
#include "game/gamelogic/player/weaponchaincomponent.h"
#include "game/gamelogic/saveload/saveloadmanager.h"
#include "game/gamelogic/weapons/weaponcomponent.h"
#include "game/gamelogic/weapons/weaponautocomponent.h"
#include "game/gamelogic/weapons/weaponpistolcomponent.h"
#include "game/gamelogic/world/levelmanager.h"

// ShockGame
#include "shockgame/shockgame.h"
#include "shockgame/shockeventlistener.h"
#include "shockgame/shocksignalmanager.h"
#include "shockgame/shockplayercontroller.h"
#include "shockgame/shockgameui.h"
#include "shockgame/shockaicomponent.h"
#include "shockgame/shockprojectilecomponent.h"
#include "shockgame/demogame.h"

// ShockGame AI
#include "shockgame/shock_component_ai_behaviourtree.h"
#include "shockgame/shock_component_ai_round_system.h"

// Engine
#include "engine/camera.h"
#include "engine/engine.h"
#include "engine/console.h"

// Graphics
#include "graphics/fontmanager.h"
#include "graphics/imguimanager.h"
#include "graphics/debugrenderer.h"
#include "graphics/fontmanager.h"

#undef min
#undef max

#include <numeric>

// Graphics Objects
#include "graphics/mesh.h"
#include "graphics/animatedmodel.h"

#include <array>
#include <algorithm>

#include "core/object/propertymanager.h"

namespace solunar
{

// implemetation for IGameInterface

static ShockGameInterface s_shockGameInterface;
IGameInterface* g_gameInterface = (IGameInterface*)&s_shockGameInterface;

ShockGameInterface* getShockGameInterface()
{
	return &s_shockGameInterface;
}

class TestRotatorComponent : public LogicComponent
{
	DECLARE_OBJECT(TestRotatorComponent);

public:
	TestRotatorComponent() : m_YAxis(0.0f)
	{}

	~TestRotatorComponent()
	{}

	static void RegisterObject()
	{
		g_typeManager->RegisterObject<TestRotatorComponent>();
	}

	void Update(float dt) override
	{
		m_YAxis += dt * 200.0f;
		GetEntity()->QuaternionRotate(glm::vec3(1.f, 1.f, 0.f), m_YAxis);

		glm::quat o = GetEntity()->GetRotation();
		glm::vec3 V;
		V[0] = 2 * (o.x * o.z - o.w * o.y);
		V[1] = 2 * (o.y * o.z + o.w * o.x);
		V[2] = 1 - 2 * (o.x * o.x + o.y * o.y);

		g_debugRender.DrawLine(GetEntity()->GetPosition(),
			GetEntity()->GetPosition() + V, glm::vec3(1.0f, 1.0f, 0.0f));

		g_debugRender.drawBoundingBox(GetEntity()->GetBoundingBox(), glm::vec3(1.0f));
	}

private:
	float m_YAxis;
};

IMPLEMENT_OBJECT(TestRotatorComponent, LogicComponent);

class TestPositionUpdaterComponent : public LogicComponent
{
	DECLARE_OBJECT(TestPositionUpdaterComponent);

public:
	TestPositionUpdaterComponent()
	{}

	~TestPositionUpdaterComponent()
	{}

	static void RegisterObject()
	{
		g_typeManager->RegisterObject<TestPositionUpdaterComponent>();
	}

	void Update(float dt) override
	{
		const float kSpeed = 2.0f;

		glm::vec3 position = GetEntity()->GetPosition();
		position.y += kSpeed * dt;
		GetEntity()->SetPosition(position);
	}

};

IMPLEMENT_OBJECT(TestPositionUpdaterComponent, LogicComponent);

class TestAnimationComponent : public LogicComponent
{
	DECLARE_OBJECT(TestAnimationComponent);
public:
	TestAnimationComponent();
	~TestAnimationComponent();

	void LoadXML(tinyxml2::XMLElement& element) override;

	void Update(float dt) override;

private:
	std::string m_animationName;
	int m_animationIndex = -1;
};

IMPLEMENT_OBJECT(TestAnimationComponent, LogicComponent);

TestAnimationComponent::TestAnimationComponent()
{
}

TestAnimationComponent::~TestAnimationComponent()
{
}

void TestAnimationComponent::LoadXML(tinyxml2::XMLElement& element)
{
	tinyxml2::XMLElement* animationNameElement = element.FirstChildElement("AnimationName");
	if (animationNameElement)
	{
		const tinyxml2::XMLAttribute* animationNameAttribute = animationNameElement->FindAttribute("name");
		if (animationNameAttribute)
		{
			m_animationName = animationNameAttribute->Value();
		}
	}
}

void TestAnimationComponent::Update(float dt)
{
	AnimatedMeshComponent* animatedMeshComponent = GetEntity()->GetComponent<AnimatedMeshComponent>();
	std::shared_ptr<ModelBase> modelBase = animatedMeshComponent->LockModel();
	AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(modelBase.get());
	if (animatedModel) {
		if (m_animationIndex == -1) {
			m_animationIndex = animatedModel->GetAnimationByName(m_animationName);
			animatedModel->PlayAnimation(m_animationIndex, true);
		}
	}

	animatedModel->Update(dt);
}

class ViewmodelAnimationController : public LogicComponent
{
	DECLARE_OBJECT(ViewmodelAnimationController);
public:
	ViewmodelAnimationController();
	~ViewmodelAnimationController();

	void Update(float dt) override;

private:
	int m_animationIndex = -1;
};

IMPLEMENT_OBJECT(ViewmodelAnimationController, LogicComponent);

ViewmodelAnimationController::ViewmodelAnimationController()
{
}

ViewmodelAnimationController::~ViewmodelAnimationController()
{
}

void ViewmodelAnimationController::Update(float dt)
{
}

// Command for toggle weapon trace debug
void Command_DebugTrace()
{
	g_debugTrace = !g_debugTrace;
	Core::Msg("Game: Trace debugging is %s", g_debugTrace ? "true" : "false");
}

void Command_MakeMeRich()
{
	if (!g_Player)
		return;

	g_Player->GetComponent<ShockPlayerController>()->AddMoney(9999999999);
}

void Command_DebugAI()
{
	g_debugAI = !g_debugAI;
}

// More beautiful way to register classes
void registerGameClasses()
{
	const TypeInfo* gameClasses[] = 
	{
		OBJECT_GET_TYPEINFO(MainMenuWorldComponent),
		OBJECT_GET_TYPEINFO(PlayerControllerComponent),
		OBJECT_GET_TYPEINFO(WeaponComponent),
		OBJECT_GET_TYPEINFO(WeaponAutoComponent),
		OBJECT_GET_TYPEINFO(WeaponPistolComponent),
		OBJECT_GET_TYPEINFO(WeaponChainComponent),
		OBJECT_GET_TYPEINFO(LevelManagerComponent),
		OBJECT_GET_TYPEINFO(TestRotatorComponent),
		OBJECT_GET_TYPEINFO(TestPositionUpdaterComponent),
		OBJECT_GET_TYPEINFO(TestAnimationComponent),
		OBJECT_GET_TYPEINFO(ViewmodelAnimationController),
		OBJECT_GET_TYPEINFO(UsableAreaComponent),
		OBJECT_GET_TYPEINFO(EditorCameraComponent),
		OBJECT_GET_TYPEINFO(FreeCameraComponent),
		OBJECT_GET_TYPEINFO(PlayerSpawnComponent),
		OBJECT_GET_TYPEINFO(DoorCoverComponent),
		OBJECT_GET_TYPEINFO(ShelterLevelManagerComponent),
	};

	for (int i = 0; i < sizeof(gameClasses) / sizeof(gameClasses[0]); i++)
		TypeManager::GetInstance()->RegisterType(gameClasses[i]);
}

void registerShockClasses()
{
	const TypeInfo* shockClasses[] =
	{
		OBJECT_GET_TYPEINFO(ShockSignal),
		OBJECT_GET_TYPEINFO(ShockPlayerController),
		OBJECT_GET_TYPEINFO(ShockAIComponent),
		OBJECT_GET_TYPEINFO(ShockProjectileComponent),
		OBJECT_GET_TYPEINFO(ShockGameMainMenuComponent),

		// AI
		OBJECT_GET_TYPEINFO(ShockAIBehaviourTree),
		OBJECT_GET_TYPEINFO(ShockAIRoundSystem),
	};

	for (int i = 0; i < sizeof(shockClasses) / sizeof(shockClasses[0]); i++)
		TypeManager::GetInstance()->RegisterType(shockClasses[i]);
}

void registerGameCommands()
{
	ConsoleCommandManager* cmdmgr = ConsoleCommandManager::GetInstance();
	cmdmgr->RegisterCommand("debugtrace", &Command_DebugTrace);
	cmdmgr->RegisterCommand("debugai", &Command_DebugAI);
	cmdmgr->RegisterCommand("imrich", &Command_MakeMeRich);
}

#define DEMO_GAME

ShockGameInterface::ShockGameInterface()
{
}

ShockGameInterface::~ShockGameInterface()
{
}

void ShockGameInterface::Initialize()
{
	Core::Msg("Initializing game");

	// register game commands
	registerGameCommands();
	
	// register game objects
	registerGameClasses();

	// register shock objects
	registerShockClasses();

#ifdef DEMO_GAME
	TypeManager::GetInstance()->RegisterType(DemoGameMainMenuComponent::GetStaticTypeInfo());
#endif // DEMO_GAME

	// add event listener
//	g_eventManager.addEventListener(&g_shockEventListener);

	// Create signal manager
	ShockSignalManager::CreateInstance();
}

void ShockGameInterface::Shutdown()
{
	// Destroy signal manager
	ShockSignalManager::DestroyInstance();

	// g_eventManager.removeEventListener(&g_shockEventListener);
}

}