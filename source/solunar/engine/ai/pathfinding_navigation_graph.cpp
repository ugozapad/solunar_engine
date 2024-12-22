#include "pathfinding_navigation_graph.h"
#include "graphics/debugrenderer.h"

namespace solunar
{
	PathfindingNavigationStaticGraph::PathfindingNavigationStaticGraph() : m_init(false), m_allocated_count(0)
	{
	}

	PathfindingNavigationStaticGraph::~PathfindingNavigationStaticGraph()
	{
	}

	void PathfindingNavigationStaticGraph::Init(tinyxml2::XMLElement& tagAIDataStorage)
	{
		auto* tag_graph = tagAIDataStorage.FirstChildElement(_kSerializationTag_Graph);

		if (tag_graph)
		{
			auto* attr_nodes_count = tag_graph->FindAttribute(_kSerializationAttribute_NodesCount);

			if (attr_nodes_count)
			{
				int nodes_count = attr_nodes_count->IntValue();

				auto* iter = tag_graph->FirstChildElement();

				if (!iter)
				{
					Core::Warning("Your graph is empty, no data to load!");
					return;
				}

				for (int i = 0; i < nodes_count; ++i)
				{
					//auto* attr_id = iter->FindAttribute(_kSerializationAttribute_id);
					//auto* attr_region_id = iter->FindAttribute(_kSerializationAttribute_region_id);
					
					auto* attr_abs_id = iter->FindAttribute(_kSerializationAttribute_abs_id);
					auto* attr_pos_x = iter->FindAttribute(_kSerializationAttribute_pos_x);
					auto* attr_pos_y = iter->FindAttribute(_kSerializationAttribute_pos_y);
					auto* attr_pos_z = iter->FindAttribute(_kSerializationAttribute_pos_z);

					if (attr_abs_id && attr_pos_x&& attr_pos_y&& attr_pos_z)
					{
						//unsigned char id = static_cast<unsigned char>(attr_id->IntValue());
						//unsigned char region_id = static_cast<unsigned char>(attr_region_id->IntValue());

						int id = attr_abs_id->IntValue();

						float pos_x = attr_pos_x->FloatValue();
						float pos_y = attr_pos_y->FloatValue();
						float pos_z = attr_pos_z->FloatValue();

						PathfindingNavigationStaticGraphNode instance;

						instance.id = id;

						instance.world_position.x = pos_x;
						instance.world_position.y = pos_y;
						instance.world_position.z = pos_z;
						instance.neighbours = nullptr;
						instance.neighbours_count = 0;

						this->m_nodes.push_back(instance);
					}
					else
					{
						Core::Warning("Invalid data of Node abs id = %d (can't load node)", i);
					}

					iter = iter->NextSiblingElement();
				}

				iter = tag_graph->FirstChildElement();
				for (int i = 0; i < nodes_count; ++i)
				{
				//	auto* attr_id = iter->FindAttribute(_kSerializationAttribute_id);
				//	auto* attr_region_id = iter->FindAttribute(_kSerializationAttribute_region_id);

				//	unsigned char id = static_cast<unsigned char>(attr_id->IntValue());
				//	unsigned char region_id = static_cast<unsigned char>(attr_region_id->IntValue());

					auto* attr_abs_id = iter->FindAttribute(_kSerializationAttribute_abs_id);

					int id = attr_abs_id->IntValue();

					auto node = std::find_if(this->m_nodes.begin(), this->m_nodes.end(), [id](const PathfindingNavigationStaticGraphNode& node) -> bool {
						return node.id == id;
					});

					Assert(node != this->m_nodes.end() && "kek!");

					auto* attr_neighbour_count = iter->FindAttribute(_kSerializationAttribute_neighbours_count);

					if (attr_neighbour_count)
					{
						unsigned char nc = static_cast<unsigned char>(attr_neighbour_count->IntValue());

						auto* iter_n = iter->FirstChildElement();

						if (iter_n)
						{
							for (unsigned char j = 0; j < nc; ++j)
							{
							//	auto* attr_neighbour_id = iter_n->FindAttribute(_kSerializationAttribute_id);
							//	auto* attr_neighbour_region_id = iter_n->FindAttribute(_kSerializationAttribute_region_id);
								auto* attr_neighbour_abs_id = iter_n->FindAttribute(_kSerializationAttribute_abs_id);

								if (attr_neighbour_abs_id)
								{
									//unsigned char neighbour_node_id = static_cast<unsigned char>(attr_neighbour_id->IntValue());
								//	unsigned char neighbour_region_id = static_cast<unsigned char>(attr_neighbour_region_id->IntValue());
									int neighbour_node_id = attr_neighbour_abs_id->IntValue();

									// not allocated space
									if (!node->neighbours)
									{
										unsigned char* pStartPointer = this->m_neighbours_allocator + (sizeof(int)*this->m_allocated_count);
										this->m_allocated_count += nc;
										node->neighbours = new (pStartPointer) int[nc];
									}

									if (node->neighbours)
									{
										node->neighbours[node->neighbours_count] = neighbour_node_id;
										++node->neighbours_count;
									}
								}
								else
								{
									Core::Warning("Invalid neighbour node, parent abs id = %d", i);
								}

								iter_n = iter_n->NextSiblingElement();
							}
						}
						else
						{
							if (nc)
								Core::Warning("FAILED to obtain neighbours but count is presented!! abs id = %d", i);
						}
					}

					iter = iter->NextSiblingElement();
				}

				m_init = true;
			}
			else
			{
				Core::Warning("FAILED to obtain attribute %s Can't load data", _kSerializationAttribute_NodesCount);
			}
		}
	}

