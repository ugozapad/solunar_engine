#include "shockgamepch.h"
#include "shockgame/shockaicomponent.h"
#include "shockgame/shockprojectilecomponent.h"
#include "shockgame/shockplayercontroller.h"

#include "graphics/fontmanager.h"

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
	m_aiType(ShockAIType_None),
	m_fire(false),
	m_disable(false)
{
}

ShockAIComponent::~ShockAIComponent()
{
}

void ShockAIComponent::Update(float dt)
{
	if (m_aiType == ShockAIType_Camera)
	{
		updateAICamera(dt);
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

void ShockAIComponent::LoadXML(tinyxml2::XMLElement& element)
{
	const tinyxml2::XMLElement* aitypeElement = element.FirstChildElement("AIType");

	const char* aitypeString = nullptr;
	aitypeElement->QueryAttribute("value", &aitypeString);

	if (aitypeString)
		m_aiType = getShockAITypeFromString(aitypeString);
}

void ShockAIComponent::SaveXML(tinyxml2::XMLElement& element)
{
	std::string aitypeString = shockAITypeToString(m_aiType);

	tinyxml2::XMLElement* aitype = element.InsertNewChildElement("AIType");
	aitype->SetAttribute("value", aitypeString.c_str());
}

ShockAIType getShockAITypeFromString(const std::string& name)
{
	if (name == "ShockAIType_None")
		return ShockAIType_None;
	if (name == "ShockAIType_Camera")
		return ShockAIType_Camera;

	Assert2(0, "Unknowed AI Type");
	return ShockAIType_None;
}

std::string shockAITypeToString(ShockAIType type)
{
	switch (type)
	{
	case ShockAIType_None:
		return "ShockAIType_None";
	case ShockAIType_Camera:
		return "ShockAIType_Camera";
	}

	Assert2(0, "Unknowed AI Type");
	return std::string();

}

}