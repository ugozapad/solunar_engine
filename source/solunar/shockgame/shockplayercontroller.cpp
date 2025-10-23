#include "shockgamepch.h"
#include "shockgame/shockplayercontroller.h"

#include "core/file/contentmanager.h"
#include "core/timer.h"

#include "engine/inputmanager.h"
#include "engine/console.h"

#include "engine/ai/pathfinding_manager.h"

#include "graphics/imguimanager.h"
#include "graphics/ifontmanager.h"
#include "graphics/animatedmodel.h"
#include "graphics/debugrenderer.h"

#include "game/gamelogic/weapons/weaponcomponent.h"

#include "shockgame/demogame.h"

#include <numeric>
#include <limits>
#include <queue>

#include "stb_sprintf.h"

namespace solunar
{

IMPLEMENT_OBJECT(ShockPlayerController, PlayerControllerComponent);

BEGIN_PROPERTY_REGISTER(ShockPlayerController)
{
	REGISTER_PROPERTY(ShockPlayerController, PropertyFloat, m_playerStats.m_health);
	REGISTER_PROPERTY(ShockPlayerController, PropertyFloat, m_playerStats.m_endurance);
}
END_PROPERTY_REGISTER(ShockPlayerController);

// #TODO: REFACTOR
Entity* g_freeCameraEntity = nullptr;
CameraFirstPersonComponent* g_freeCamera = nullptr;

class ShockPlayerHUD : public Singleton<ShockPlayerHUD>
{
public:
	static ShockPlayerHUD ms_ShockPlayerHUD;
	static IFont* ms_HealthFont;
	static IFont* ms_AmmoFont;

public:
	void Draw();

	void AddHUDMessageQueue(const char* text);
	void AddInfoMessageQueue_Timed(const char* text, float time);

private:
	void DrawInfo();
	void DrawCrosshair();
	void DrawHUDMessages();

private:
	struct HUDInfoMessage
	{
		std::string m_text;
		float m_time;
		bool m_centred;
	};
	
	HUDInfoMessage m_hudmessage;

