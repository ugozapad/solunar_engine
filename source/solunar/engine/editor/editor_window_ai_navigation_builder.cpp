#include "editor_window_ai_navigation_builder.h"
#include "imgui.h"
#include "ai\pathfinding_navigation_graph.h"
#include "editor_manager.h"
#include "inputmanager.h"
#include "camera.h"
#include "entity/world.h"
#include "graphics\debugrenderer.h"
#include "entity/cameracomponent.h"
#include <random>
#include "engine.h"
#include "core/file/contentmanager.h"

#undef max
#undef min

namespace solunar
{
	constexpr float _kMGLengthOfRoot = 0.5f;
	constexpr float _kMGLengthOfArrow = 0.25f;
	glm::vec3 _kMGSelectedNodeColor = glm::vec3(1.0f, 0.0f, 0.0f);



	EditorWindow_AINavigationBuilder::EditorWindow_AINavigationBuilder(void) : IEditorWindow(), m_show(false), m_current_type(eNavigationType::kNavigationManualGraph)
	{
		std::memset(&this->m_conf_mg, 1, sizeof(unsigned char) * 3 + sizeof(bool));
	}

	EditorWindow_AINavigationBuilder::~EditorWindow_AINavigationBuilder(void)
	{
		for (auto& pair : this->m_conf_mg.nodes)
		{
			for (auto* pNode : pair.second)
			{
				delete pNode;
			}

			pair.second.clear();
		}
	}

	void EditorWindow_AINavigationBuilder::Init(void)
	{
		this->m_current_type = eNavigationType::kNavigationManualGraph;

		this->InitConfig_ManualGraph(this->m_conf_mg);
	}

	void EditorWindow_AINavigationBuilder::Draw(World* pWorld)
	{
		if (ImGui::Begin("[AI] - Navigation Builder"))
		{
			bool is_editing = g_editorManager->IsAINavigationEditingEnabled();
			ImGui::Checkbox("Graph Editing", &is_editing);
			g_editorManager->SetAINavigationEditingEnabled(is_editing);

			ImGui::SameLine();
			if (ImGui::Button("Build##AINavigationBuilder"))
			{
				this->Compile();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				this->ClearNodes();
			}
			ImGui::Separator();

			const char* combobox_navtypes_labels[eNavigationType::kEndOfEnum];

			for (unsigned char i = 0; i < static_cast<unsigned char>(eNavigationType::kEndOfEnum); ++i)
			{
				combobox_navtypes_labels[i] = convert_enum_navtype_to_text(static_cast<eNavigationType>(i));
			}

			int selected_navtype = static_cast<int>(this->m_current_type);
			ImGui::Combo("Navigation type", &selected_navtype, combobox_navtypes_labels, static_cast<int>(eNavigationType::kEndOfEnum));
			ImGui::Separator();

			switch (this->m_current_type)
			{
			case eNavigationType::kNavigationManualGraph:
			{
				constexpr unsigned char _kMaxSizeOfUnsignedChar = std::numeric_limits<unsigned char>::max();
				constexpr unsigned char _kMinSizeOfUnsignedChar = 1;

				bool controls_was_changed = false;
				controls_was_changed = ImGui::SliderScalar("max nodes per region", ImGuiDataType_U8, &m_conf_mg.max_nodes_count, &_kMinSizeOfUnsignedChar, &_kMaxSizeOfUnsignedChar);

				if (controls_was_changed)
				{
					this->m_conf_mg.need_to_update_stats = true;
				}


				controls_was_changed = ImGui::SliderScalar("max neighbours per node", ImGuiDataType_U8, &m_conf_mg.max_nodes_neighbour_per_node_count, &_kMinSizeOfUnsignedChar, &_kMaxSizeOfUnsignedChar);

				if (controls_was_changed)
				{
					this->m_conf_mg.need_to_update_stats = true;
				}

				controls_was_changed = ImGui::SliderScalar("max regions", ImGuiDataType_U8, &m_conf_mg.max_regions_count, &_kMinSizeOfUnsignedChar, &_kMaxSizeOfUnsignedChar);

				if (controls_was_changed)
				{
					this->m_conf_mg.need_to_update_stats = true;
				}

				if (this->m_conf_mg.need_to_update_stats)
				{
					UpdateStats(this->m_conf_mg);

					this->m_conf_mg.need_to_update_stats = false;
				}


				ImGui::Text("Statistics:");

				ImGui::Text("\tMax possible nodes: %d", this->m_conf_mg.max_possible_total_nodes);
				ImGui::Text("\tCreated nodes: %d", this->m_conf_mg.total_nodes);
				ImGui::Text("\tMemory be used: ");

				ImGui::Text("\t\treal: %d bytes (%d Kb)", 0, 0);
				ImGui::Text("\t\testimated: %d bytes (%d Kb)", this->m_conf_mg.memory_allocation_after_compilation, this->m_conf_mg.memory_allocation_after_compilation / 1024);

				if (ImGui::CollapsingHeader("Editing"))
				{
					if (ImGui::Checkbox("Selecting Mode", &this->m_conf_mg.is_selecting_node_mode))
					{
						if (!this->m_conf_mg.is_selecting_node_mode)
						{
							if (this->m_conf_mg.pHoveredNode)
							{
								this->m_conf_mg.pHoveredNode->hovered = false;
								this->m_conf_mg.pHoveredNode = nullptr;
							}

							if (this->m_conf_mg.pSelectedNode)
							{
								this->m_conf_mg.pSelectedNode->selected = false;
								this->m_conf_mg.pSelectedNode = nullptr;
							}
						}
					}

					if (this->m_conf_mg.is_selecting_node_mode)
					{
						if (this->m_conf_mg.pHoveredNode)
						{
							ImGui::Text("Hovered Node:");

							ImGui::Text("\tid: %d", this->m_conf_mg.pHoveredNode->id);
							ImGui::Text("\tregion id: %d", this->m_conf_mg.pHoveredNode->region_id);
						}


						if (this->m_conf_mg.pSelectedNode)
						{
							ImGui::Text("Selected Node:");

							ImGui::Text("\tid: %d", this->m_conf_mg.pSelectedNode->id);
							ImGui::Text("\tregion id: %d", this->m_conf_mg.pSelectedNode->region_id);
						}
					}

					ImGui::Text("Node Creation:");

					ImGui::Checkbox("One-direction (Selected->Created)", &this->m_conf_mg.node_creation_flag_onedirectional);

					if (this->m_conf_mg.node_creation_flag_onedirectional)
						this->m_conf_mg.node_creation_flag_bidirectional = false;

					ImGui::Checkbox("Bi-direction (Selected<->Created)", &this->m_conf_mg.node_creation_flag_bidirectional);

					if (this->m_conf_mg.node_creation_flag_bidirectional)
						this->m_conf_mg.node_creation_flag_onedirectional = false;


					ImGui::Text("Linking:");

					ImGui::Checkbox("One-direction", &this->m_conf_mg.is_linking_node_onedirectional);
					ImGui::Checkbox("Bi-direction", &this->m_conf_mg.is_linking_node_bidirectional);
				}

				if (ImGui::CollapsingHeader("Details"))
				{
					for (const auto& pair : this->m_conf_mg.nodes)
					{
						ImGui::Text("\t\tRegion: %d", pair.first);
						ImGui::Text("\t\t\tNodes: %d", pair.second.size());

						char node_name[32];
						char button_move_to_name[16];
						char region_name[16];

						sprintf(region_name, sizeof(region_name), "Region - %d", pair.first);
						if (ImGui::CollapsingHeader(region_name))
						{
							for (const auto* pNode : pair.second)
							{
								sprintf(node_name, sizeof(node_name), "Node: %d (reg:%d)", pNode->id, pair.first);
								ImGui::Text(node_name);


								sprintf(button_move_to_name, sizeof(button_move_to_name), "Move To##%d", pNode->id);
								if (ImGui::Button(button_move_to_name))
								{
									// strage behaviour setting world pos but it is not correct position like it is expected 1:1
									CameraProxy::GetInstance()->GetCameraComponent()->GetEntity()->SetPosition(pNode->position);
								}

								ImGui::Separator();
							}
						}
					}
				}


				break;
			}
			case eNavigationType::kNavigationAutoGrid:
			{
				Core::Msg("[WARNING] navigation type: [%s] not implemented", convert_enum_navtype_to_text(this->m_current_type));
				this->m_current_type = eNavigationType::kNavigationManualGraph;
				break;
			}
			case eNavigationType::kNavigationNavMesh:
			{
				Core::Msg("[WARNING] navigation type: [%s] not implemented", convert_enum_navtype_to_text(this->m_current_type));
				this->m_current_type = eNavigationType::kNavigationManualGraph;
				break;
			}
			default:
			{
				Assert(false && "unreachable code");
				break;
			}
			}
		}

		ImGui::End();


		this->DrawDebug();
	}

