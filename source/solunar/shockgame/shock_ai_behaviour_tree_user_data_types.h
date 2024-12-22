// -----------------------------------------
//
//   author: wh1t3lord (https://github.com/wh1t3lord)
//	 description: User data types for handling logic and passing data between all nodes of behaviour tree (for exchange/interaction purposes)
//
// -----------------------------------------

#ifndef SHOCK_AI_BT_USER_DATA_TYPES_H
#define SHOCK_AI_BT_USER_DATA_TYPES_H

namespace solunar
{
	class Entity;
}

namespace solunar
{
	// stores information for handling Zombie's AI
	struct ZombieLogicStateType
	{
		bool can_reach_target;
		bool need_to_validate_node;
		int count_of_path;
		int current_path_index;
		int current_target_node_id;
		// todo: kirrik -> replace to id for accessing entity
		Entity* pTarget;

		int path[16];
	};

	// if you don't want to use logic state then
	struct NoLogicStateType
	{};
}

#endif