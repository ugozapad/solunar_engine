#include "pathfinding_navigation.h"
#include "pathfinding_solver.h"
#include "pathfinding_manager.h"

namespace solunar
{
	PathfindingManager* g_aiPathfindingManager = nullptr;

	PathfindingManager::PathfindingManager() : m_enable_debug_draw(false), m_pNavigationData(nullptr), m_pSolver(nullptr)
	{
	}

	PathfindingManager::~PathfindingManager()
	{
	}

	void PathfindingManager::Init(IPathfindingNavigationData* pDataImplBackend, IPathfindingSolver* pSolver)
	{
		Assert(pDataImplBackend && "you must pass a valid implementation, just allocate the needed class implementation of IPathfindingNavigationData interface");
		Assert(pSolver && "you must pass a valid implementation, just allocate the needed class implementation of IPathfindingSolver");

		this->m_pNavigationData = pDataImplBackend;
		this->m_pSolver = pSolver;
	}

	void PathfindingManager::Shutdown()
	{
		if (m_pNavigationData)
		{
			mem_delete(m_pNavigationData);
			m_pNavigationData = nullptr;
		}

		if (m_pSolver)
		{
			mem_delete(m_pSolver);
			m_pSolver = nullptr;
		}
	}
	void PathfindingManager::ToggleDebugDraw()
	{
		this->m_enable_debug_draw = !this->m_enable_debug_draw;
	}
	void PathfindingManager::DebugDraw()
	{
		if (this->m_enable_debug_draw)
		{
			if (this->m_pNavigationData)
			{
				this->m_pNavigationData->DebugDraw();
			}
		}
	}
	int PathfindingManager::GetNearestPoint(const glm::vec3& object_position)
	{
		Assert(this->m_pSolver && "expected to be initialized when you call this method!");

		int result = -1;

		if (this->m_pSolver)
		{
			result = this->m_pSolver->GetNearestPoint(object_position);
		}

		return result;
	}

	const glm::vec3 _kDef(0.0,0.0,0.0);
	const glm::vec3& PathfindingManager::GetNodePosition(int node_id)
	{
		Assert(this->m_pSolver && "must be implementation!");

		if (this->m_pSolver)
		{
			return this->m_pSolver->GetNodePosition(node_id);
		}

		return _kDef;
	}

	void PathfindingManager::BuildPathToTarget(const glm::vec3& object_position, const glm::vec3& target_position, int* pRawBuffer, int number_of_elements)
	{
		if (!this->m_pSolver)
			return;

		if (!pRawBuffer)
			return;

		if (!number_of_elements || number_of_elements < 0)
			return;

		this->m_pSolver->BuildPathToTarget(object_position, target_position, pRawBuffer, number_of_elements);
	}
}