	const char* EditorWindow_AINavigationBuilder::GetName(void) const
	{
		return "AI Navigation Builder";
	}

	bool EditorWindow_AINavigationBuilder::IsShow(void) const
	{
		return m_show;
	}

	void EditorWindow_AINavigationBuilder::SetShow(bool status)
	{
		m_show = status;
	}

	int EditorWindow_AINavigationBuilder::GetEditingMode()
	{
		return EditingMode::kEditingMode_AIGraphNavigation;
	}

	void EditorWindow_AINavigationBuilder::UpdateEditingMode(InputManager* pInputManager, World* pWorld)
	{
		SHORT lbm_state = GetAsyncKeyState(VK_LBUTTON);

		this->UpdateSelectingNode();
		this->UpdateDeletingNode();
		this->UpdateLinkingNode();

		if (
			lbm_state & 0x01 &&
			!ImGui::IsAnyItemHovered() &&
			!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
			)
		{
			Camera* pCamera = CameraProxy::GetInstance();

			if (pCamera)
			{
				const glm::vec2& mouse_pos = pInputManager->GetCursorPos();

				POINT cursor;
				GetCursorPos(&cursor);
				glm::vec3 ray_dir = CameraProxy::GetInstance()->GetScreenRay(cursor.x, cursor.y);

				RayCastResult rc_result;

				glm::vec3 cam_pos = CameraProxy::GetInstance()->GetPosition();
				if (pWorld->RayCast(rc_result, cam_pos, cam_pos + ray_dir * 1000.0f))
				{
					this->AddNodeOnSurface(
						rc_result.m_hitPosition,
						rc_result.m_hitNormal
					);
				}
			}
		}
	}

