#include "editor_window_level_inspector.h"
#include "imgui.h"
#include "graphics\light.h"
#include "../entity/world.h"
#include "editor_manager.h"

#include "engine/camera.h"
#include "engine/engine.h"
#include "graphics/mesh.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>

void CreateLight()
{
	using namespace solunar;

	Entity* entity = Engine::ms_world->CreateEntity();
	entity->CreateComponent<PointLightComponent>();
}

void CreateWall()
{
	using namespace solunar;

	Entity* entity = Engine::ms_world->CreateEntity();
	MeshComponent* mesh = entity->CreateComponent<MeshComponent>();
	mesh->LoadModel("models/common_wall_2x2.dae");
}

solunar::EditorWindow_LevelInspector::EditorWindow_LevelInspector() : m_show(false)
{
}

solunar::EditorWindow_LevelInspector::~EditorWindow_LevelInspector()
{
}

void solunar::EditorWindow_LevelInspector::Init(void)
{
}

inline glm::vec3 Glm_GetPositionFromMat4(const glm::mat4& m)
{
	glm::vec3 scale = glm::vec3(0.0f);
	glm::quat orientation = glm::identity<glm::quat>();
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 skew = glm::vec3(0.0f);
	glm::vec4 persp = glm::vec4(0.0f);
	glm::decompose(m, scale, orientation, position, skew, persp);
	return position;
}

void solunar::EditorWindow_LevelInspector::Draw(World* pWorld)
{
	Assert(pWorld && "must be valid!");
	Assert(g_editorManager && "must be valid!");

	if (ImGui::Begin("Level Inspector"))
	{
		if (ImGui::Button("Create Light")) CreateLight();

		/*
		static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		static bool align_label_with_current_x_position = false;
		static bool test_drag_and_drop = false;

		// todo: kirrik -> make commands and command queue for processing these
	//	if (ImGui::Button("Create Light")) CreateLight();
	//	if (ImGui::Button("Create Wall")) CreateWall();

		if (align_label_with_current_x_position)
			ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

		// 'selection_mask' is dumb representation of what may be user-side selection state.
		//  You may retain selection state inside or outside your objects in whatever format you see fit.
		// 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
		/// of the loop. May be a pointer to your own node type, etc.
		static int selection_mask = (1 << 2);
		int node_clicked = -1;
		const std::vector<Entity*>& entities = pWorld->GetEntityManager().GetEntities();

		for (int i = 0; i < entities.size(); i++)
		{
			Entity* entity = entities[i];
			// Disable the default "open on single-click behavior" + set Selected flag according to our selection.
			// To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
			ImGuiTreeNodeFlags node_flags = base_flags;
			const bool is_selected = (selection_mask & (1 << i)) != 0;
			if (is_selected)
				node_flags |= ImGuiTreeNodeFlags_Selected;

#if 1
			bool isLight = !!entity->GetComponent<LightComponent>();
			if (isLight)
				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, "Light 0x%p", entity);
			else
				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, "Entity 0x%p", entity);
#endif
			// Items 0..2 are Tree Node

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				node_clicked = i;
			if (test_drag_and_drop && ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
				ImGui::Text("This is a drag and drop source");
				ImGui::EndDragDropSource();
			}
		}

		if (node_clicked != -1)
		{
			
			//g_SelectedEntity = entities[node_clicked];
			
			g_editorManager->SetSelectedEntity(entities[node_clicked]);

			// Update selection state
			// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
			if (ImGui::GetIO().KeyCtrl)
				selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
			else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
				selection_mask = (1 << node_clicked);           // Click to single-select
		}
		if (align_label_with_current_x_position)
			ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		//	ImGui::TreePop();
		*/

		

		if (ImGui::BeginTabBar("TabBar##LevelInspector"))
		{
			if (ImGui::BeginTabItem("All"))
			{
				const std::vector<Entity*>& entities = pWorld->GetEntityManager().GetEntities();
				int node_clicked = -1;

				for (int i = 0; i < entities.size(); i++)
				{
					Entity* entity = entities[i];

					char node_name[64];
					sprintf(node_name, sizeof(node_name), "%s##%p", entity->GetTypeInfo()->GetClassName(), entity);

					if (ImGui::TreeNode(node_name))
					{
						const auto& components = entity->GetAllComponents();

						for (Component* pComponent : components)
						{
							if (pComponent)
							{
								char component_name[64];
								sprintf(component_name, sizeof(component_name), "%s##%p", pComponent->GetTypeInfo()->GetClassName(), pComponent);

								if (ImGui::TreeNode(component_name))
								{


									ImGui::TreePop();
								}
							}
						}

						ImGui::TreePop();
					}

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						node_clicked = i;
				}

				ImGui::EndTabItem();

				if (node_clicked != -1)
				{
					g_editorManager->SetSelectedEntity(entities[node_clicked]);
				}
			}

			if (ImGui::BeginTabItem("Light"))
			{
				const std::vector<Entity*>& entities = pWorld->GetEntityManager().GetEntities();

				for (int i = 0; i < entities.size(); i++)
				{
					Entity* entity = entities[i];


					LightComponent* pLight = entity->GetComponent<LightComponent>();

					if (pLight)
					{
						char node_name[64];
						sprintf(node_name, sizeof(node_name), "%s##%p", entity->GetTypeInfo()->GetClassName(), entity);

						if (ImGui::TreeNode(node_name))
						{
							const auto& components = entity->GetAllComponents();

							for (Component* pComponent : components)
							{
								if (pComponent)
								{
									char component_name[64];
									sprintf(component_name, sizeof(component_name), "%s##%p", pComponent->GetTypeInfo()->GetClassName(), pComponent);

									if (ImGui::TreeNode(component_name))
									{


										ImGui::TreePop();
									}
								}
							}

							ImGui::TreePop();
						}
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("AI"))
			{
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Sound"))
			{
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	ImGui::End();


	if (g_editorManager->GetSelectedEntity())
	{
		//g_debugRender.drawBoundingBox(g_SelectedEntity->GetBoundingBox(), glm::vec3(0.0f, 0.0f, 1.0f));

		Entity* g_SelectedEntity = (Entity*)g_editorManager->GetSelectedEntity();

		static glm::mat4 objectMatrix = g_SelectedEntity->GetWorldTranslation();

		View* view = CameraProxy::GetInstance()->GetView();

		glm::mat4 cameraView = view->m_view;
		glm::mat4 cameraProj = view->m_projection;

		static int g_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
		static int g_CurrentGizmoMode = ImGuizmo::WORLD;

		if (ImGuizmo::Manipulate(
			&cameraView[0][0],
			&cameraProj[0][0],
			(ImGuizmo::OPERATION)g_CurrentGizmoOperation,
			(ImGuizmo::MODE)g_CurrentGizmoMode,
			&objectMatrix[0][0]))
		{
			glm::vec3 pos = Glm_GetPositionFromMat4(objectMatrix);
			g_SelectedEntity->SetPosition(pos);

		}
	}
}

const char* solunar::EditorWindow_LevelInspector::GetName(void) const
{
	return "Level Inspector";
}

bool solunar::EditorWindow_LevelInspector::IsShow(void) const
{
	return m_show;
}

void solunar::EditorWindow_LevelInspector::SetShow(bool status)
{
	m_show = status;
}

int solunar::EditorWindow_LevelInspector::GetEditingMode()
{
	return -1;
}

void solunar::EditorWindow_LevelInspector::UpdateEditingMode(InputManager* pInputManager, World* pWorld)
{

}

void solunar::EditorWindow_LevelInspector::Load(tinyxml2::XMLElement& tagWorld)
{
}
