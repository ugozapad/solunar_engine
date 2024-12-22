#ifndef SOLUNAR_AI_PATHFINDING_SOLVER_DIJKSTRA_H
#define SOLUNAR_AI_PATHFINDING_SOLVER_DIJKSTRA_H

#include "pathfinding_solver.h"

namespace solunar
{
	class PathfindingSolverDijkstra : public IPathfindingSolver
	{
	public:
		PathfindingSolverDijkstra(void);
		~PathfindingSolverDijkstra(void);

		void Init(IPathfindingNavigationData* pDataImpl) override;
		void Shutdown() override;

		void GetNearestPoint(
			unsigned char* pOutNodeID,
			unsigned char* pOutRegionID,
			float* pOutWorldPointPosition,
			unsigned char out_vec_size
		) override;

		// if returns -1 it means failed to obtain
		int GetNearestPoint(
			const glm::vec3& object_position
		) override;

		void BuildPathToTarget(
			const glm::vec3& object_position,
			const glm::vec3& target_position,
			std::vector<int>& nodes
		) override;

		void BuildPathToTarget(
			const glm::vec3& object_position,
			const glm::vec3& target_position,
			int* pRawBuffer,
			int number_of_elements
		) override;

		void BuildPathToTarget(
			int start_node_id,
			int goal_node_id,
			int* pRawBuffer,
			int number_of_elements
		) override;

		const glm::vec3& GetNodePosition(int node_id) const override;

	private:
		IPathfindingNavigationData* m_pNavigationData;
		// if GetNodePosition failed
		glm::vec3 m_def_pos;
	};
}

#endif