	std::queue<HUDInfoMessage> m_messagesQueue;
	float m_messagesQueueTime = 0.0f;
};

ShockPlayerHUD ShockPlayerHUD::ms_ShockPlayerHUD;
IFont* ShockPlayerHUD::ms_HealthFont = nullptr;
IFont* ShockPlayerHUD::ms_AmmoFont = nullptr;

void ShockPlayerHUD::Draw()
{
	if (!ms_HealthFont)
		ms_HealthFont = g_fontManager->CreateFont("textures/ui/RobotoMono-Bold.ttf", 32.0f);

	if (!ms_AmmoFont)
		ms_AmmoFont = g_fontManager->CreateFont("textures/ui/Anton-Regular.ttf", 50.0f);

	DrawInfo();
	DrawCrosshair();
	DrawHUDMessages();

	// debug stuff
#if 0
	int nodeId = g_aiPathfindingManager->GetNearestPoint(g_Player->GetWorldPosition());
	
	static char debugText[64];
	stbsp_snprintf(debugText, sizeof(debugText), "%i", nodeId);

	ms_HealthFont->DrawText(debugText, 500.0f, 500.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
#endif+
}

void ShockPlayerHUD::AddHUDMessageQueue(const char* text)
{
	//m_messagesQueue.push(text);
}

void ShockPlayerHUD::AddInfoMessageQueue_Timed(const char* text, float time)
{
	HUDInfoMessage info;
	info.m_text = text;
	info.m_time = time;
	info.m_centred = true;
	m_hudmessage = info;

	m_messagesQueueTime = 0.0f;
}

void ShockPlayerHUD::DrawInfo()
{
	if (!g_Player)
		return;

	ShockPlayerController* shockPlayerController = g_Player->GetComponent<ShockPlayerController>();
	if (!shockPlayerController)
		return;

	ShockPlayerStats playerStats = shockPlayerController->GetPlayerStats();

	View* view = CameraProxy::GetInstance()->GetView();

	static char healthText[64];
	stbsp_snprintf(healthText, sizeof(healthText), "Health: %.0f", playerStats.m_health);

	static char enduranceText[64];
	stbsp_snprintf(enduranceText, sizeof(enduranceText), "Endurance: %.0f", playerStats.m_endurance);

	static char moneyText[64];
	stbsp_snprintf(moneyText, sizeof(moneyText), "Money: %i", playerStats.m_money);

	ms_HealthFont->DrawText(healthText, 25.0f, view->m_height - 50.0f, glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
	ms_HealthFont->DrawText(moneyText, view->m_width - 256.0f, view->m_height - 25.0f, glm::vec4(0.0f, 0.5f, 0.0f, 1.0f));

	Entity* weaponEntity = shockPlayerController->GetActiveWeaponEntity();
	if (!weaponEntity)
		return;

	WeaponComponent* weaponComponent = weaponEntity->GetComponent<WeaponComponent>();
	if (!weaponComponent)
		return;

	static char s_Buffer[256];
	stbsp_snprintf(s_Buffer, sizeof(s_Buffer), "Ammo: %i/%i", weaponComponent->GetAmmo(), weaponComponent->GetClipSize());
	ms_AmmoFont->DrawText(s_Buffer, 25.0f, view->m_height - 20.0f, glm::vec4(1.0f, 0.2f, 0.0f, 1.0f));
}

void ShockPlayerHUD::DrawCrosshair()
{
	{
		View* view = CameraProxy::GetInstance()->GetView();

		ImVec2 pos = ImVec2((float)view->m_width / 2.0f, (float)view->m_height / 2.0f);

		//	ImGui::GetBackgroundDrawList()->AddCircle(pos, 12.0f, 0xff0000ff);

			//{
			//	ImVec2 p1 = pos;
			//	p1.x = p1.x - 16.0f;
			//	ImVec2 p2 = pos;
			//	p2.x = p2.x + 16.0f;
			//	ImGui::GetBackgroundDrawList()->AddLine(p1, p2, 0xffffffff);
			//}
			//{
			//	ImVec2 p1 = pos;
			//	p1.y = p1.y - 16.0f;
			//	ImVec2 p2 = pos;
			//	p2.y = p2.y + 16.0f;
			//	ImGui::GetBackgroundDrawList()->AddLine(p1, p2, 0xffffffff);
			//}

		const float kVoid = 8.0f;
		const float kLenght = 24.0f;

		// vertical line
		{
			ImVec2 p1 = pos;
			p1.x = p1.x - kLenght;
			ImVec2 p2 = pos;
			p2.x = p2.x - kVoid;
			ImVec2 p3 = pos;
			p3.x = p3.x + kVoid;
			ImVec2 p4 = pos;
			p4.x = p4.x + kLenght;

			// black
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(p1.x - 1.0f, p1.y), ImVec2(p2.x + 1.0f, p2.y), 0xff000000, 3.0f);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(p3.x - 1.0f, p3.y), ImVec2(p4.x + 1.0f, p4.y), 0xff000000, 3.0f);

			// white 
			ImGui::GetBackgroundDrawList()->AddLine(p1, p2, 0xffffffff);
			ImGui::GetBackgroundDrawList()->AddLine(p3, p4, 0xffffffff);
		}

		// horizontal
		{
			ImVec2 p1 = pos;
			p1.y = p1.y - kLenght;
			ImVec2 p2 = pos;
			p2.y = p2.y - kVoid;
			ImVec2 p3 = pos;
			p3.y = p3.y + kVoid;
			ImVec2 p4 = pos;
			p4.y = p4.y + kLenght;

			// black
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(p1.x, p1.y - 1.0f), ImVec2(p2.x, p2.y + 1.0f), 0xff000000, 3.0f);
			ImGui::GetBackgroundDrawList()->AddLine(ImVec2(p3.x, p3.y - 1.0f), ImVec2(p4.x, p4.y + 1.0f), 0xff000000, 3.0f);

			// white 
			ImGui::GetBackgroundDrawList()->AddLine(p1, p2, 0xffffffff);
			ImGui::GetBackgroundDrawList()->AddLine(p3, p4, 0xffffffff);
		}

		//ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(pos.x - 1.0f, pos.y - 1.0f), ImVec2(pos.x + 1.0f, pos.y + 1.0f), 0xff000000, 2.0f);
		ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 1.0f, pos.y + 1.0f), 0xffffffff);

		//ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(pos.x - 16.0f, pos.y - 16.0f), ImVec2(pos.x + 16.0f, pos.y + 16.0f), 0xff0000ff);
	}
}

void ShockPlayerHUD::DrawHUDMessages()
{
#if 0
	if (!m_messagesQueue.empty())
		m_messagesQueueTime += Timer::GetInstance()->GetDelta();

	View* view = CameraProxy::GetInstance()->GetView();

	float time = 4.0f;

	if (!m_messagesQueue.empty() && time < m_messagesQueueTime) {
		m_messagesQueue.pop();
		m_messagesQueueTime = 0.0f;
	}
	
	std::queue<HUDInfoMessage> messagesQueue = m_messagesQueue;

	for (; !messagesQueue.empty(); ) {
		auto it = messagesQueue.front();
		float alpha = 1.0f;// m_messagesQueueTime / it.m_time;
		ms_AmmoFont->DrawText(it.m_text.c_str(), view->m_width / 2.0f, view->m_height / 2.0f, glm::vec4(0.0f, 0.1f, 1.0f, alpha));
		messagesQueue.pop();
	}
#else
	View* view = CameraProxy::GetInstance()->GetView();
	m_messagesQueueTime += Timer::GetInstance()->GetDelta();
	if (m_hudmessage.m_time >= m_messagesQueueTime) {
		float alpha = m_hudmessage.m_time / (m_messagesQueueTime / 2.5f);
		ms_AmmoFont->DrawText(m_hudmessage.m_text.c_str(), view->m_width / 2.0f, view->m_height / 2.0f, glm::vec4(0.0f, 0.1f, 1.0f, alpha));
	}
#endif


//	ms_AmmoFont->DrawText(s_Buffer, 25.0f, view->m_height - 20.0f, glm::vec4(1.0f, 0.2f, 0.0f, 1.0f));

}

