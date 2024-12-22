#ifndef SOLUNAR_AI_PATHFINDING_NAVIGATION_INTERFACE_H
#define SOLUNAR_AI_PATHFINDING_NAVIGATION_INTERFACE_H

namespace solunar
{
	enum eNavigationDataRepresentationType
	{
		kNavigationData_Graph_XML,
		kNavigationData_Graph_Binary,
		kNavigationData_AutoGrid,
		kNavigationData_NavigationMesh,
		kNavigationData_Unknown = -1
	};

	class IPathfindingNavigationData
	{
	public:
		virtual ~IPathfindingNavigationData(void){}


		virtual eNavigationDataRepresentationType GetType() const = 0;
		virtual void DebugDraw() = 0;
	private:
	};
}

#endif