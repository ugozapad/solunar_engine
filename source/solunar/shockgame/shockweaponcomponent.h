#ifndef SHOCKWEAPONCOMPONENT_H
#define SHOCKWEAPONCOMPONENT_H

#include "engine/entity/logiccomponent.h"

namespace solunar
{

struct WeaponData
{
	int m_animIdle;
	int m_animReload;
	int m_animShot;

	WeaponData() :
		m_animIdle(-1),
		m_animReload(-1),
		m_animShot(-1)
	{
	}
};

class ShockWeaponComponent : public LogicComponent
{
	DECLARE_OBJECT(ShockWeaponComponent);
public:
	ShockWeaponComponent();
	~ShockWeaponComponent();

	static void RegisterObject();

	void OnInit() override;

	void Update(float dt) override;

private:
	WeaponData m_weaponData;

private:
	// This is player stuff and should be moved out here!
	glm::vec3 GetLookingEntityPos(const glm::vec3& rayStart, const glm::vec3& rayEnd);
};

}

#endif // !SHOCKWEAPONCOMPONENT_H
