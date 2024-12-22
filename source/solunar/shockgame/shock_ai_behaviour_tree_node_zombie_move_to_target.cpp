#include "shock_ai_behaviour_tree_node_zombie_move_to_target.h"
#include "shock_ai_behaviour_tree_user_data_types.h"
#include "engine/ai/pathfinding_manager.h"
#include "engine/physics/rigidbodycomponent.h"

namespace solunar
{
	BehaviourTreeActionNodeZombieMoveToTarget::BehaviourTreeActionNodeZombieMoveToTarget(const char* pDebugName) : BehaviourTreeNode(pDebugName)
	{
	}

	BehaviourTreeActionNodeZombieMoveToTarget::~BehaviourTreeActionNodeZombieMoveToTarget()
	{
	}

	eBehaviourTreeStatus BehaviourTreeActionNodeZombieMoveToTarget::Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt)
	{
		constexpr float _kSpeedFactor = 0.5f;

		eBehaviourTreeStatus status = eBehaviourTreeStatus::kFailure;


		ZombieLogicStateType* pSharedData = static_cast<ZombieLogicStateType*>(pUserStateData);

		if (pSharedData->need_to_validate_node)
		{
			const glm::vec3& entity_position = pOwner->GetPosition();

			const glm::vec3& node_position = g_aiPathfindingManager->GetNodePosition(pSharedData->current_target_node_id);

			float dist = glm::length(entity_position - node_position);

			const glm::vec3& dir = glm::normalize(node_position - entity_position);

			RigidBodyComponent* pBody = pOwner->GetComponent<RigidBodyComponent>();


			Assert(pBody && "implementation supposed to have RB");

			if (pBody)
			{
				pBody->SetLinearVelocity(dir);
			}


			if (dist < 0.25f)
			{
				pSharedData->need_to_validate_node = false;
				// means we can't proceed further logic e.g. attack something because we just found our node
				status = eBehaviourTreeStatus::kFailure;

				pSharedData->current_path_index = -1;
				pSharedData->current_target_node_id = -1;
				pSharedData->count_of_path = -1;

				if (pBody)
				{
					pBody->SetLinearVelocity(glm::vec3());
				}
			}
		}
		else
		{
			if (pSharedData->current_target_node_id == -1)
			{
				pSharedData->current_target_node_id = pSharedData->path[pSharedData->current_path_index];
			}

			const glm::vec3& entity_position = pOwner->GetPosition();

			const glm::vec3& node_position = g_aiPathfindingManager->GetNodePosition(pSharedData->current_target_node_id);

			float dist = glm::length(entity_position - node_position);

			const glm::vec3& dir = glm::normalize(node_position - entity_position);

			RigidBodyComponent* pBody = pOwner->GetComponent<RigidBodyComponent>();


			Assert(pBody && "implementation supposed to have RB");

			if (pBody)
			{
				pBody->SetLinearVelocity(dir);
			}


			if (dist < 0.25f)
			{
				status = eBehaviourTreeStatus::kRunning;

				pSharedData->current_path_index += 1;
				pSharedData->current_target_node_id = -1;

				if (pBody)
				{
					pBody->SetLinearVelocity(glm::vec3());
				}

				if (pSharedData->current_path_index == pSharedData->count_of_path)
				{
					status = eBehaviourTreeStatus::kSuccess;
				}
			}
		}

		return status;
	}
}