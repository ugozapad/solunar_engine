#include "pathfinding_solver_astar.h"
#include "pathfinding_navigation_graph.h"

namespace solunar
{
	PathfindingSolverAstar::PathfindingSolverAstar(void) : m_pNavigationData(nullptr), m_def_pos(0.0f, 0.0f, 0.0f)
	{
	}

	PathfindingSolverAstar::~PathfindingSolverAstar(void)
	{
	}

	void PathfindingSolverAstar::Init(IPathfindingNavigationData* pDataImpl)
	{
		Assert(pDataImpl && "must be valid");
		this->m_pNavigationData = pDataImpl;
	}

	void PathfindingSolverAstar::Shutdown()
	{
	}

	void PathfindingSolverAstar::GetNearestPoint(unsigned char* pOutNodeID, unsigned char* pOutRegionID, float* pOutWorldPointPosition, unsigned char out_vec_size)
	{
		Assert(!"Not implemented");
	}

#undef max
	int PathfindingSolverAstar::GetNearestPoint(const glm::vec3& object_position)
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

	void PathfindingSolverAstar::BuildPathToTarget(const glm::vec3& object_position, const glm::vec3& target_position, std::vector<int>& nodes)
	{
	}

	void PathfindingSolverAstar::BuildPathToTarget(const glm::vec3& object_position, const glm::vec3& target_position, int* pRawBuffer, int number_of_elements)
	{
		int start_node = GetNearestPoint(object_position);
		int end_node = GetNearestPoint(target_position);

		BuildPathToTarget(start_node, end_node, pRawBuffer, number_of_elements);
	}

	void PathfindingSolverAstar::BuildPathToTarget(int start_node_id, int goal_node_id, int* pRawBuffer, int number_of_elements)
	{
		if (start_node_id == -1)
			return;

		if (goal_node_id == -1)
			return;

		if (!pRawBuffer)
			return;

		if (number_of_elements <= 0)
			return;

		int current_iter = 0;



	}

	const glm::vec3& PathfindingSolverAstar::GetNodePosition(int node_id) const
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