	void EditorWindow_AINavigationBuilder::Compile()
	{
		auto& xml_doc = g_editorManager->GetWorldXML();

		auto* tag_world = xml_doc.FirstChildElement("World");

		if (tag_world)
		{
			auto* p_existed = tag_world->FirstChildElement(_kSerializationTag_AI);
			if (p_existed)
				tag_world->DeleteChild(p_existed);

			auto* tag_ai = tag_world->InsertNewChildElement(_kSerializationTag_AI);

			if (tag_ai)
			{
				auto* tag_backend = tag_ai->InsertNewChildElement(_kSerializationTag_AIBackend);

				if (tag_backend)
				{
					tag_backend->SetAttribute(_kSerializationAttribute_NavigationType, convert_enum_navtype_to_text(this->m_current_type));

					switch (this->m_current_type)
					{
					case eNavigationType::kNavigationManualGraph:
					{
						tag_backend->SetAttribute(_kSerializationAttribute_AIDataStorageTagName, _kSerializationTag_AIGraphData);
						tag_backend->SetAttribute(_kSerializationAttribute_MaxNodeCount, this->m_conf_mg.max_nodes_count);
						tag_backend->SetAttribute(_kSerializationAttribute_MaxRegionCount, this->m_conf_mg.max_regions_count);
						tag_backend->SetAttribute(_kSerializationAttribute_MaxNeighboursPerNode, this->m_conf_mg.max_nodes_neighbour_per_node_count);
						break;
					}
					default:
					{
						Assert(!"not implemented!");
						break;
					}
					}

					tag_backend->SetAttribute(_kSerializationAttribute_AISolver, "Astar");
				}

				switch (this->m_current_type)
				{
				case eNavigationType::kNavigationManualGraph:
				{
					auto* tag_ai_data = tag_ai->InsertNewChildElement(_kSerializationTag_AIGraphData);

					if (tag_ai_data)
					{
						auto* tag_graph_root = tag_ai_data->InsertNewChildElement(_kSerializationTag_Graph);

						if (tag_graph_root)
						{
							int total_children = 0;
							int regions_count = this->m_conf_mg.nodes.size();
							for (const auto& pair : this->m_conf_mg.nodes)
							{
								total_children += pair.second.size();
							}

							tag_graph_root->SetAttribute(_kSerializationAttribute_NodesCount, total_children);
							tag_graph_root->SetAttribute(_kSerializationAttribute_RegionsCount, regions_count);



							int abs_id_indexer = 0;
							// compiling abs ids
							for (auto& pair : this->m_conf_mg.nodes)
							{
								for (auto* pNode : pair.second)
								{
									pNode->abs_id = abs_id_indexer;
									++abs_id_indexer;
								}
							}

							for (const auto& pair : this->m_conf_mg.nodes)
							{
								for (const auto* pNode : pair.second)
								{
									auto* tag_node = tag_graph_root->InsertNewChildElement(_kSerializationTag_Node);

									if (tag_node)
									{
										constexpr unsigned char _kMaxUC = std::numeric_limits<unsigned char>::max();
										tag_node->SetAttribute(_kSerializationAttribute_id, pNode->id);
										tag_node->SetAttribute(_kSerializationAttribute_region_id, pNode->region_id);
										tag_node->SetAttribute(_kSerializationAttribute_abs_id, pNode->abs_id);
										tag_node->SetAttribute(_kSerializationAttribute_neighbours_count, pNode->neighbours.size());

										tag_node->SetAttribute(_kSerializationAttribute_pos_x, pNode->position.x);
										tag_node->SetAttribute(_kSerializationAttribute_pos_y, pNode->position.y);
										tag_node->SetAttribute(_kSerializationAttribute_pos_z, pNode->position.z);


										for (const auto* pNodeN : pNode->neighbours)
										{
											auto* tag_neighbour = tag_node->InsertNewChildElement(_kSerializationTag_NodeNeigbour);

											if (tag_neighbour)
											{
												tag_neighbour->SetAttribute(_kSerializationAttribute_id, pNodeN->id);
												tag_neighbour->SetAttribute(_kSerializationAttribute_region_id, pNodeN->region_id);
												tag_neighbour->SetAttribute(_kSerializationAttribute_abs_id, pNodeN->abs_id);
											}
										}

									}

								}
							}
						}
					}

					break;
				}
				default:
				{
					Assert(!"not implemeneted!");
					break;
				}
				}
			}


		}

		const char* pWorkingPath = g_contentManager->GetInstance()->GetWorkingPath("game");
		const std::string& srcPath = (pWorkingPath + std::string("/data/"));
		const std::string& fullPath = srcPath + EngineStateManager::GetInstance()->GetWorldName();
		auto status = xml_doc.SaveFile(fullPath.c_str());

		if (status == tinyxml2::XML_SUCCESS)
		{
			MessageBoxA(nullptr, "Successful!", "Compilation", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			MessageBoxA(nullptr, "Failed!", "Compilation", MB_OK | MB_ICONERROR);
		}

	}

	void EditorWindow_AINavigationBuilder::Load(tinyxml2::XMLElement& tagWorld)
	{
		if (strcmp(tagWorld.Name(), "World") != 0)
		{
			Assert(!"wrong document?!");

			Core::Error("FAILED to obtain data for AI map editor Can't load data");
			return;
		}

		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			for (auto& pair : this->m_conf_mg.nodes)
			{
				for (auto* pNode : pair.second)
				{
					delete pNode;
				}

				pair.second.clear();
			}

			this->m_conf_mg.nodes.clear();

			break;
		}
		default:
		{
			Assert(!"noot impleented!");
			break;
		}
		}

		auto* tag_ai = tagWorld.FirstChildElement(_kSerializationTag_AI);

