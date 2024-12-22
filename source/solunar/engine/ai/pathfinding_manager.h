#ifndef SOLUNAR_PATHFINDING_MANAGER_H
#define SOLUNAR_PATHFINDING_MANAGER_H

namespace solunar
{
	class IPathfindingNavigationData;
	class IPathfindingSolver;
}

namespace solunar
{

	constexpr const char* _kPathfindingSolver_Astar = "Astar";
	constexpr const char* _kPathfindingSerializationAttribute_AISolver = "AISolver";

	enum eSolverBackend 
	{
		kSolver_Astar,
		kSolver_Unknown=-1
	};


	class PathfindingManager : public Singleton<PathfindingManager>
	{
	public:
		PathfindingManager();
		~PathfindingManager();

		void Init(
			IPathfindingNavigationData* pDataImplBackend, 
			IPathfindingSolver* pSolver
		);

		void Shutdown();

		void ToggleDebugDraw();

		void DebugDraw();

		int GetNearestPoint(const glm::vec3& object_position);
		const glm::vec3& GetNodePosition(int node_id);

		void BuildPathToTarget(
			const glm::vec3& object_position,
			const glm::vec3& target_position,
			int* pRawBuffer,
			int number_of_elements
		);

	private:
		bool m_enable_debug_draw;
		IPathfindingNavigationData* m_pNavigationData;
		IPathfindingSolver* m_pSolver;
	};

	extern PathfindingManager* g_aiPathfindingManager;
}

#endif