#include "shock_ai_behaviour_tree_node_zombie_attack_target.h"

namespace solunar
{
	BehaviourTreeActionNodeZombieAttackTarget::BehaviourTreeActionNodeZombieAttackTarget(const char* pDebugName) : BehaviourTreeNode(pDebugName)
	{
	}

	BehaviourTreeActionNodeZombieAttackTarget::~BehaviourTreeActionNodeZombieAttackTarget()
	{
	}

	eBehaviourTreeStatus BehaviourTreeActionNodeZombieAttackTarget::Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt)
	{
		return eBehaviourTreeStatus();
	}
}