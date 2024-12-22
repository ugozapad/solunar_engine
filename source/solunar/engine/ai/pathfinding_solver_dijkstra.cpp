#include "pathfinding_solver_dijkstra.h"
#include "pathfinding_navigation_graph.h"

namespace solunar
{
	PathfindingSolverDijkstra::PathfindingSolverDijkstra(void) : m_pNavigationData(nullptr), m_def_pos(0.0f, 0.0f, 0.0f)
	{
	}

	PathfindingSolverDijkstra::~PathfindingSolverDijkstra(void)
	{
	}

	void PathfindingSolverDijkstra::Init(IPathfindingNavigationData* pDataImpl)
	{
		Assert(pDataImpl && "must be valid");
		this->m_pNavigationData = pDataImpl;
	}

	void PathfindingSolverDijkstra::Shutdown()
	{
	}

	void PathfindingSolverDijkstra::GetNearestPoint(unsigned char* pOutNodeID, unsigned char* pOutRegionID, float* pOutWorldPointPosition, unsigned char out_vec_size)
	{
		Assert(!"Not implemented");
	}

#undef max
	int PathfindingSolverDijkstra::GetNearestPoint(const glm::vec3& object_position)
	{
		Assert(this->m_pNavigationData && "early calling, initialize this before call this");

		int result = -1;

		if (!this->m_pNavigationData)
			return result;


		switch (this->m_pNavigationData->GetType())
		{
		case eNavigationDataRepresentationType::kNavigationData_Graph_XML:
		{
			PathfindingNavigationStaticGraph* pCasted = static_cast<PathfindingNavigationStaticGraph*>(this->m_pNavigationData);

			if (pCasted)
			{
				const auto& nodes = pCasted->GetNodes();

				char out[32];
				float min_dist = std::numeric_limits<float>::max();


				for (const auto& node : nodes)
				{
					float dist = glm::length(object_position - node.world_position);


					if (dist < min_dist)
					{
						sprintf(out, sizeof(out), "dist: %.3f\n", dist);
						OutputDebugStringA(out);
						result = node.id;
						min_dist = dist;
					}
				}
			}

			break;
		}
		default:
		{
			Assert(!"not implemeneted yet!");
			break;
		}
		}


		return result;
	}

	void PathfindingSolverDijkstra::BuildPathToTarget(const glm::vec3& object_position, const glm::vec3& target_position, std::vector<int>& nodes)
	{
	}

	void PathfindingSolverDijkstra::BuildPathToTarget(const glm::vec3& object_position, const glm::vec3& target_position, int* pRawBuffer, int number_of_elements)
	{
		int start_node = GetNearestPoint(object_position);
		int end_node = GetNearestPoint(target_position);

		BuildPathToTarget(start_node, end_node, pRawBuffer, number_of_elements);
	}

	void PathfindingSolverDijkstra::BuildPathToTarget(int start_node_id, int goal_node_id, int* pRawBuffer, int number_of_elements)
	{
		if (start_node_id == -1)
			return;

		if (goal_node_id == -1)
			return;

		if (!pRawBuffer)
			return;

		if (number_of_elements <= 0)
			return;

		Assert(this->m_pNavigationData && "must be initialized");

		if (!this->m_pNavigationData)
			return;

		int current_iter = 0;

		// invalidate path
		memset(pRawBuffer, -1, sizeof(pRawBuffer[0]) * number_of_elements);

		switch (this->m_pNavigationData->GetType())
		{
		case eNavigationDataRepresentationType::kNavigationData_Graph_XML:
		{
			PathfindingNavigationStaticGraph* pCasted = static_cast<PathfindingNavigationStaticGraph*>(this->m_pNavigationData);

			if (pCasted)
			{
				const auto& nodes = pCasted->GetNodes();
				const auto* pNode = &pCasted->GetNodeByID(start_node_id);


				const glm::vec3& goal_node_position = pCasted->GetNodeByID(goal_node_id).world_position;
				// not true implementation at all
				// todo: implement real dijkstra pls
				const PathfindingNavigationStaticGraphNode* pSelectedNode = nullptr;
				constexpr float _kFLOATMAX = std::numeric_limits<float>::max();
				bool visited[1024];
				memset(visited, 0, sizeof(visited));
				visited[start_node_id] = true;
				Assert(pCasted->GetNodes().size() < (sizeof(visited) / sizeof(visited[0])) && "overflow!");

				while (pNode->id != goal_node_id)
				{
					pSelectedNode = nullptr;
					float min_dist = _kFLOATMAX;
					for (int i = 0; i < pNode->neighbours_count; ++i)
					{
						const auto* pNodeNeighbour = &pCasted->GetNodeByID(pNode->neighbours[i]);

						if (pNodeNeighbour->id == goal_node_id)
						{
							pSelectedNode = pNodeNeighbour;
							break;
						}

						if (visited[pNodeNeighbour->id])
							continue;

						float dist = glm::length(goal_node_position - pNodeNeighbour->world_position);

						if (dist < min_dist)
						{
							pSelectedNode = pNodeNeighbour;
							min_dist = dist;
						}
					}

					if (pSelectedNode)
					{
						if (current_iter + 1 == number_of_elements)
							break;

						pRawBuffer[current_iter] = pSelectedNode->id;
						++current_iter;
						pNode = pSelectedNode;
						visited[pSelectedNode->id] = true;
					}
					else
					{
						Assert(!"your navigation topology is not complete please connect all nodes between each other in order to make goal node reachable");
						// unable to find path to goal
						break;
					}
				}
			}

			break;
		}
		default:
		{
			Assert(!"Dijkstra is not good for grids, use Astar");
			break;
		}
		}
	}

	const glm::vec3& PathfindingSolverDijkstra::GetNodePosition(int node_id) const
	{
		if (this->m_pNavigationData)
		{
			switch (this->m_pNavigationData->GetType())
			{
			case eNavigationDataRepresentationType::kNavigationData_Graph_XML:
			{
				PathfindingNavigationStaticGraph* pCasted = static_cast<PathfindingNavigationStaticGraph*>(this->m_pNavigationData);

				const auto& nodes = pCasted->GetNodes();

				if (node_id > nodes.size() || node_id < 0)
				{
					Core::Msg("AI: Can't obtain node by id %d", node_id);
					return this->m_def_pos;
				}

				return nodes[node_id].world_position;
			}
			default:
			{
				Assert(!"not implemented!");
				break;
			}
			}
		}

		return this->m_def_pos;
	}
}