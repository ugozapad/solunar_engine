#include "shockgamepch.h"
#include "shockgame/shockweaponcomponent.h"

namespace solunar
{

IMPLEMENT_OBJECT(ShockWeaponComponent, LogicComponent);

ShockWeaponComponent::ShockWeaponComponent()
{
}

ShockWeaponComponent::~ShockWeaponComponent()
{
}

void ShockWeaponComponent::RegisterObject()
{
	g_typeManager->RegisterObject<ShockWeaponComponent>();
}

void ShockWeaponComponent::OnInit()
{
}

void ShockWeaponComponent::Update(float dt)
{
	if (!IsActive())
		return;


}

glm::vec3 ShockWeaponComponent::GetLookingEntityPos(const glm::vec3& rayStart, const glm::vec3& rayEnd)
{
	RayCastResult rq = {};
	if (GetWorld()->RayCast(rq, rayStart, rayEnd))
	{
		Entity* entity = rq.m_entity;
		return rq.m_hitPosition;
	}

	return rayStart;
}

}