	void PathfindingNavigationStaticGraph::Shutdown()
	{
	}
	
	const std::vector<PathfindingNavigationStaticGraphNode>& PathfindingNavigationStaticGraph::GetNodes(void) const
	{
		return this->m_nodes;
	}

	const PathfindingNavigationStaticGraphNode& PathfindingNavigationStaticGraph::GetNodeByID(int node_id) const
	{
		return this->m_nodes[node_id];
	}

	eNavigationDataRepresentationType PathfindingNavigationStaticGraph::GetType() const
	{
		return eNavigationDataRepresentationType::kNavigationData_Graph_XML;
	}

	glm::vec3 _kDebugColor(0.0f, 1.0f, 0.25f);
	constexpr float _kMGLengthOfRoot = 0.5f;
	constexpr float _kMGLengthOfArrow = 0.25f;

	void PathfindingNavigationStaticGraph::DebugDraw()
	{
		if (!this->m_init)
			return;

		for (const PathfindingNavigationStaticGraphNode& node : this->m_nodes)
		{
			this->DebugDraw(node);
			this->DebugDrawConnections(node);
		}
	}
	void PathfindingNavigationStaticGraph::DebugDraw(const PathfindingNavigationStaticGraphNode& node)
	{
		g_debugRender.DrawLine(node.world_position, node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), _kDebugColor);
		g_debugRender.DrawLine(node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), node.world_position + glm::vec3(_kMGLengthOfArrow, 0.0f, 0.0f), _kDebugColor);
		g_debugRender.DrawLine(node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), node.world_position + glm::vec3(-_kMGLengthOfArrow, 0.0f, 0.0f), _kDebugColor);
		g_debugRender.DrawLine(node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), node.world_position + glm::vec3(0.0f, 0.0f, _kMGLengthOfArrow), _kDebugColor);
		g_debugRender.DrawLine(node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), node.world_position + glm::vec3(0.0f, 0.0f, -_kMGLengthOfArrow), _kDebugColor);

	}

	void PathfindingNavigationStaticGraph::DebugDrawConnections(const PathfindingNavigationStaticGraphNode& node)
	{
		for (int i = 0; i < node.neighbours_count; ++i)
		{
			const PathfindingNavigationStaticGraphNode& neighbour_node = this->m_nodes[node.neighbours[i]];

			glm::vec3 from_me_to_neigbour_dir = (neighbour_node.world_position - node.world_position);
			from_me_to_neigbour_dir = glm::normalize(from_me_to_neigbour_dir);

			g_debugRender.DrawLine(node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (from_me_to_neigbour_dir * _kMGLengthOfArrow), neighbour_node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), _kDebugColor);

			glm::vec3 right = (neighbour_node.world_position - node.world_position);
			right = glm::cross(right, glm::vec3(0.0, 1.0, 0.0));
			right = glm::normalize(right);
			g_debugRender.DrawLine(neighbour_node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), (neighbour_node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (right * _kMGLengthOfArrow)) - (glm::normalize((neighbour_node.world_position - node.world_position)) * 0.6f), _kDebugColor);
			g_debugRender.DrawLine(neighbour_node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), (neighbour_node.world_position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (right * -_kMGLengthOfArrow)) - (glm::normalize((neighbour_node.world_position - node.world_position)) * 0.6f), _kDebugColor);
		}
	}
}