		if (tag_ai)
		{
			auto* tag_ai_backend = tag_ai->FirstChildElement(_kSerializationTag_AIBackend);

			if (tag_ai_backend)
			{
				auto* attr_navigation_type = tag_ai_backend->FindAttribute(_kSerializationAttribute_NavigationType);

				if (attr_navigation_type)
				{
					this->m_current_type = convert_string_to_navtype(attr_navigation_type->Value());
				}

				switch (this->m_current_type)
				{
				case eNavigationType::kNavigationManualGraph:
				{
					auto* attr_max_nodes_count = tag_ai_backend->FindAttribute(_kSerializationAttribute_MaxNodeCount);

					if (attr_max_nodes_count)
					{
						this->m_conf_mg.max_nodes_count = attr_max_nodes_count->IntValue();
					}

					auto* attr_max_region_count = tag_ai_backend->FindAttribute(_kSerializationAttribute_MaxRegionCount);

					if (attr_max_region_count)
					{
						this->m_conf_mg.max_regions_count = attr_max_region_count->IntValue();
					}

					auto* attr_max_neighbours_per_node = tag_ai_backend->FindAttribute(_kSerializationAttribute_MaxNeighboursPerNode);

					if (attr_max_neighbours_per_node)
					{
						this->m_conf_mg.max_nodes_neighbour_per_node_count = static_cast<unsigned char>(attr_max_neighbours_per_node->IntValue());
					}

					auto* attr_storage_tag = tag_ai_backend->FindAttribute(_kSerializationAttribute_AIDataStorageTagName);

					if (attr_storage_tag)
					{
						const char* pAIGraphStorageTagName = attr_storage_tag->Value();
						auto* tag_ai_graph_data = tag_ai->FirstChildElement(pAIGraphStorageTagName);

						if (tag_ai_graph_data)
						{
							auto* tag_graph = tag_ai_graph_data->FirstChildElement(_kSerializationTag_Graph);

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
										auto* attr_id = iter->FindAttribute(_kSerializationAttribute_id);
										auto* attr_region_id = iter->FindAttribute(_kSerializationAttribute_region_id);
										// no need because in editor we recalculate abs_id field
										// auto* attr_abs_id = iter->FindAttribute(_kSerializationAttribute_abs_id);
										auto* attr_pos_x = iter->FindAttribute(_kSerializationAttribute_pos_x);
										auto* attr_pos_y = iter->FindAttribute(_kSerializationAttribute_pos_y);
										auto* attr_pos_z = iter->FindAttribute(_kSerializationAttribute_pos_z);

										if (attr_id && attr_region_id && attr_pos_x && attr_pos_y && attr_pos_z)
										{
											unsigned char id = static_cast<unsigned char>(attr_id->IntValue());
											unsigned char region_id = static_cast<unsigned char>(attr_region_id->IntValue());

											float pos_x = attr_pos_x->FloatValue();
											float pos_y = attr_pos_y->FloatValue();
											float pos_z = attr_pos_z->FloatValue();

											BuilderConfig_ManualGraph::Node* pInstance = new BuilderConfig_ManualGraph::Node();

											pInstance->id = id;
											pInstance->region_id = region_id;

											pInstance->position.x = pos_x;
											pInstance->position.y = pos_y;
											pInstance->position.z = pos_z;

											this->m_conf_mg.nodes[region_id].push_back(pInstance);
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
										auto* attr_id = iter->FindAttribute(_kSerializationAttribute_id);
										auto* attr_region_id = iter->FindAttribute(_kSerializationAttribute_region_id);

										unsigned char id = static_cast<unsigned char>(attr_id->IntValue());
										unsigned char region_id = static_cast<unsigned char>(attr_region_id->IntValue());

										auto node = std::find_if(this->m_conf_mg.nodes[region_id].begin(), this->m_conf_mg.nodes[region_id].end(), [id](const BuilderConfig_ManualGraph::Node* pElement) -> bool {
											Assert(pElement && "can't be!");
											return pElement->id == id;
										});

										Assert(node != this->m_conf_mg.nodes[region_id].end() && "lol it is broken! Can't be!!");

										auto* attr_neighbour_count = iter->FindAttribute(_kSerializationAttribute_neighbours_count);

										if (attr_neighbour_count)
										{
											unsigned char nc = static_cast<unsigned char>(attr_neighbour_count->IntValue());

											auto* iter_n = iter->FirstChildElement();

											if (iter_n)
											{
												for (unsigned char j = 0; j < nc; ++j)
												{
													auto* attr_neighbour_id = iter_n->FindAttribute(_kSerializationAttribute_id);
													auto* attr_neighbour_region_id = iter_n->FindAttribute(_kSerializationAttribute_region_id);
													
													if (attr_neighbour_id && attr_neighbour_region_id)
													{
														unsigned char neighbour_node_id = static_cast<unsigned char>(attr_neighbour_id->IntValue());
														unsigned char neighbour_region_id = static_cast<unsigned char>(attr_neighbour_region_id->IntValue());

														auto neighbour_node = std::find_if(this->m_conf_mg.nodes[neighbour_region_id].begin(), this->m_conf_mg.nodes[neighbour_region_id].end(), [neighbour_node_id](const BuilderConfig_ManualGraph::Node* pNode) -> bool {
															Assert(pNode && "can't be!");
															return pNode->id == neighbour_node_id;
													});

														Assert(neighbour_node != this->m_conf_mg.nodes[neighbour_region_id].end() && "can't be!");

														if (neighbour_node != this->m_conf_mg.nodes[neighbour_region_id].end())
														{
															(*node)->neighbours.push_back(*neighbour_node);
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
								}
								else
								{
									Core::Warning("FAILED to obtain attribute %s Can't load data", _kSerializationAttribute_NodesCount);
								}
							}
							else
							{
								Core::Warning("FAILED to obtain tag %s Can't load data", _kSerializationTag_Graph);
							}

						}
					}
					else
					{
						Core::Warning("FAILED can't obtain data about graph because attribute AIDataStorageTagName is missing! Can't load data");
					}

					break;
				}
				default:
				{
					Assert(!"not implemented");

					Core::Warning("Other navigation types not supported yet. Can't load data!");

					return;
				}
				}
			}
			else
			{
				Core::Warning("FAILED to obtain ai backend tag! Can't load data");
			}
		}
		else
		{
			Core::Warning("FAILED to obtain AI tag! Can't load data");
		}


		this->m_conf_mg.need_to_update_stats = true;
		this->UpdateStats(this->m_conf_mg);
	}

	void EditorWindow_AINavigationBuilder::UpdateStats(BuilderConfig_ManualGraph& conf)
	{
		if (conf.need_to_update_stats)
		{
			conf.max_possible_total_nodes = conf.max_nodes_count * conf.max_regions_count;
			conf.memory_allocation_after_compilation = 0;
			conf.total_nodes = 0;

			unsigned char max_neighbours_count = 0;
			for (const auto& pair : conf.nodes)
			{
				for (const BuilderConfig_ManualGraph::Node* pNode : pair.second)
				{
					size_t neighbours_count = pNode->neighbours.size();
					if (neighbours_count > max_neighbours_count)
					{
						max_neighbours_count = static_cast<unsigned char>(neighbours_count);
					}
				}

				this->m_conf_mg.total_nodes += pair.second.size();
			}

			conf.real_max_neighbours_count_for_one_node = max_neighbours_count;

			// NODES SECTION 

			conf.memory_allocation_after_compilation += conf.max_possible_total_nodes * sizeof(PathfindingNavigationStaticCachedGraphNode);

			// NEIGHBOURS SECTION

			conf.memory_allocation_after_compilation += conf.max_possible_total_nodes * conf.real_max_neighbours_count_for_one_node;

			// REGION CENTERS

			conf.memory_allocation_after_compilation += conf.max_regions_count * sizeof(float) * 3;

			// NODES POSITIONS

			conf.memory_allocation_after_compilation += conf.max_possible_total_nodes * sizeof(float) * 3;

		}
	}

	void EditorWindow_AINavigationBuilder::AddNodeOnSurface(const glm::vec3& world_pos, const glm::vec3& normal)
	{
		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			if (this->m_conf_mg.is_selecting_node_mode)
				break;

#ifdef _DEBUG
			char buf[64];
			sprintf(buf, sizeof(buf), "Add node: %.2f %.2f %.2f Normal: %.2f %.2f %.2f \n",
				world_pos.x,
				world_pos.y,
				world_pos.z,
				normal.x,
				normal.y,
				normal.z
			);
			OutputDebugStringA(buf);
#endif

			if (this->m_conf_mg.pSelectedNode)
			{
				Assert(this->m_conf_mg.nodes.find(this->m_conf_mg.pSelectedNode->region_id) != this->m_conf_mg.nodes.end() && "can't be because for selecting node you had to add node to storage!!! Something is broken");

				if (
					this->m_conf_mg.nodes[this->m_conf_mg.pSelectedNode->region_id].size() < this->m_conf_mg.max_nodes_count)
				{
					// todo: heuristic is not so optimal but gurantees that id is unique
					bool free_ids[std::numeric_limits<unsigned char>::max()];
					memset(free_ids, 0, sizeof(free_ids));

					for (const auto* pNode : this->m_conf_mg.nodes[this->m_conf_mg.pSelectedNode->region_id])
					{
						free_ids[pNode->id] = true;
					}

					constexpr unsigned char _kMAX = std::numeric_limits<unsigned char>::max();
					unsigned char free_id = -1;
					for (unsigned char i = 0; i < _kMAX; ++i)
					{
						if (!free_ids[i])
						{
							free_id = i;
							break;
						}
					}

					Assert(free_id != unsigned char(-1) && "failed to obtain unique id, means no free space left!!!!!!!! You should've handle it...");

					BuilderConfig_ManualGraph::Node* instance = new BuilderConfig_ManualGraph::Node();
					instance->id = free_id;
					instance->position = world_pos;
					instance->region_id = this->m_conf_mg.pSelectedNode->region_id;
					instance->abs_id = -1;
					this->m_conf_mg.nodes[this->m_conf_mg.pSelectedNode->region_id].push_back(instance);
				}
			}
			else
			{
				unsigned char free_region = unsigned char(-1);

				for (unsigned char i = 0; i < 255; ++i)
				{
					if (i > this->m_conf_mg.max_regions_count)
						break;

					if (this->m_conf_mg.nodes.find(i) == this->m_conf_mg.nodes.end())
					{
						if (i < this->m_conf_mg.max_regions_count)
						{
							free_region = i;
							break;
						}
					}
					else
					{
						if (this->m_conf_mg.nodes[i].size() < this->m_conf_mg.max_nodes_count)
						{
							free_region = i;
							break;
						}
					}
				}

				if (free_region != unsigned char(-1))
				{

					bool free_ids[std::numeric_limits<unsigned char>::max()];
					memset(free_ids, 0, sizeof(free_ids));

					for (const auto* pNode : this->m_conf_mg.nodes[free_region])
					{
						free_ids[pNode->id] = true;
					}

					constexpr unsigned char _kMAX = std::numeric_limits<unsigned char>::max();
					unsigned char free_id = -1;
					for (unsigned char i = 0; i < _kMAX; ++i)
					{
						if (!free_ids[i])
						{
							free_id = i; break;
						}
					}

					Assert(free_id != unsigned char(-1) && "failed to obtain unique id, means no free space left!!!!!!!! You should've handle it...");

					BuilderConfig_ManualGraph::Node* instance = new BuilderConfig_ManualGraph::Node();
					instance->id = free_id;
					instance->position = world_pos;
					instance->region_id = free_region;
					instance->abs_id = -1;
					this->m_conf_mg.nodes[free_region].push_back(instance);
				}
			}



			this->m_conf_mg.need_to_update_stats = true;


			break;
		}
		default:
		{
			Assert(!"not implemented!");
			break;
		}
		}
	}

	void EditorWindow_AINavigationBuilder::UpdateCentersOfRegions()
	{

	}

	void EditorWindow_AINavigationBuilder::ClearNodes()
	{
		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			this->m_conf_mg.nodes.clear();
			this->m_conf_mg.need_to_update_stats = true;

#ifdef _DEBUG
			OutputDebugStringA("Cleared nodes!");
#endif

			break;
		}
		default:
		{
			Assert(!"Not implemented!");
			break;
		}
		}
	}

	void EditorWindow_AINavigationBuilder::DrawDebug()
	{
		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			for (const auto& region_and_nodes : this->m_conf_mg.nodes)
			{
				const std::vector<BuilderConfig_ManualGraph::Node*>& nodes = region_and_nodes.second;

				for (const auto* pNode : nodes)
				{
					this->DrawDebugNode(region_and_nodes.first, pNode);
					this->DrawDebugNodeConnections(region_and_nodes.first, pNode);
				}
			}

			break;
		}
		default:
		{
			Assert(!"not implemented");
			break;
		}
		}
	}

	void EditorWindow_AINavigationBuilder::DrawDebugNode(
		unsigned char region_id,
		const BuilderConfig_ManualGraph::Node* pNode
	)
	{
		glm::vec3 color_for_region = this->m_conf_mg.debug_region_colors[region_id];

		if (pNode->hovered)
			color_for_region = glm::vec3(1.0f, 1.0f, 1.0f) - color_for_region;

		if (pNode->selected)
			color_for_region = _kMGSelectedNodeColor;

		g_debugRender.DrawLine(pNode->position, pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), color_for_region);
		g_debugRender.DrawLine(pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), pNode->position + glm::vec3(_kMGLengthOfArrow, 0.0f, 0.0f), color_for_region);
		g_debugRender.DrawLine(pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), pNode->position + glm::vec3(-_kMGLengthOfArrow, 0.0f, 0.0f), color_for_region);
		g_debugRender.DrawLine(pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), pNode->position + glm::vec3(0.0f, 0.0f, _kMGLengthOfArrow), color_for_region);
		g_debugRender.DrawLine(pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot, 0.0f), pNode->position + glm::vec3(0.0f, 0.0f, -_kMGLengthOfArrow), color_for_region);

	}

	void EditorWindow_AINavigationBuilder::DrawDebugNodeConnections(unsigned char region_id, const BuilderConfig_ManualGraph::Node* pNode)
	{
		for (const auto* pNodeNeighbour : pNode->neighbours)
		{
			glm::vec3 color_connection = (this->m_conf_mg.debug_region_colors[pNodeNeighbour->region_id] + this->m_conf_mg.debug_region_colors[pNode->region_id]) * 0.5f;

			glm::vec3 from_me_to_neigbour_dir = (pNodeNeighbour->position - pNode->position);
			from_me_to_neigbour_dir = glm::normalize(from_me_to_neigbour_dir);

			g_debugRender.DrawLine(pNode->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (from_me_to_neigbour_dir * _kMGLengthOfArrow), pNodeNeighbour->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), color_connection);

			glm::vec3 right = (pNodeNeighbour->position - pNode->position);
			right = glm::cross(right, glm::vec3(0.0, 1.0, 0.0));
			right = glm::normalize(right);
			g_debugRender.DrawLine(pNodeNeighbour->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), (pNodeNeighbour->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (right * _kMGLengthOfArrow)) - (glm::normalize((pNodeNeighbour->position - pNode->position)) * 0.6f), this->m_conf_mg.debug_region_colors[pNodeNeighbour->region_id]);
			g_debugRender.DrawLine(pNodeNeighbour->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) - (from_me_to_neigbour_dir * _kMGLengthOfArrow), (pNodeNeighbour->position + glm::vec3(0.0f, _kMGLengthOfRoot * 0.5f, 0.0f) + (right * -_kMGLengthOfArrow)) - (glm::normalize((pNodeNeighbour->position - pNode->position)) * 0.6f), this->m_conf_mg.debug_region_colors[pNodeNeighbour->region_id]);
		}
	}

	void EditorWindow_AINavigationBuilder::InitConfig_ManualGraph(BuilderConfig_ManualGraph& config)
	{
		config.max_nodes_count = 32;
		config.max_regions_count = 8;
		config.max_nodes_neighbour_per_node_count = 4;
		config.max_possible_total_nodes = config.max_nodes_count * config.max_regions_count;
		config.need_to_update_stats = false;
		config.total_nodes = 0;
		config.memory_allocation_after_compilation = 0;

		config.node_collision_radius = _kMGLengthOfArrow;

		config.node_creation_flag_bidirectional = false;
		config.node_creation_flag_onedirectional = false;
		config.is_selecting_node_mode = false;
		config.is_linking_node_bidirectional = false;
		config.is_linking_node_onedirectional = false;
		config.pHoveredNode = nullptr;
		config.pSelectedNode = nullptr;
		config.pLinkLeft = nullptr;
		config.pLinkRight = nullptr;

		for (int i = 0; i < 255; ++i)
		{
			config.debug_region_colors[i].x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			config.debug_region_colors[i].y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			config.debug_region_colors[i].z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

			if (config.debug_region_colors[i].x > 0.5f)
			{
				config.debug_region_colors[i].x *= 0.5f;
			}
		}
	}

	void EditorWindow_AINavigationBuilder::UpdateSelectingNode()
	{
		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			if (this->m_conf_mg.is_selecting_node_mode)
			{
				if (this->isRightMouseButtonPressed() || (ImGui::IsAnyItemHovered() ||
					ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)))
					return;

				POINT cursor;
				GetCursorPos(&cursor);
				glm::vec3 ray_dir = CameraProxy::GetInstance()->GetScreenRay(cursor.x, cursor.y);
				glm::vec3 ray_begin = CameraProxy::GetInstance()->GetPosition();

				float t_min = std::numeric_limits<float>::max();
				BuilderConfig_ManualGraph::Node* pClosestNode = nullptr;
				for (auto& regionid_and_nodes : this->m_conf_mg.nodes)
				{
					auto& nodes = regionid_and_nodes.second;

					for (auto* pNode : nodes)
					{
						float t;
						glm::vec3 point_of_intersection;
						if (this->RayIntersectionSphereNode(
							ray_begin,
							ray_dir,
							pNode->position,
							this->m_conf_mg.node_collision_radius,
							t,
							point_of_intersection
						))
						{
							if (t < t_min)
							{
								pClosestNode = pNode;
								t_min = t;
							}
						}
						else
						{
							if (pNode->hovered)
							{
								pNode->hovered = false;

								if (pNode->id == this->m_conf_mg.pHoveredNode->id && pNode->region_id == this->m_conf_mg.pHoveredNode->region_id)
									this->m_conf_mg.pHoveredNode = nullptr;
							}
						}
					}
				}

				if (pClosestNode)
				{
					if (this->m_conf_mg.pHoveredNode)
						this->m_conf_mg.pHoveredNode->hovered = false;

					this->m_conf_mg.pHoveredNode = pClosestNode;
					pClosestNode->hovered = true;
				}

				bool was_pressed_lbm = isLeftMouseButtonPressed();

				if (was_pressed_lbm)
				{
					if (!this->m_conf_mg.pHoveredNode)
					{
						if (this->m_conf_mg.pSelectedNode)
						{
							this->m_conf_mg.pSelectedNode->selected = false;
							this->m_conf_mg.pSelectedNode = nullptr;
						}
					}
					else
					{
						if (this->m_conf_mg.pSelectedNode)
						{
							this->m_conf_mg.pSelectedNode->selected = false;
							this->m_conf_mg.pSelectedNode = nullptr;
						}

						this->m_conf_mg.pHoveredNode->hovered = false;
						this->m_conf_mg.pSelectedNode = this->m_conf_mg.pHoveredNode;
						this->m_conf_mg.pHoveredNode = nullptr;
						this->m_conf_mg.pSelectedNode->selected = true;
					}
				}
			}

			break;
		}
		default:
		{
			Assert(!"not implemented!");
			break;
		}
		}
	}

	void EditorWindow_AINavigationBuilder::UpdateDeletingNode()
	{
		if (this->m_conf_mg.pSelectedNode)
		{
			bool was_pressed_delete = isDeleteKeyButtonPressed();

			if (was_pressed_delete)
			{
				if (this->m_conf_mg.nodes.find(this->m_conf_mg.pSelectedNode->region_id) != this->m_conf_mg.nodes.end())
				{
					unsigned char id = this->m_conf_mg.pSelectedNode->id;
					unsigned char region_id = this->m_conf_mg.pSelectedNode->region_id;
					auto& nodes = this->m_conf_mg.nodes[this->m_conf_mg.pSelectedNode->region_id];

					for (auto& pair : this->m_conf_mg.nodes)
					{
						for (auto* pNode : pair.second)
						{
							auto iter = std::find_if(pNode->neighbours.begin(), pNode->neighbours.end(), [id, region_id](const BuilderConfig_ManualGraph::Node* pNode)->bool {
								return pNode->id == id && pNode->region_id == region_id;
								});

							if (iter != pNode->neighbours.end())
							{
								pNode->neighbours.erase(iter);
							}

						}
					}

					auto iter = std::find_if(nodes.begin(), nodes.end(), [id](const BuilderConfig_ManualGraph::Node* pNode) -> bool {
						return pNode->id == id;
						});

					Assert(iter != nodes.end() && "something is wrong, element must persist in vector");

					if (iter != nodes.end())
					{
						delete (*iter);
						nodes.erase(iter);
					}

					this->m_conf_mg.pSelectedNode = nullptr;

					this->m_conf_mg.need_to_update_stats = true;
				}
			}
		}
	}

	void EditorWindow_AINavigationBuilder::UpdateLinkingNode()
	{
		switch (this->m_current_type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			if (this->m_conf_mg.is_selecting_node_mode)
			{
				if (isLAltKeyButtonPressed() && isLCtrlKeyButtonPressed())
				{
					this->m_conf_mg.is_linking_node_bidirectional = false;
					this->m_conf_mg.is_linking_node_onedirectional = false;
					return;
				}

				if (!this->m_conf_mg.pHoveredNode)
					return;

				if (!this->m_conf_mg.pSelectedNode)
					return;

				if (isLCtrlKeyButtonPressed())
				{
					this->m_conf_mg.is_linking_node_bidirectional = true;
					this->m_conf_mg.is_linking_node_onedirectional = true;
				}

				if (isLAltKeyButtonPressed())
				{
					this->m_conf_mg.is_linking_node_bidirectional = false;
					this->m_conf_mg.is_linking_node_onedirectional = true;
				}


				// adds HoveredNode to SelectedNode
				if (this->m_conf_mg.is_linking_node_onedirectional)
				{
					if (this->m_conf_mg.pHoveredNode && this->m_conf_mg.pSelectedNode)
					{
						if ((this->m_conf_mg.pHoveredNode->region_id == this->m_conf_mg.pSelectedNode->region_id && this->m_conf_mg.pHoveredNode->id != this->m_conf_mg.pSelectedNode->id) || (this->m_conf_mg.pHoveredNode->region_id != this->m_conf_mg.pSelectedNode->region_id))
						{
							unsigned char region_id = this->m_conf_mg.pHoveredNode->region_id;
							unsigned char id = this->m_conf_mg.pHoveredNode->id;

							auto iter = std::find_if(
								this->m_conf_mg.pSelectedNode->neighbours.begin(),
								this->m_conf_mg.pSelectedNode->neighbours.end(),
								[id, region_id](const BuilderConfig_ManualGraph::Node* pNode)->bool {
									return pNode->id == id && pNode->region_id == region_id;
								});

							// add only if wasn't added
							if (iter == this->m_conf_mg.pSelectedNode->neighbours.end())
							{
								this->m_conf_mg.pSelectedNode->neighbours.push_back(this->m_conf_mg.pHoveredNode);
							}
						}
					}
				}

				// adds SelectedNode to HoveredNode
				if (this->m_conf_mg.is_linking_node_bidirectional)
				{
					if (this->m_conf_mg.pHoveredNode && this->m_conf_mg.pSelectedNode)
					{
						if ((this->m_conf_mg.pHoveredNode->region_id == this->m_conf_mg.pSelectedNode->region_id && this->m_conf_mg.pHoveredNode->id != this->m_conf_mg.pSelectedNode->id) || (this->m_conf_mg.pHoveredNode->region_id != this->m_conf_mg.pSelectedNode->region_id))
						{
							unsigned char region_id = this->m_conf_mg.pSelectedNode->region_id;
							unsigned char id = this->m_conf_mg.pSelectedNode->id;

							auto iter = std::find_if(
								this->m_conf_mg.pHoveredNode->neighbours.begin(),
								this->m_conf_mg.pHoveredNode->neighbours.end(),
								[id, region_id](const BuilderConfig_ManualGraph::Node* pNode)->bool {
									return pNode->id == id && pNode->region_id == region_id;
								});

							// add only if wasn't added
							if (iter == this->m_conf_mg.pHoveredNode->neighbours.end())
							{
								this->m_conf_mg.pHoveredNode->neighbours.push_back(this->m_conf_mg.pSelectedNode);
							}
						}
					}
				}


				this->m_conf_mg.is_linking_node_bidirectional = false;
				this->m_conf_mg.is_linking_node_onedirectional = false;

			}

			break;
		}
		default:
		{
			Assert(!"Not implemented");
			break;
		}
		}
	}

	bool EditorWindow_AINavigationBuilder::RayIntersectionSphereNode(const glm::vec3& ray_origin, const glm::vec3& ray_dir, const glm::vec3& sphere_pos, float sphere_radius, float& t1, glm::vec3& point)
	{
		glm::vec3 m = ray_origin - sphere_pos;
		float b = glm::dot(m, ray_dir);
		float c = glm::dot(m, m) - sphere_radius * sphere_radius;

		// Exit if r’s origin outside s (c > 0) and r pointing away from s (b > 0) 
		if (c > 0.0f && b > 0.0f)
			return false;

		float discr = b * b - c;

		// A negative discriminant corresponds to ray missing sphere 
		if (discr < 0.0f) return 0;

		// Ray now found to intersect sphere, compute smallest t value of intersection
		float t = -b - sqrt(discr);

		// If t is negative, ray started inside sphere so clamp t to zero 
		if (t < 0.0f)
			t = 0.0f;

		point = ray_origin + t * ray_dir;
		t1 = t;
		return true;
	}

	bool EditorWindow_AINavigationBuilder::isLeftMouseButtonPressed()
	{
		auto button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
		return GetAsyncKeyState(button) < 0;
	}

	bool EditorWindow_AINavigationBuilder::isRightMouseButtonPressed()
	{
		auto button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON;
		return GetAsyncKeyState(button) < 0;
	}

	bool EditorWindow_AINavigationBuilder::isDeleteKeyButtonPressed()
	{
		return GetAsyncKeyState(VK_DELETE) < 0;
	}

	bool EditorWindow_AINavigationBuilder::isLAltKeyButtonPressed()
	{
		return GetAsyncKeyState(VK_LMENU) < 0;
	}

	bool EditorWindow_AINavigationBuilder::isLCtrlKeyButtonPressed()
	{
		return GetAsyncKeyState(VK_LCONTROL) < 0;
	}

	const char* convert_enum_navtype_to_text(eNavigationType type)
	{
		switch (type)
		{
		case eNavigationType::kNavigationManualGraph:
		{
			return _kManualGraphName;
		}
		case eNavigationType::kNavigationAutoGrid:
		{
			return _kAutoGridName;
		}
		case eNavigationType::kNavigationNavMesh:
		{
			return _kNavMeshName;
		}
		default:
			return "UNDEFINED_NAVIGATION_TYPE";
		}
	}
	eNavigationType convert_string_to_navtype(const char* text)
	{
		eNavigationType result = eNavigationType::kEndOfEnum;

		if (!strcmp(text, _kManualGraphName))
			result = eNavigationType::kNavigationManualGraph;
		else if (!strcmp(text, _kNavMeshName))
			result = eNavigationType::kNavigationNavMesh;
		else if (!strcmp(text, _kAutoGridName))
			result = eNavigationType::kNavigationAutoGrid;

		return result;
	}
}