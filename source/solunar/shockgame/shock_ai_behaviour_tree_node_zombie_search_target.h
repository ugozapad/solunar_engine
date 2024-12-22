// -----------------------------------------
//
//   author: wh1t3lord (https://github.com/wh1t3lord)
//	 description: Action node that provides search target functionality to zombie
//
// -----------------------------------------

#ifndef SHOCK_AI_BT_NODE_SEARCH_TARGET_H
#define SHOCK_AI_BT_NODE_SEARCH_TARGET_H

#include "engine/ai/behaviour_tree.h"

namespace solunar
{
	struct ZombieLogicStateType;

	class BehaviourTreeActionNodeZombieSearchTarget : public BehaviourTreeNode
	{
	public:
		BehaviourTreeActionNodeZombieSearchTarget(const char* pDebugName);
		~BehaviourTreeActionNodeZombieSearchTarget();

		eBehaviourTreeStatus Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt) override;

	private:

		bool SearchBarricade();
		bool ValidateMyNavigationPlacement(Entity* pOwner, ZombieLogicStateType* pSharedData);

		bool SearchHumanToAttack(World* pWorld, Entity* pOwner, ZombieLogicStateType* pSharedData);

	private:
		bool m_validated_ai_navigation;
	};
}

#endif