#include "shockgamepch.h"
#include "engine/ai/pathfinding_manager.h"
#include "shock_ai_behaviour_tree_node_zombie_attack_target.h"
#include "shock_ai_behaviour_tree_user_data_types.h"
#include "shockplayercontroller.h"
#include "demogame.h"

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
		ZombieLogicStateType* pSharedData = static_cast<ZombieLogicStateType*>(pUserStateData);

		// is we are in closest point to the player
		int myNodeId = pSharedData->current_target_node_id;
		int playerNodeId = g_aiPathfindingManager->GetNearestPoint(g_Player->GetWorldPosition());
		if (myNodeId == playerNodeId)
		{


			return eBehaviourTreeStatus::kSuccess;
		}

		return eBehaviourTreeStatus::kFailure;
	}
}