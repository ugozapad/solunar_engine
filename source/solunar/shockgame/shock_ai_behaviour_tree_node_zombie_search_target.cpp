#include "shock_ai_behaviour_tree_node_zombie_search_target.h"
#include "engine/ai/pathfinding_manager.h"
#include "shock_ai_behaviour_tree_user_data_types.h"
#include "demogame.h"

namespace solunar
{
	BehaviourTreeActionNodeZombieSearchTarget::BehaviourTreeActionNodeZombieSearchTarget(const char* pDebugName) : m_validated_ai_navigation(false),  BehaviourTreeNode(pDebugName)
	{
	}

	BehaviourTreeActionNodeZombieSearchTarget::~BehaviourTreeActionNodeZombieSearchTarget()
	{
	}

	eBehaviourTreeStatus BehaviourTreeActionNodeZombieSearchTarget::Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt)
	{
		eBehaviourTreeStatus status = eBehaviourTreeStatus::kFailure;

		ZombieLogicStateType* pSharedData = static_cast<ZombieLogicStateType*>(pUserStateData);

		if (this->ValidateMyNavigationPlacement(pOwner, pSharedData))
		{
			if (pSharedData->need_to_validate_node)
			{
				status = eBehaviourTreeStatus::kSuccess;
			}

			// if returns here true means we have a free way so we start to search for human
			if (this->SearchBarricade())
			{
				if (this->SearchHumanToAttack(pWorld, pOwner, pSharedData))
				{
					status = eBehaviourTreeStatus::kSuccess;
				}
			}
			else
			{
				status = eBehaviourTreeStatus::kSuccess;
			}
		}

		return status;
	}

	// todo: someone implement this
	bool BehaviourTreeActionNodeZombieSearchTarget::SearchBarricade()
	{
		return true;
	}

	bool BehaviourTreeActionNodeZombieSearchTarget::ValidateMyNavigationPlacement(Entity* pOwner, ZombieLogicStateType* pSharedData)
	{
		if (!pOwner)
		{
			this->m_validated_ai_navigation = false;
			return this->m_validated_ai_navigation;
		}

		if (this->m_validated_ai_navigation == false)
		{
			const glm::vec3& position = pOwner->GetWorldPosition();
			int node_id = g_aiPathfindingManager->GetNearestPoint(position);

			if (node_id != -1)
			{
				const glm::vec3& node_pos = g_aiPathfindingManager->GetNodePosition(node_id);

				float dist = glm::length(position - node_pos);

				// means we're not near to node and need to move a little bit to it
				if (dist < 7.0f && dist > 2.0f)
				{
					if (pSharedData)
					{
						pSharedData->can_reach_target = true;
						pSharedData->need_to_validate_node = true;
						pSharedData->count_of_path = 1;
						pSharedData->path[0] = node_id;
						pSharedData->current_path_index = 0;
						pSharedData->current_target_node_id = node_id;
					}

					this->m_validated_ai_navigation = true;
				}
				else
				{
					// todo: we need to kill a such entity!!!
					if (dist > 7.0f)
						this->m_validated_ai_navigation = false;

					// we don't need to validate positioning of npc
					if (dist < 2.0f)
						this->m_validated_ai_navigation = true;
				}
			}
		}

		return this->m_validated_ai_navigation;
	}

	bool BehaviourTreeActionNodeZombieSearchTarget::SearchHumanToAttack(World* pWorld, Entity* pOwner, ZombieLogicStateType* pSharedData)
	{
		bool result = false;

		if (!pSharedData->need_to_validate_node)
		{
			if (pSharedData->current_path_index == -1 || pSharedData->current_path_index==pSharedData->count_of_path)
			{
				// search new path

				const std::vector<Entity*>& players = pWorld->GetEntityManager().GetEntitiesWithComponent<PlayerSpawnComponent>();

				auto* pPlayerEntity = players.front();

				const glm::vec3& target_position = pPlayerEntity->GetPosition();
				const glm::vec3& my_position = pOwner->GetPosition();

				g_aiPathfindingManager->BuildPathToTarget(my_position, target_position, pSharedData->path, sizeof(pSharedData->path) / sizeof(pSharedData->path[0]));

				if (pSharedData->path[0] != -1)
				{
					pSharedData->current_path_index = 0;
					pSharedData->pTarget = pPlayerEntity;
					pSharedData->current_target_node_id = -1;
					int size = sizeof(pSharedData->path) / sizeof(pSharedData->path[0]);

					pSharedData->count_of_path = 0;
					for (int i = 0; i < size; ++i)
					{
						if (pSharedData->path[i] != -1)
							pSharedData->count_of_path += 1;
						else
							break;
					}
				}
			}
			else
			{
				// proceede walking
				result = true;
			}
		}
		

		return result;
	}
}