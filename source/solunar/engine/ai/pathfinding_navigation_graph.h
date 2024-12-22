#ifndef SOLUNAR_AI_PATHFINDING_NAVIGATION_GRAPH_H
#define SOLUNAR_AI_PATHFINDING_NAVIGATION_GRAPH_H

#include "pathfinding_navigation.h"

namespace solunar
{
	struct PathfindingNavigationStaticCachedGraphNode
	{
		unsigned char id;
		unsigned char region_id;
	};

	// at least it is optimized representation that i can provide for now
	struct PathfindingNavigationStaticGraphNode
	{
		unsigned char neighbours_count;
		int id;
		int* neighbours;
		glm::vec3 world_position;
	};

	// todo: probably better to rename like add prefix _kPathfinding_ instead just _k
	constexpr const char* _kManualGraphName = "Manual Graph";
	constexpr const char* _kAutoGridName = "Auto Grid";
	constexpr const char* _kNavMeshName = "Navigation Mesh";

	//serialization
	//tags
	constexpr const char* _kSerializationTag_AI = "AI";
	constexpr const char* _kSerializationTag_AIBackend = "AIBackend";
	constexpr const char* _kSerializationTag_AIGraphData = "AIGraphData";
	constexpr const char* _kSerializationTag_Graph = "Graph";
	constexpr const char* _kSerializationTag_Node = "Node";
	constexpr const char* _kSerializationTag_NodeNeigbour = "NodeNeigbour";


	//attributes
	constexpr const char* _kSerializationAttribute_NavigationType = "NavigationType";
	constexpr const char* _kSerializationAttribute_AIDataStorageTagName = "AIDataStorageTagName";
	constexpr const char* _kSerializationAttribute_MaxNodeCount = "MaxNodeCount";
	constexpr const char* _kSerializationAttribute_MaxRegionCount = "MaxRegionCount";
	constexpr const char* _kSerializationAttribute_MaxNeighboursPerNode = "MaxNeighboursPerNode";
	constexpr const char* _kSerializationAttribute_AISolver = "AISolver";
	constexpr const char* _kSerializationAttribute_NodesCount = "NodesCount";
	constexpr const char* _kSerializationAttribute_RegionsCount = "RegionsCount";
	constexpr const char* _kSerializationAttribute_id = "id";
	constexpr const char* _kSerializationAttribute_region_id = "region_id";
	constexpr const char* _kSerializationAttribute_abs_id = "abs_id";
	constexpr const char* _kSerializationAttribute_neighbours_count = "neighbours_count";
	constexpr const char* _kSerializationAttribute_pos_x = "pos_x";
	constexpr const char* _kSerializationAttribute_pos_y = "pos_y";
	constexpr const char* _kSerializationAttribute_pos_z = "pos_z";

	// todo: provide implementation for streaming graph from file for HUGE worlds
	// name it PathfindingNavigationStreamingGraph (if it is needed)

	// todo: provide implementation for binary :((

	/*
		binary file (after compilation):
			
			HEADER SECTION

			name															type

			total_nodes_count=nodes_per_region_count*total_regions_count,	unsigned char
			nodes_per_region_count,											unsigned char
			total_regions_count,											unsigned char
			calculated_max_neighbours_in_all_nodes,							unsigned char
			calculated_memory_for_allocation,								size_t

			NODES SECTION	(total_nodes_count * sizeof(PathfindingNavigationStaticCachedGraphNode))

			node_id (unsigned char), region_id (unsigned char)	
			0						0
			1						0
			2						0
			3						0
			4						0
			...

			NEIGBOURS SECTION (total_nodes_count * calculated_max_neighbours_in_all_nodes)
			
			since it is a look-up table so it is sorted by ids

		0:	neighbours_count,	unsigned char
			neighbour			unsigned char
			neighbour			unsigned char
			neighbour			unsigned char
			...

		1: neighbours_count,	unsigned char
			neighbour			unsigned char
			neighbour			unsigned char
			neighbour			unsigned char
			...

			REGION CENTERS, size=(max_region_count * sizeof(float)*3)

		1:	x	float
			y	float
			z	float

			NODES POSITIONS, size=(total_nodes_count * sizeof(float)*3)

		0:	x	float
			y	float
			z	float


		total memory for allocation = NODES POSITIONS + REGION CENTERS + NEIGBOURS SECTION + NODES SECTION 

		for example:
			total_nodes_count=720
			total_regions_count=10
			max_neighbours=16

			NODES POSITIONS = 720 * 12 = 8640 bytes
			REGION CENTERS = 10 * 12 = 120 bytes
			NEIGBOURS SECTION = 720 * 16 = 11520 bytes (because one node can have 16 nodes as neighbours)
			NODES SECTION = 720 * 2 = 1440 bytes

			total memory = 21720 bytes

			So world positions it is pointless to pack into long long, so just store as is. 

			But neighbours it is vital to keep neighbours max size minimal!

			You can specify in graph builder settings:
				- max nodes per region count  
				- max neighbours per node count
				- max region count
	*/
	class PathfindingNavigationStaticGraphBinary : IPathfindingNavigationData
	{
	public:
		PathfindingNavigationStaticGraphBinary(void);
		~PathfindingNavigationStaticGraphBinary(void);

		void Init();
		eNavigationDataRepresentationType GetType() const override;
	private:
		unsigned char m_region_count;
		unsigned char m_nodes_per_region_count;
		unsigned char m_max_neighbours_per_node;
		float* m_pWorldPositionCenterOfRegions;  
		PathfindingNavigationStaticCachedGraphNode* m_pNodes;  
		unsigned char** m_pNeighbours; // look-up table
		float* m_pWorldPositionOfNodes;
	};

	// todo: provide streaming for this too :D

	constexpr int _kPNSG_MaxNeighbourCount = 1024;

	class PathfindingNavigationStaticGraph : public IPathfindingNavigationData
	{
	public:
		PathfindingNavigationStaticGraph();
		~PathfindingNavigationStaticGraph();

		void Init(tinyxml2::XMLElement& tagAIDataStorage);
		void Shutdown();

		const std::vector<PathfindingNavigationStaticGraphNode>& GetNodes(void) const;
		const PathfindingNavigationStaticGraphNode& GetNodeByID(int node_id) const;
		eNavigationDataRepresentationType GetType() const override;

		void DebugDraw() override;

	private:
		void DebugDraw(const PathfindingNavigationStaticGraphNode& node);
		void DebugDrawConnections(const PathfindingNavigationStaticGraphNode& node);

	private:
		bool m_init;
		// placement new allocating data
		int m_allocated_count;
		unsigned char m_neighbours_allocator[sizeof(int) * _kPNSG_MaxNeighbourCount];

		// look-up table of all nodes
		std::vector<PathfindingNavigationStaticGraphNode> m_nodes;
	};
}

#endif