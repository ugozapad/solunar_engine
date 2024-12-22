#include "shockgamepch.h"
#include "shockgame/shockaicomponent.h"
#include "shockgame/shockprojectilecomponent.h"
#include "shockgame/shockplayercontroller.h"

#include "shockgame/demogame.h"
#include "shockgame/shock_component_ai_behaviourtree.h"

#include "graphics/fontmanager.h"
#include "graphics/debugrenderer.h"

namespace solunar
{

IMPLEMENT_OBJECT(ShockAIComponent, LogicComponent);

BEGIN_PROPERTY_REGISTER(ShockAIComponent)
{
	REGISTER_PROPERTY(ShockAIComponent, PropertyInt, m_aiType);
	REGISTER_PROPERTY(ShockAIComponent, PropertyBool, m_fire);
	REGISTER_PROPERTY(ShockAIComponent, PropertyBool, m_disable);
}
END_PROPERTY_REGISTER(ShockAIComponent)

ShockAIComponent::ShockAIComponent() :
	m_animatedComponent(nullptr),
	m_aiType(ShockAIType_None),
	m_fire(false),
	m_disable(false)
{
}

ShockAIComponent::~ShockAIComponent()
{
}

void ShockAIComponent::OnInit()
{
	Entity* entity = GetEntity();

	m_animatedComponent = entity->GetComponent<AnimatedMeshComponent>();
	if (!m_animatedComponent)
	{
		Core::Msg("ShockAIComponent::OnInit: entity 0x%p doesn't have animated mesh component!", entity);
		return;
	}
	
	// Initialize animations
	std::shared_ptr<AnimatedModel> animatedModel = m_animatedComponent->LockAnimatedModel();
	m_zombieData.m_idleAnimation = animatedModel->GetAnimationByName("idle");
	m_zombieData.m_walkAnimation = animatedModel->GetAnimationByName("walk");
	m_zombieData.m_attackAnimation = animatedModel->GetAnimationByName("attack");
	m_zombieData.m_dieAnimation = animatedModel->GetAnimationByName("die");
}

void ShockAIComponent::Update(float dt)
{
	if (m_aiType == ShockAIType_Camera)
	{
		updateAICamera(dt);
	}
	else if (m_aiType == ShockAIType_Zombie)
	{
	//	UpdateZombie(dt);

		ShockAIBehaviourTree* pBT = this->GetEntity()->GetComponent<ShockAIBehaviourTree>();

		if (pBT)
		{
			pBT->Update(dt);
		}
	}
}

void ShockAIComponent::updateAICamera(float dt)
{
	// find player
	std::vector<Entity*> players = GetWorld()->GetEntityManager().GetEntitiesWithComponent<ShockPlayerController>();

	Entity* player = players[0];

	// check distance to the camera
	glm::vec3 distance = player->GetPosition() - GetEntity()->GetPosition();

	char aiDebugText[64];
	snprintf(aiDebugText, sizeof(aiDebugText), "distance %.2f %.2f %.2f",
		distance.x, distance.y, distance.z);

	g_fontManager->DrawSystemFontShadowed(aiDebugText, 0, 100, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	snprintf(aiDebugText, sizeof(aiDebugText), "length %.2f", glm::length(distance));
	g_fontManager->DrawSystemFontShadowed(aiDebugText, 0, 120, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));


	if (glm::length(distance) < 8.0f)
	{
		m_fire = true;
	}

	if (m_fire && !m_disable)
	{
		Entity* projectile = Engine::ms_world->CreateEntity();
		projectile->SetPosition(GetEntity()->GetPosition());

		ShockProjectileComponent* projectileComponent = projectile->CreateComponent<ShockProjectileComponent>();
		//projectileComponent->onSpawn(glm::normalize(player->getPosition()), glm::vec3(1.0f), "test");
		projectileComponent->onSpawn(player->GetPosition(), glm::vec3(1.0f), "test");

		m_fire = false;
		m_disable = true;
	}

	// rotate camera torwads to player
	glm::quat rotation = glm::quatLookAt(glm::normalize(player->GetPosition() - GetEntity()->GetPosition()), glm::vec3(0.0f, 1.0f, 0.0f));
	GetEntity()->SetRotation(glm::slerp(GetEntity()->GetRotation(), rotation, dt * 0.8f));
}

void ShockAIComponent::updateFire(float dt)
{

}

void ShockAIComponent::UpdateZombie(float dt)
{
	glm::vec3 direction = glm::normalize(g_Player->GetPosition() - GetEntity()->GetPosition());
	
	/*
	glm::vec3 pos = GetEntity()->GetPosition();
	pos += direction * dt;
	pos.y = GetEntity()->GetPosition().y;
	GetEntity()->SetPosition(pos);
	*/

	RigidBodyComponent* rigidBody = (RigidBodyComponent*)GetEntity()->GetComponentByTypeInfo(RigidBodyComponent::GetStaticTypeInfo());

	// to wh1t3lord: uncomment this to move entity !!!!
//	if (rigidBody)
//		rigidBody->SetLinearVelocity(direction * 2.0f);

	// rotate camera torwads to player
	glm::vec3 playerPos = g_Player->GetPosition();
	playerPos.y = 0.0f;

	glm::vec3 characterPos = GetEntity()->GetPosition();
	characterPos.y = 0.0f;

	glm::quat rotation = glm::quatLookAt(glm::normalize(playerPos - characterPos), glm::vec3(0.0f, 1.0f, 0.0f));
	GetEntity()->SetRotation(rotation);

	if (m_animatedComponent)
		UpdateZombie_AnimationController(dt);
}

void ShockAIComponent::UpdateZombie_AnimationController(float dt)
{
	// cycle of playing anims
#if 0
	static int playingAnimationId = 0;

	std::shared_ptr<AnimatedModel> animatedModel = m_animatedComponent->LockAnimatedModel();
	if (animatedModel->IsStoped())
	{
		if (playingAnimationId >= animatedModel->GetAnimationCount())
			playingAnimationId = 0;

		animatedModel->PlayAnimation( playingAnimationId );

		playingAnimationId++;
	}
#endif

	std::shared_ptr<AnimatedModel> animatedModel = m_animatedComponent->LockAnimatedModel();
	if (animatedModel->IsStoped())
	{
		animatedModel->PlayAnimation( m_zombieData.m_idleAnimation );
	}

	animatedModel->Update(dt);

	// debug stuff
#if 0
	static char debugText[128];
	sprintf(debugText, "Animation: %s", animatedModel->GetCurrentAnimation()->m_name.c_str());
	Debug_Draw3DText(debugText, GetEntity()->GetPosition(), glm::vec4(1.f,1.f,1.f,1.f), -50.0f);
	sprintf(debugText, "Time: %f", animatedModel->GetCurrentTime());
	Debug_Draw3DText(debugText, GetEntity()->GetPosition(), glm::vec4(1.f, 1.f, 1.f, 1.f), -25.0f);
#endif
}

void ShockAIComponent::LoadXML(tinyxml2::XMLElement& element)
{
	const tinyxml2::XMLElement* aitypeElement = element.FirstChildElement("AIType");

	const char* aitypeString = nullptr;
	aitypeElement->QueryAttribute("value", &aitypeString);

	if (aitypeString)
		m_aiType = GetShockAITypeFromString(aitypeString);
}

void ShockAIComponent::SaveXML(tinyxml2::XMLElement& element)
{
	std::string aitypeString = ShockAITypeToString(m_aiType);

	tinyxml2::XMLElement* aitype = element.InsertNewChildElement("AIType");
	aitype->SetAttribute("value", aitypeString.c_str());
}

ShockAIType GetShockAITypeFromString(const std::string& name)
{
	if (name == "ShockAIType_None")
		return ShockAIType_None;
	if (name == "ShockAIType_Camera")
		return ShockAIType_Camera;
	if (name == "ShockAIType_Zombie")
		return ShockAIType_Zombie;

	Assert2(0, "Unknowed AI Type");
	return ShockAIType_None;
}

std::string ShockAITypeToString(ShockAIType type)
{
	switch (type)
	{
	case ShockAIType_None:
		return "ShockAIType_None";
	case ShockAIType_Camera:
		return "ShockAIType_Camera";
	case ShockAIType_Zombie:
		return "ShockAIType_Zombie";
	}

	Assert2(0, "Unknowed AI Type");
	return "";

}

}