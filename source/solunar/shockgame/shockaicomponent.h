#ifndef SHOCKAICOMPONENT_H
#define SHOCKAICOMPONENT_H

#include "graphics/animatedmodel.h"
#include "graphics/mesh.h"

namespace solunar
{

enum ShockAIType
{
	ShockAIType_None,
	ShockAIType_Camera,
	ShockAIType_Zombie,
};

enum ShockAIAnimationState
{
	ShockAIAnimationState_Idle,
	ShockAIAnimationState_Walk,
	ShockAIAnimationState_Attack,
	ShockAIAnimationState_Die,

	ShockAIAnimationState_Count
};

struct ShockAIZombieData
{
	int m_idleAnimation;
	int m_walkAnimation;
	int m_attackAnimation;
	int m_dieAnimation;

	ShockAIZombieData() :
		m_idleAnimation(-1),
		m_walkAnimation(-1),
		m_attackAnimation(-1),
		m_dieAnimation(-1)
	{
	}
};

class ShockAIComponent : public LogicComponent
{
	DECLARE_OBJECT(ShockAIComponent);
	DECLARE_PROPERTY_REGISTER(ShockAIComponent);
public:
	ShockAIComponent();
	~ShockAIComponent();

	void OnInit() override;

	void Update(float dt) override;
	void updateAICamera(float dt);
	void updateFire(float dt);

	void UpdateZombie(float dt);
	void UpdateZombie_AnimationController(float dt);
	void UpdateZombie_FSM(float dt);

	void LoadXML(tinyxml2::XMLElement& element) override;
	void SaveXML(tinyxml2::XMLElement& element) override;

	void Damage(Entity* from, float amount);

	void SetAnimationState(ShockAIAnimationState state);

private:
	void PlayAIAnimation(int animation, bool looped);

	void UpdateZombie_DumpState();

	bool IsAnimationFinished();

private:
	ShockAIZombieData m_zombieData;
	AnimatedMeshComponent* m_animatedComponent;
	ShockAIType m_aiType;
	
	ShockAIAnimationState m_currentState;
	ShockAIAnimationState m_nextState;
	float m_health;
	bool m_fire;
	bool m_disable;
	bool m_death;
};

ShockAIType GetShockAITypeFromString(const std::string& name);
std::string ShockAITypeToString(ShockAIType type);

}

#endif // !SHOCKAICOMPONENT_H
