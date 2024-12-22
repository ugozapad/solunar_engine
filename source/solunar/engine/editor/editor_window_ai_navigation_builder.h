#ifndef SOLUNAR_EDITOR_AI_NAVIGATION_BUILDER_H
#define SOLUNAR_EDITOR_AI_NAVIGATION_BUILDER_H

#include "editor_window.h"

namespace solunar
{
	enum eNavigationType
	{
		kNavigationManualGraph,
		kNavigationAutoGrid,
		kNavigationNavMesh,
		kEndOfEnum
	};

	class EditorWindow_AINavigationBuilder : public IEditorWindow
	{
		struct BuilderConfig_ManualGraph
		{
			struct Node
			{
				bool selected = false;
				bool hovered = false;
				unsigned char region_id;
				unsigned char id;
				int abs_id; // for linear representation on game's side
				glm::vec3 position;
				std::vector<Node*> neighbours;
			};

			struct SelectedNode
			{
				unsigned char region_id;
				unsigned char id;
			};

			bool need_to_update_stats;
			// per region
			unsigned char max_nodes_count;
			unsigned char max_regions_count;
			unsigned char max_nodes_neighbour_per_node_count;


			unsigned char real_max_neighbours_count_for_one_node;
			// adds neighbour for each
			bool node_creation_flag_bidirectional;
			bool node_creation_flag_onedirectional;
			bool is_selecting_node_mode;
			bool is_linking_node_onedirectional;
			bool is_linking_node_bidirectional;
			// for picking our nodes on scene
			float node_collision_radius;
			int total_nodes;
			int max_possible_total_nodes;
			size_t memory_allocation_after_compilation;
			Node* pHoveredNode;
			Node* pSelectedNode;

			// linking is between two nodes 1 and 2 so left and right terminology
			Node* pLinkLeft;
			Node* pLinkRight;

			// key=region|value=Node
			std::unordered_map<unsigned char, std::vector<Node*>> nodes;
			std::vector<glm::vec3> m_centers_of_regions;
			glm::vec3 debug_region_colors[255];
		};

	public:
		EditorWindow_AINavigationBuilder(void);
		~EditorWindow_AINavigationBuilder(void);

		void Init(void) override;
		void Draw(World* pWorld) override;
		const char* GetName(void) const override;
		bool IsShow(void) const override;
		void SetShow(bool status) override;

		int GetEditingMode() override;
		void UpdateEditingMode(InputManager* pInputManager, World* pWorld) override;

		void Compile();
		void Load(tinyxml2::XMLElement& tagWorld) override;
	private:
		void UpdateStats(BuilderConfig_ManualGraph& conf);

		// Editing
		void AddNodeOnSurface(const glm::vec3& world_pos, const glm::vec3& normal);
		void UpdateCentersOfRegions();

		void ClearNodes();

		void DrawDebug();
		void DrawDebugNode(unsigned char region_id, const BuilderConfig_ManualGraph::Node* pNode);
		void DrawDebugNodeConnections(
			unsigned char region_id,
			const BuilderConfig_ManualGraph::Node* pNode
		);

		void InitConfig_ManualGraph(BuilderConfig_ManualGraph& config);

		void UpdateSelectingNode();
		void UpdateDeletingNode();
		void UpdateLinkingNode();

		bool RayIntersectionSphereNode(const glm::vec3& ray_origin, const glm::vec3& ray_dir, const glm::vec3& sphere_pos, float sphere_radius, float& t, glm::vec3& point);

		bool isLeftMouseButtonPressed();
		bool isRightMouseButtonPressed();
		bool isDeleteKeyButtonPressed();
		bool isLAltKeyButtonPressed();
		bool isLCtrlKeyButtonPressed();
	private:
		bool m_show;
		eNavigationType m_current_type;

		BuilderConfig_ManualGraph m_conf_mg;
	};

	const char* convert_enum_navtype_to_text(eNavigationType type);
	eNavigationType convert_string_to_navtype(const char* text);
}

#endif