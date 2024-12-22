#include "editor_window_entity_editor.h"
#include "imgui.h"
#include "editor_manager.h"
#include "engine.h"
#include <graphics/light.h>

namespace solunar
{
	static void DrawProperties(const std::vector<IProperty*>& properties, Object* object)
	{
		for (int i = 0; i < properties.size(); i++)
		{
			IProperty* property = properties[i];
			if (property->GetType() == PropertyType_Float)
			{
				float value;
				PropertyGetValue(object, property, value);
				ImGui::DragFloat(property->GetName(), &value);
			}
			if (property->GetType() == PropertyType_Vector3)
			{
				glm::vec3 value;
				PropertyGetValue(object, property, value);
				ImGui::DragFloat3(property->GetName(), &value[0]);
			}
			if (property->GetType() == PropertyType_Vector4)
			{
				glm::vec4 value;
				PropertyGetValue(object, property, value);
				ImGui::DragFloat4(property->GetName(), &value[0]);
			}
			if (property->GetType() == PropertyType_Quaternion)
			{
				glm::quat value;
				PropertyGetValue(object, property, value);
				ImGui::DragFloat4(property->GetName(), &value[0]);
			}
			if (property->GetType() == PropertyType_String)
			{
				std::string value;
				PropertyGetValue(object, property, value);
				ImGui::InputText(property->GetName(), (char*)value.c_str(), value.size() + 1);
			}
			if (property->GetType() == PropertyType_Bool)
			{
				bool value;
				PropertyGetValue(object, property, value);
				ImGui::Checkbox(property->GetName(), &value);
			}
		}
	}

	static void DrawEntityPropertyWindow(Entity* entity)
	{
	//	if (ImGui::Begin("Entity Properties"))
		{
			if (entity)
			{
				ImGui::Text("%s", entity->GetTypeInfo()->GetClassName());
				std::vector<IProperty*> properties;
				PropertyManager::GetInstance()->GetProperties(entity->GetTypeInfo(), properties);
				DrawProperties(properties, entity);

				PointLightComponent* pl = entity->GetComponent<PointLightComponent>();
				if (pl)
				{
					ImGui::DragFloat("RADIUS", &pl->m_radius, 0.1f);
				}

				// components
				const std::vector<Component*>& components = entity->GetAllComponents();
				for (int i = 0; i < components.size(); i++)
				{
					ImGui::Separator();
					ImGui::Text("%s", components[i]->GetTypeInfo()->GetClassName());

					properties.clear();

					PropertyManager::GetInstance()->GetProperties(components[i]->GetTypeInfo(), properties);

					DrawProperties(properties, components[i]);
				}
			}
		}

//		ImGui::End();
	}


	EditorWindow_EntityEditor::EditorWindow_EntityEditor() : m_show(false), m_selected_type(0)
	{
	}

	EditorWindow_EntityEditor::~EditorWindow_EntityEditor()
	{
	}

	void EditorWindow_EntityEditor::Init(void)
	{
		const auto& types = TypeManager::GetInstance()->GetAllTypes();

		Assert(types.empty() == false && "must be registered already here!!!!");
	}

	void EditorWindow_EntityEditor::Draw(World* pWorld)
	{
#ifdef _DEBUG
		if (g_engineData.m_editor)
		{
			Assert(g_editorManager && "must be valid!");
		}
#endif

		if (ImGui::Begin("Entity editor##Editor"))
		{
			Entity* pSelectedEntity = static_cast<Entity*>(g_editorManager->GetSelectedEntity());
			
			if (pSelectedEntity)
			{
				if (ImGui::BeginTabBar("Entity##Entity editor"))
				{
					if (ImGui::BeginTabItem("Components"))
					{

					//	ImGui::Combo("##EntityEditorComponents");

						if (ImGui::Button("Add component"))
						{
						}

						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
			
				DrawEntityPropertyWindow(pSelectedEntity);
			}
		}

		ImGui::End();
	}

	const char* EditorWindow_EntityEditor::GetName(void) const
	{
		return "Entity editor";
	}

	bool EditorWindow_EntityEditor::IsShow(void) const
	{
		return m_show;
	}

	void EditorWindow_EntityEditor::SetShow(bool status)
	{
		m_show = status;
	}

	int EditorWindow_EntityEditor::GetEditingMode()
	{
		return EditingMode::kEditingMode_ObjectSelection;
	}

	void EditorWindow_EntityEditor::UpdateEditingMode(InputManager* pInputManager, World* pWorld)
	{

	}
	void EditorWindow_EntityEditor::Load(tinyxml2::XMLElement& tagWorld)
	{
	}
}