struct WeaponInfo
{
	WeaponsType type;
	int idleAni;
	int fireAni;
	int prior_to_reload_Ani;
	int reload_one_Ani;
	int reload_last_one_Ani;
};

static glm::vec3 g_weaponOffset = glm::vec3(0.2f, -0.25f, -0.3f);

Entity* CreateWeapon(Entity* cameraEntity, WeaponsType type)
{
	// create weapon
	Entity* hackEntity = cameraEntity->CreateChild();
	//Entity* hackEntity = getEntity()->createChild();
	hackEntity->QuaternionRotate(glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

	Entity* weaponEntity = hackEntity->CreateChild();
	weaponEntity->SetPosition(glm::vec3(0.2f, -0.25f, -0.3f));

	// load model
	AnimatedMeshComponent* weaponMesh = weaponEntity->CreateComponent<AnimatedMeshComponent>();

	if (type == WeaponsType::Pistol)
		weaponMesh->LoadModel("models/viewmodel_pistol.glb");
	if (type == WeaponsType::Shotgun)
		weaponMesh->LoadModel("models/viewmodel_shotgun.glb");

	WeaponComponent* weaponComponent = weaponEntity->CreateComponent<WeaponComponent>();
	weaponComponent->SetWeaponType(type);

	//std::weak_ptr<AnimatedModel> weaponModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_weaponMesh->getModel());
	//int rootNodeIndex = weaponModel.lock()->getNodeByName("Root Node");
	//weaponModel.lock()->setNodeScale(rootNodeIndex, glm::vec3(0.1f));

	// little hack #TODO: please remove ViewmodelAnimationController from shockgame.cpp
	//Component* viewmodelComponent = (Component*)TypeManager::getInstance()->createObjectByName("ViewmodelAnimationController");
	//m_weaponEntity->addComponent(viewmodelComponent);
	return weaponEntity;
}

static glm::vec3 g_prevWeaponPos;
static glm::vec3 g_currentWeaponPos;
static glm::vec3 g_weaponVelocity;

// Quakeworld bob code, this fixes jitters in the mutliplayer since the clock (pparams->time) isn't quite linear
float V_CalcBob()
{
	static	double	bobtime;
	static float	bob;
	float			cycle;
	static float	lasttime;
	const float		cl_bob = 0.01f;
	const float		cl_bobup = 0.5f;
	const float		cl_bobcycle = 0.8f;
	glm::vec3		vel;

	bobtime += Timer::GetInstance()->GetDelta();
	cycle = bobtime - (int)(bobtime / cl_bobcycle) * cl_bobcycle;
	cycle /= cl_bobcycle;

	if (cycle < cl_bobup)
	{
		cycle = maths::PI * cycle / cl_bobup;
	}
	else
	{
		cycle = maths::PI + maths::PI * (cycle - cl_bobup) / (1.0 - cl_bobup);
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	//VectorCopy(pparams->simvel, vel);
	vel = g_weaponVelocity;
	vel[1] = 0;

	bob = sqrt(vel[0] * vel[0] + vel[2] * vel[2]);//* cl_bob;
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	bob = min(bob, 4.0f);
	bob = max(bob, -7.0f);
	return bob;

}

ShockPlayerController::ShockPlayerController() :
	m_cameraEntity(nullptr),
	m_camera(nullptr),
	m_activeWeaponEntity(nullptr),
	m_rigidBody(nullptr),
	m_flyCam(true)
{
	memset(&m_playerStats, 0, sizeof(m_playerStats));
	m_playerStats.m_health = 100.0f;
	m_playerStats.m_endurance = 25.0f;
	m_playerStats.m_money = 500;
	m_weaponSwayAngles = glm::vec3(0.0f);

	m_weaponEntity[0] = nullptr;
	m_weaponEntity[1] = nullptr;
	
	m_dead = false;
}

ShockPlayerController::~ShockPlayerController()
{
}

void ShockPlayerController::RegisterObject()
{
	g_typeManager->RegisterObject<ShockPlayerController>();
}

void ShockPlayerController::OnEntitySet(Entity* entity)
{
	PlayerControllerComponent::OnEntitySet(entity);

	g_Player = entity;

	InitializeCamera();
	InitializeComponents();
}	

void ShockPlayerController::OnEntityRemove()
{
	g_freeCamera = nullptr;
	g_Player = nullptr;

	// m_cameraNode = nullptr;
	// m_cameraTransform = nullptr;
	// m_camera = nullptr;
	// m_rigidBody = nullptr;

	PlayerControllerComponent::OnEntityRemove();
}

void ShockPlayerController::AddMoney(int amount)
{
	m_playerStats.m_money += amount;

	static char s_strHUDMessage[64];
	stbsp_snprintf(s_strHUDMessage, sizeof(s_strHUDMessage), "+ %i$", amount);
	ShockPlayerHUD::GetInstance()->AddHUDMessageQueue(s_strHUDMessage);
}

void ShockPlayerController::PrintMessage(const char* msg)
{
}

void ShockPlayerController::ActivateCamera()
{
	CameraProxy::GetInstance()->SetCameraComponent(m_camera);

	g_engineData.m_shouldCaptureMouse = true;
	g_engineData.m_shouldHideMouse = true;
}

void ShockPlayerController::InitializeCamera()
{
	// m_cameraNode = getNode()->CreateChild();
	// m_cameraTransform = m_cameraNode->createComponentByType<TransformComponent>();
	// m_camera = m_cameraNode->createComponentByType<CameraFirstPersonComponent>();

	m_cameraEntity = GetEntity()->CreateChild();
	m_cameraEntity->SetPosition(glm::vec3(0.0f, 0.6f, 0.0f));

	m_camera = m_cameraEntity->CreateComponent<CameraFirstPersonComponent>();
	//m_camera = getEntity()->createComponent<CameraFirstPersonComponent>();


	m_rigidBody = GetEntity()->CreateComponent<RigidBodyProxyComponent>();
	m_rigidBody->CreatePlayerController();

	ActivateCamera();

	// create weapon
#if 0
	Entity* hackEntity = m_cameraEntity->CreateChild();
	//Entity* hackEntity = getEntity()->createChild();
	hackEntity->QuaternionRotate(glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

	m_weaponEntity = hackEntity->CreateChild();
	m_weaponEntity->SetPosition(glm::vec3(0.2f, -0.25f, -0.3f));

	// load model
	m_weaponMesh = m_weaponEntity->CreateComponent<AnimatedMeshComponent>();
	m_weaponMesh->LoadModel("models/viewmodel_shotgun.glb");

	m_weaponEntity->CreateComponent<WeaponComponent>();

	std::weak_ptr<AnimatedModel> weaponModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_weaponMesh->getModel());
	//int rootNodeIndex = weaponModel.lock()->getNodeByName("Root Node");
	//weaponModel.lock()->setNodeScale(rootNodeIndex, glm::vec3(0.1f));

	// little hack #TODO: please remove ViewmodelAnimationController from shockgame.cpp
	//Component* viewmodelComponent = (Component*)TypeManager::getInstance()->createObjectByName("ViewmodelAnimationController");
	//m_weaponEntity->addComponent(viewmodelComponent);
#endif

	// #TODO: REMOVE FROM THIS
	if (!g_freeCamera)
	{
		g_freeCameraEntity = GetWorld()->CreateEntity();
		g_freeCamera = g_freeCameraEntity->CreateComponent<CameraFirstPersonComponent>();
	}
}

void ShockPlayerController::InitializeComponents()
{
	// m_transform = getNode()->getComponentByTypeSafe<TransformComponent>();
	// m_rigidBody = getNode()->getComponentByTypeSafe<RigidBodyComponent>();
}

static IFont* s_font = nullptr;

void ShockPlayerController::Update(float dt)
{
	// if (m_rigidBody == nullptr)
	// {
	// 	initializeComponents();
	// 	//ASSERT2(m_rigidBody, "Node don't have rigid body component.");
	// }

		// #TODO: REMOVE FROM THIS
	//if (InputManager::GetInstance()->IsPressedWithReset(KeyboardKeys::KEY_F2))
	//{
	//	ActivateCamera();
	//}

	//if (InputManager::GetInstance()->IsPressedWithReset(KeyboardKeys::KEY_F3))
	//{
	//	CameraProxy::GetInstance()->SetCameraComponent(g_freeCamera);
	//	g_freeCameraEntity->SetPosition(GetEntity()->GetWorldPosition());
	//}

	if (CameraProxy::GetInstance()->GetCameraComponent() == g_freeCamera && !g_console->IsToggled())
	{
		InputManager* input = InputManager::GetInstance();
		glm::vec2 mousePos = input->GetCursorPos();

		glm::vec2 deltaMousePos = input->GetDeltaCursorPos();
		g_freeCamera->updateFromMousePosition(deltaMousePos);

		glm::vec3 cameraDirection = CameraProxy::GetInstance()->GetDirection();
		glm::vec3 pos = g_freeCameraEntity->GetPosition();
		float camSpeed = 8.0f * dt;

		if (input->IsPressed(KeyboardKeys::KEY_LEFT_SHIFT))
			camSpeed = 18.0f * dt;

		if (input->IsPressed(KeyboardKeys::KEY_UP))
			pos += camSpeed * cameraDirection;
		if (input->IsPressed(KeyboardKeys::KEY_DOWN))
			pos -= camSpeed * cameraDirection;

		//if (input->IsPressed(KeyboardKeys::KEY_Q))
		//	pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;
		//if (input->IsPressed(KeyboardKeys::KEY_E))
		//	pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;

		if (input->IsPressed(KeyboardKeys::KEY_LEFT))
			pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;
		if (input->IsPressed(KeyboardKeys::KEY_RIGHT))
			pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;

		g_freeCameraEntity->SetPosition(pos);
	}

	g_engineData.m_shouldCaptureMouse = true;
	//g_engineData.m_shouldHideMouse = false;

#if 0
	// set position
	static glm::vec3 s_weaponOffset = glm::vec3(0.0f);
	ImGui::DragFloat3("", glm::value_ptr(s_weaponOffset), 0.1, -4.0f, 4.0f);

	m_weaponEntity->SetPosition(s_weaponOffset);
#endif

	View* view = CameraProxy::GetInstance()->GetView();
	
	if (m_playerStats.m_health <= 0.0f)
	{
		char healthText[64];
		stbsp_snprintf(healthText, sizeof(healthText), "WE ARE DEAD :((((");
		ShockPlayerHUD::ms_HealthFont->DrawText(healthText, 500, 500, glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
		stbsp_snprintf(healthText, sizeof(healthText), "Press F8 to restart");
		ShockPlayerHUD::ms_HealthFont->DrawText(healthText, 500, 525, glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));

		m_cameraEntity->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

		m_activeWeaponEntity->GetComponent<WeaponComponent>()->SetActive(false);
		m_activeWeaponEntity->GetComponent<AnimatedMeshComponent>()->SetActive(false);

		m_dead = true;
	}

	// update camera look
	UpdateCamera(dt);

	// update player movement
	UpdateMovement(dt);
	
	///////////////////////////////////////////////////////////////////////
	// TO PHYSICS 
	m_rigidBody->Update(dt);

	btTransform trans = m_rigidBody->GetGhostObject()->getWorldTransform();
	GetEntity()->SetPosition(btVectorToGlm(trans.getOrigin()));
	///////////////////////////////////////////////////////////////////////
	
	// Update game logic of player controller
	UpdateLogic(dt);

	// update debug
	DebugUpdate(dt);

	if (InputManager::GetInstance()->IsPressedWithReset(KeyboardKeys::KEY_F1))
	{
		GetWorld()->TogglePhysicsDebugDraw();
	}
}

void ShockPlayerController::doHit(float amount)
{
	if (m_playerStats.m_health <= 0.0f)
		return; // we are dead

	m_playerStats.m_health -= amount;
}

void ShockPlayerController::UpdateCamera(float dt)
{
	 InputManager* input = InputManager::GetInstance();

	 glm::vec2 deltaMousePos = input->GetDeltaCursorPos();
	 m_camera->updateFromMousePosition(deltaMousePos);

	glm::quat rot = glm::eulerAngleYX(glm::radians(-m_camera->m_yaw), glm::radians(m_camera->m_pitch));

	if (m_activeWeaponEntity)
	{
		g_currentWeaponPos = m_activeWeaponEntity->GetWorldPosition();

		g_weaponVelocity = btVectorToGlm(m_rigidBody->GetCharacterController()->getLinearVelocity());
		
		glm::vec3 weaponVelocity = btVectorToGlm(m_rigidBody->GetCharacterController()->getLinearVelocity());
		glm::vec3 horizVel = glm::vec3(weaponVelocity.x, 0.0f, weaponVelocity.z);
		float horizSpeed = glm::length(horizVel);
		float verticalSpeed = weaponVelocity.y;

		// Base offset
		glm::vec3 weaponPosition = g_weaponOffset;

		// Bobbing parameters
		const float baseFreq = 8.0f;
		const float maxAmpY = 0.03f;
		const float maxAmpX = 0.015f;
		const float returnSpeed = 5.0f;

		// Time accumulator
		static float bobTime = 0.0f;
		bobTime += Timer::GetInstance()->GetDelta() * glm::clamp(horizSpeed * 60.0f, 0.0f, 1.0f); // time scales with speed

		// Smooth amplitude based on movement
		static float currentAmp = 0.0f;
		float targetAmp = (horizSpeed > 0.001f) ? 1.0f : 0.0f;
		currentAmp = glm::mix(currentAmp, targetAmp, Timer::GetInstance()->GetDelta() * returnSpeed);

		// Calculate smoothed bob
		float offsetY = sin(bobTime * baseFreq) * maxAmpY * currentAmp;
		float offsetX = sin(bobTime * baseFreq * 0.5f + glm::half_pi<float>()) * maxAmpX * currentAmp; // phase offset for side sway

		// Apply offsets
		weaponPosition.y += offsetY;
		weaponPosition.x += offsetX;

		// Jump/fall compensation (optional subtle dip)
		if (fabs(verticalSpeed) > 0.1f)
		{
			weaponPosition.y -= glm::clamp(verticalSpeed * 0.002f, -0.05f, 0.05f);
		}

		m_activeWeaponEntity->SetPosition(weaponPosition);
		m_activeWeaponEntity->SetRotation(rot);

		g_prevWeaponPos = g_currentWeaponPos;
	}
		
#if 0
	 glm::vec3 cameraDirection = CameraProxy::GetInstance()->GetDirection();
	 glm::vec3 pos = GetEntity()->GetPosition();
	 float camSpeed = 8.0f * dt;

	 if (input->IsPressed(KeyboardKeys::KEY_LEFT_SHIFT))
		 camSpeed = 18.0f * dt;
	 
	 if (input->IsPressed(KeyboardKeys::KEY_W))
	 	pos += camSpeed * cameraDirection;
	 if (input->IsPressed(KeyboardKeys::KEY_S))
	 	pos -= camSpeed * cameraDirection;
	 
	 if (input->IsPressed(KeyboardKeys::KEY_A))
	 	pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;
	 if (input->IsPressed(KeyboardKeys::KEY_D))
	 	pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;
	 
	 GetEntity()->SetPosition(pos);
#endif
}

void ShockPlayerController::UpdateMovement(float dt)
{
	if (m_dead) {
		m_rigidBody->SetDirection(glm::vec3(0.0f));
		return;
	}

	if (g_console->IsToggled())
	{
		m_rigidBody->SetDirection(glm::vec3(0.0f));
		return;
	}

	glm::vec3 moveVector = glm::vec3(0.0f);
	 
	InputManager* inputManager = InputManager::GetInstance();
	 
	bool isPlayerMove = (inputManager->IsPressed(KeyboardKeys::KEY_W)) ||
	 					(inputManager->IsPressed(KeyboardKeys::KEY_S)) ||
	 					(inputManager->IsPressed(KeyboardKeys::KEY_A)) ||
	 					(inputManager->IsPressed(KeyboardKeys::KEY_D));
	 
	Camera* camera = CameraProxy::GetInstance();
	
	m_onTheGround = m_rigidBody->GetCharacterController()->onGround();
	if (isPlayerMove && m_onTheGround)
	{
		 glm::vec3 camdir = camera->GetDirection();
		 camdir.y = 0.0f;
	
		 glm::vec3 dir = glm::vec3(0.0f);
		 if (inputManager->IsPressed(KeyboardKeys::KEY_W))
			 dir += camdir;
		 if (inputManager->IsPressed(KeyboardKeys::KEY_S))
			 dir -= camdir;
		 if (inputManager->IsPressed(KeyboardKeys::KEY_A))
			 dir -= glm::normalize(glm::cross(camdir, glm::vec3(0.0f, 1.0f, 0.0f)));
		 if (inputManager->IsPressed(KeyboardKeys::KEY_D))
			 dir += glm::normalize(glm::cross(camdir, glm::vec3(0.0f, 1.0f, 0.0f)));
	
		 // apply impulse to rigid body
		 //m_rigidBody->ApplyImpulse(dir);
		 if (glm::length(dir) >= 0.01f)
			m_rigidBody->SetDirection(glm::normalize(dir) * dt * 4.0f);
		 //m_rigidBody->SetPositionForce(getEntity()->getPosition());
		
	}
	
	if (!isPlayerMove)
		 m_rigidBody->SetDirection(glm::vec3(0.0f));
	
	// #TODO: Save jump velocity vector
	if (inputManager->IsPressed(KeyboardKeys::KEY_SPACE) && m_onTheGround)
	{
		 const glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
		 const float jumpPower = 3.0f;
	
		 glm::vec3 cameraDirection = m_camera->GetDirection(); // camera->GetDirection();
		 cameraDirection.y = 0.0f;
		 
		 m_rigidBody->GetCharacterController()->jump(glmVectorToBt(upVector * jumpPower));
	}
}

void ShockPlayerController::UpdateLogic(float dt)
{
	const float kDistance = 4.0f;

	static int cnt = 0;
	if (cnt == 0)
	{
		cnt++;
		return;
	}

	if (m_dead) {
		return;
	}

	if (InputManager::GetInstance()->IsPressed(KEY_1))
		SwitchWeapon(WeaponsType::Pistol);


	if (InputManager::GetInstance()->IsPressed(KEY_2))
		SwitchWeapon(WeaponsType::Shotgun);

	if (InputManager::GetInstance()->IsPressed(KEY_ESCAPE)) {
		g_demoGameMainMenu->SetActive(true);
		Engine::ms_pause = true;

		g_engineData.m_shouldCaptureMouse = false;
		g_engineData.m_shouldHideMouse = false;

		// todo: typo ne ok, because if you handle input by g_engineData thus
		// you have to debug your state and disable/enable because InputManager
		// it is not right to manually call it here if you comment last two lines of code
		// the cursor will hide and only alt/tab will reveive it and it is not okay :(
		InputManager::GetInstance()->SetCursorHiding(false);
		InputManager::GetInstance()->SetCursorCapture(false);
	}

	Camera* camera = CameraProxy::GetInstance();

	glm::vec3 rayBegin = camera->GetPosition() + camera->GetDirection();
	glm::vec3 rayEnd = camera->GetPosition() + (camera->GetDirection() * kDistance);

	RayCastResult rayCastResult;
	if (GetWorld()->RayCast(rayCastResult, rayBegin, rayEnd, PhysicsFilter_Usable))
	{
		UsableAreaComponent* usableArea = rayCastResult.m_entity->GetComponent<UsableAreaComponent>();
		if (usableArea)
		{
			if (InputManager::GetInstance()->IsPressedWithReset(KEY_E))
			{
				std::string scriptName = usableArea->GetScriptName();
				if (!scriptName.empty())
				{
					std::string command;
					std::string argument;

					size_t spacePosition = scriptName.find_first_of(' ');
					if (spacePosition != std::string::npos)
					{
						command = scriptName.substr(0, spacePosition);
						argument = scriptName.substr(spacePosition + 1, scriptName.length());
					}
					else
					{
						command = scriptName;
					}

					if (command == "give_weapon")
					{
						if (usableArea->GetCost() > m_playerStats.m_money) {
							ShockPlayerHUD::GetInstance()->AddInfoMessageQueue_Timed("Not enough money", 3.0f);
							return;
						}

						if (argument.empty())
						{
							Core::Msg("UsableAreaComponent(Entity 0x%p): command give_weapon doesn't have any argument!", usableArea->GetEntity());
						}
						else if (argument == "shotgun")
						{
							if (m_weaponEntity[1])
							{
								Core::Msg("UsableAreaComponent(Entity 0x%p): command %s %s already have weapon!", usableArea->GetEntity(), command.c_str(), argument.c_str());
								return;
							}

							m_weaponEntity[1] = CreateWeapon(m_cameraEntity, WeaponsType::Shotgun);

							if (m_weaponEntity[0])
								SwitchWeapon(WeaponsType::Shotgun);
							else
								m_activeWeaponEntity = m_weaponEntity[1];
							
							Core::Msg("UsableAreaComponent(Entity 0x%p): command %s %s ok", usableArea->GetEntity(), command.c_str(), argument.c_str());
						}
						else if (argument == "pistol")
						{
							if (m_weaponEntity[0])
							{
								Core::Msg("UsableAreaComponent(Entity 0x%p): command %s %s already have weapon!", usableArea->GetEntity(), command.c_str(), argument.c_str());
								return;
							}

							m_weaponEntity[0] = CreateWeapon(m_cameraEntity, WeaponsType::Pistol);

							if (m_weaponEntity[1])
								SwitchWeapon(WeaponsType::Pistol);
							else
								m_activeWeaponEntity = m_weaponEntity[0];

							Core::Msg("UsableAreaComponent(Entity 0x%p): command %s %s ok", usableArea->GetEntity(), command.c_str(), argument.c_str());
						}
						else
						{
							Core::Msg("UsableAreaComponent(Entity 0x%p): command give_weapon have unknowed argument %s!", usableArea->GetEntity(), argument.c_str());
						}
					}
				}
			}

			View* view = CameraProxy::GetInstance()->GetView();

			glm::vec4 vp = glm::vec4(0.0f, 0.0f, (float)view->m_width, (float)view->m_height);
			glm::vec3 proj = glm::project(rayCastResult.m_hitPosition, view->m_view, view->m_projection, vp);
			if (proj.z >= 1.0f)
				return; // clip

			proj.y = ((float)view->m_height - 1.0f - proj.y);

			static char s_LabelBuffer[128];

			if (usableArea)
				stbsp_snprintf(s_LabelBuffer, sizeof(s_LabelBuffer), "Press E to use (%i$)", usableArea->GetCost());
			else
				strcpy(s_LabelBuffer, "");

			const char* label = s_LabelBuffer;
			const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

			ShockPlayerHUD::ms_HealthFont->DrawText(label, proj.x - label_size.x, proj.y + 64.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

			//ImGui::GetForegroundDrawList()->AddText(ImVec2(proj.x - label_size.x/2.0f, proj.y + 32.0f /*+ label_size.y / 2.0f*/), 0xff0000ff, label);

			proj = glm::project(rayCastResult.m_hitPosition, view->m_view, view->m_projection, vp);
			proj.y = ((float)view->m_height - 1.0f - proj.y);


			//ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(proj.x - 16.0f, proj.y - 16.0f), ImVec2(proj.x + 16.0f, proj.y + 16.0f), 0xff0000ff);
		}
	}

	ShockPlayerHUD::ms_ShockPlayerHUD.Draw();
}

void ShockPlayerController::DebugUpdate(float dt)
{
	//g_debugRender.drawAxis(m_camera->GetEntity()->GetWorldPosition());

	if (!m_rigidBody)
		return;
	return;

	static char buf[256];
	
	// Ghost object stuff ...

	stbsp_snprintf(buf, sizeof(buf), "--- Ghost Object ---");
	g_fontManager->DrawSystemFontShadowed(buf, 0, 200, glm::vec4(1.0f));

	btTransform trans = m_rigidBody->GetGhostObject()->getWorldTransform();
	
	glm::vec3 ghostObjectOrigin = btVectorToGlm(trans.getOrigin());
	stbsp_snprintf(buf, sizeof(buf), "Origin: %.2f %.2f %.2f",
		ghostObjectOrigin.x, 
		ghostObjectOrigin.y, 
		ghostObjectOrigin.z);

	g_fontManager->DrawSystemFontShadowed(buf, 0, 220, glm::vec4(1.0f));

	btQuaternion ghostObjectRotation = trans.getRotation();
	stbsp_snprintf(buf, sizeof(buf), "Basis: %.2f %.2f %.2f %.2f",
		ghostObjectRotation.x(),
		ghostObjectRotation.y(),
		ghostObjectRotation.z(),
		ghostObjectRotation.w());

	g_fontManager->DrawSystemFontShadowed(buf, 0, 240, glm::vec4(1.0f));

	stbsp_snprintf(buf, sizeof(buf), "--- Character Controller ---");
	g_fontManager->DrawSystemFontShadowed(buf, 0, 280, glm::vec4(1.0f));

	stbsp_snprintf(buf, sizeof(buf), "Delta: %f", dt);
	g_fontManager->DrawSystemFontShadowed(buf, 0, 300, glm::vec4(1.0f));

	btVector3 linearVelocity = m_rigidBody->GetCharacterController()->getLinearVelocity();

	stbsp_snprintf(buf, sizeof(buf), "Linear velocity: %.2f %.2f %.2f",
		ghostObjectRotation.x(),
		ghostObjectRotation.y(),
		ghostObjectRotation.z());

	g_fontManager->DrawSystemFontShadowed(buf, 0, 320, glm::vec4(1.0f));

	btVector3 gravity = m_rigidBody->GetCharacterController()->getGravity();
	stbsp_snprintf(buf, sizeof(buf), "Gravity: %.2f %.2f %.2f",
		ghostObjectRotation.x(),
		ghostObjectRotation.y(),
		ghostObjectRotation.z());

	g_fontManager->DrawSystemFontShadowed(buf, 0, 340, glm::vec4(1.0f));

	btScalar fallSpeed = m_rigidBody->GetCharacterController()->getFallSpeed();
	stbsp_snprintf(buf, sizeof(buf), "fallSpeed: %f", fallSpeed);
	g_fontManager->DrawSystemFontShadowed(buf, 0, 360, glm::vec4(1.0f));

	bool onGround = m_rigidBody->GetCharacterController()->onGround();
	stbsp_snprintf(buf, sizeof(buf), "onGround: %s", onGround ? "true" : "false");
	g_fontManager->DrawSystemFontShadowed(buf, 0, 380, glm::vec4(1.0f));
}

void ShockPlayerController::SwitchWeapon(WeaponsType type)
{
	if (!m_weaponEntity[0] || !m_weaponEntity[1])
		return;

	if (type == WeaponsType::Shotgun) {
		m_weaponEntity[0]->GetComponent<AnimatedMeshComponent>()->SetActive(false);
		m_weaponEntity[0]->GetComponent<WeaponComponent>()->SetActive(false);

		m_activeWeaponEntity = m_weaponEntity[1];
		m_activeWeaponEntity->GetComponent<AnimatedMeshComponent>()->SetActive(true);
		m_activeWeaponEntity->GetComponent<WeaponComponent>()->SetActive(true);
	}
	else if (type == WeaponsType::Pistol) {
		m_weaponEntity[1]->GetComponent<AnimatedMeshComponent>()->SetActive(false);
		m_weaponEntity[1]->GetComponent<WeaponComponent>()->SetActive(false);

		m_activeWeaponEntity = m_weaponEntity[0];
		m_activeWeaponEntity->GetComponent<AnimatedMeshComponent>()->SetActive(true);
		m_activeWeaponEntity->GetComponent<WeaponComponent>()->SetActive(true);
	}
}

void shockGamePlayerDebug(bool* open)
{
	if (ImGui::Begin("Shock Player Debug", open))
	{
		World* world = Engine::ms_world;
		if (world)
		{
			std::vector<Entity*> players = world->GetEntityManager().GetEntitiesWithComponent<ShockPlayerController>();
			if (!players.empty())
			{
				Entity* player = players[0];
				ShockPlayerController* playerController = player->GetComponent<ShockPlayerController>();
				if (playerController)
				{
					ImGui::Checkbox("Fly cam", &playerController->m_flyCam);
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Player with ShockPlayerController doesn't exist!");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "World is not loaded!");
		}
	}

	ImGui::End();

}



}