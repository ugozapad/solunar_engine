#include "editor_manager.h"
#include "editor_window.h"
#include "inputmanager.h"
#include "engine.h"
#include "camera.h"
#include "entity\cameracomponent.h"
#include "imgui.h"

namespace solunar
{
	EditorManager* g_editorManager = nullptr;

	float wrapBetween2(float value, float min, float max) {
		// Algorithm from http://stackoverflow.com/a/5852628/599884

		if (min > max) {
			// Swap min and max
			float temp = min;
			min = max;
			max = temp;
		}

		float range = max - min;
		if (range == 0) {
			return max;
		}

		return (float)(value - range * floor((value - min) / range));
	}

	EditorManager::EditorManager() :
		m_object_selection_enabled(false),
		m_ai_navigation_editing_enabled(false),
		m_game_simulate(false),
		m_close_application(false),
		m_show_modal_close_application_window(false),
		m_current_editing_mode(EditingMode::kEditingMode_NoSelection),
		m_pSelectedEntity(nullptr),
		m_pWorld(nullptr),
		m_pEditingMode_AINavigationGraph(nullptr),
		m_pEditingMode_ObjectSelection(nullptr),
		m_p_allocated_memory(nullptr),
		m_length_of_file(0),
		m_cam{}
	{
	}

	EditorManager::~EditorManager()
	{
		if (!m_windows.empty())
			Core::Msg("EditorManager::~EditorManager:: you forgot to clear (or even to delete data)!");

		Shutdown();
	}

	void EditorManager::Init(World* pWorld)
	{
		Assert(pWorld && "you must pass a valid world!");

		this->m_pWorld = pWorld;
	}

	void EditorManager::PostInit()
	{
		this->InitWindows();
		this->Load(*this->GetWorldXML().FirstChildElement());
	}

	void EditorManager::InitWindows()
	{
		for (IEditorWindow* pWindow : m_windows)
		{
			if (pWindow)
			{
				pWindow->Init();
			}
		}

		IEditorWindow* pWindow = GetWindowByEditingMode(EditingMode::kEditingMode_AIGraphNavigation);
		
		if (pWindow)
		{
			this->m_pEditingMode_AINavigationGraph = pWindow;
		}

		pWindow = GetWindowByEditingMode(EditingMode::kEditingMode_ObjectSelection);

		if (pWindow)
		{
			this->m_pEditingMode_ObjectSelection = pWindow;
		}
	}

	void EditorManager::UpdateEditingModes()
	{
		if (this->m_ai_navigation_editing_enabled == false && this->m_object_selection_enabled == false)
		{
			if (this->m_current_editing_mode != EditingMode::kEditingMode_NoSelection)
			{
				this->m_current_editing_mode = EditingMode::kEditingMode_NoSelection;
			}
		}

		switch (this->m_current_editing_mode)
		{
		case EditingMode::kEditingMode_AIGraphNavigation:
		{
			this->UpdateEditingMode_AIGraphNavigation();

			break;
		}
		case EditingMode::kEditingMode_ObjectSelection:
		{
			this->UpdateEditingMode_ObjectSelection();




			break;
		}
		case EditingMode::kEditingMode_NoSelection:
		{
			// nothing it means user doesn't have ANY editing mode as ENABLED (true)
			// but we have to ensure that other programmers didn't break the logic

			Assert(this->m_ai_navigation_editing_enabled == false && this->m_object_selection_enabled == false);

			break;
		}
		}
	}

	void EditorManager::UpdateEditingMode_AIGraphNavigation()
	{
		Assert(this->m_current_editing_mode == EditingMode::kEditingMode_AIGraphNavigation && "you can't call this method if your editing mode is different!");
		Assert(this->m_pEditingMode_AINavigationGraph && "you must obtain window at initialization stage");

		auto* pInputManager = InputManager::GetInstance();

		if (pInputManager)
		{
			if (this->m_pEditingMode_AINavigationGraph)
			{
				this->m_pEditingMode_AINavigationGraph->UpdateEditingMode(pInputManager, Engine::ms_world);
			}
		}
	}

	void EditorManager::UpdateEditingMode_ObjectSelection()
	{
		Assert(this->m_current_editing_mode == EditingMode::kEditingMode_ObjectSelection && "you can't call this method if your editing mode is different!");
		Assert(this->m_pEditingMode_ObjectSelection && "you must obtain window at initialization stage");

		auto* pInputManager = InputManager::GetInstance();

		if (pInputManager)
		{
			if (this->m_pEditingMode_ObjectSelection)
			{
				this->m_pEditingMode_ObjectSelection->UpdateEditingMode(pInputManager, Engine::ms_world);
			}
		}
	}

	void EditorManager::UpdateCamera()
	{
		if (!this->m_game_simulate)
		{
			CameraProxy* pCamera = CameraProxy::GetInstance();

			if (pCamera)
			{
				InputManager* pInputManager = InputManager::GetInstance();

				if (pInputManager)
				{
					if (pInputManager->IsPressed(KeyboardKeys::KEY_W))
					{
						glm::vec3 pos= pCamera->GetCameraComponent()->GetEntity()->GetPosition();
						pos += pCamera->GetDirection() * 0.02f;
						pCamera->GetCameraComponent()->GetEntity()->SetPosition(pos);
					}

					if (pInputManager->IsPressed(KeyboardKeys::KEY_S))
					{
						glm::vec3 pos = pCamera->GetCameraComponent()->GetEntity()->GetPosition();
						pos -= pCamera->GetDirection() * 0.02f;
						pCamera->GetCameraComponent()->GetEntity()->SetPosition(pos);
					}

					constexpr glm::vec3 _kYAxis = glm::vec3(0.0f, 1.0f, 0.0f);

					if (pInputManager->IsPressed(KeyboardKeys::KEY_D))
					{
						glm::vec3 pos = pCamera->GetCameraComponent()->GetEntity()->GetPosition();
						pos += glm::normalize(glm::cross(pCamera->GetDirection(), _kYAxis)) * 0.02f;
						pCamera->GetCameraComponent()->GetEntity()->SetPosition(pos);
					}

					if (pInputManager->IsPressed(KeyboardKeys::KEY_A))
					{
						glm::vec3 pos = pCamera->GetCameraComponent()->GetEntity()->GetPosition();
						pos -= glm::normalize(glm::cross(pCamera->GetDirection(), _kYAxis)) * 0.02f;
						pCamera->GetCameraComponent()->GetEntity()->SetPosition(pos);
					}

					if (pInputManager->IsMouseButtonPressed(MouseButtons::MOUSE_BUTTON_RIGHT))
					{
						g_engineData.m_shouldCaptureMouse = true;
						g_engineData.m_shouldHideMouse = true;

						glm::vec2 delta = pInputManager->GetDeltaCursorPos();

						float xoffset = delta.x;
						float yoffset = delta.y;

						xoffset *= 0.1f;
						yoffset *= 0.1f;

						m_cam.yaw += xoffset;
						m_cam.pitch += yoffset;

						m_cam.pitch = glm::clamp(m_cam.pitch, -89.0f, 89.0f);
						m_cam.yaw = wrapBetween2(m_cam.yaw, -180.0f, 180.0f);
						m_cam.pitch = wrapBetween2(m_cam.pitch, -180.0f, 180.0f);

						glm::vec3 front;
						front.x = cos(glm::radians(m_cam.yaw)) * cos(glm::radians(m_cam.pitch));
						front.y = sin(glm::radians(m_cam.pitch));
						front.z = sin(glm::radians(m_cam.yaw)) * cos(glm::radians(m_cam.pitch));

						m_cam.direction = glm::normalize(front);

						pCamera->GetCameraComponent()->SetDirection(m_cam.direction);
					}
					else
					{
						g_engineData.m_shouldCaptureMouse = false;
						g_engineData.m_shouldHideMouse = false;

						// todo: typo ne ok, because if you handle input by g_engineData thus
						// you have to debug your state and disable/enable because InputManager
						// it is not right to manually call it here if you comment last two lines of code
						// the cursor will hide and only alt/tab will reveive it and it is not okay :(
						InputManager::GetInstance()->SetCursorHiding(false);
						InputManager::GetInstance()->SetCursorCapture(false);
					}
				}
			}
		}
	}

	IEditorWindow* EditorManager::GetWindowByEditingMode(EditingMode mode)
	{
		IEditorWindow* pResult = 0;

		auto it = std::find_if(this->m_windows.begin(), this->m_windows.end(), [mode](IEditorWindow* pWindow) {
			if (!pWindow)
				return false;

			return static_cast<EditingMode>(pWindow->GetEditingMode()) == mode;
		});

		if (it != this->m_windows.end())
			pResult = *it;

		return pResult;
	}

	void EditorManager::ShowModalWindowOnCloseApplication()
	{
		if (!this->m_show_modal_close_application_window)
			return;

		bool show = this->m_show_modal_close_application_window;

		if (show)
		{
			ImGui::OpenPopup("Close Editor?");
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Close Editor?", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (ImGui::Button("Yes##ModalCloseEditor"))
			{
				this->m_close_application = true;
				this->m_show_modal_close_application_window = false;
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("No##ModalCloseEditor"))
			{
				this->m_close_application = false;
				this->m_show_modal_close_application_window = false;
				EngineStateManager::GetInstance()->RestoreState();
			}

			ImGui::EndPopup();
		}
	}

	void EditorManager::Shutdown()
	{
		for (IEditorWindow* pWindow : m_windows)
		{
			if (pWindow)
			{
				mem_delete(pWindow);
			}
		}


		if (this->m_p_allocated_memory)
		{
			delete[] this->m_p_allocated_memory;
			this->m_p_allocated_memory = nullptr;
		}

		m_windows.clear();
	}

	void EditorManager::Update()
	{
		this->ShowModalWindowOnCloseApplication();

		for (IEditorWindow* pWindow : m_windows)
		{
			if (pWindow)
			{
				if (pWindow->IsShow())
				{
					pWindow->Draw(this->m_pWorld);
				}
			}
		}

		this->UpdateEditingModes();

		this->UpdateCamera();
	}

	void EditorManager::RegisterWindow(IEditorWindow* pWindow)
	{
		Assert(pWindow && "you must pass a valid window pointer!");

		m_windows.push_back(pWindow);
	}

	void* EditorManager::GetSelectedEntity()
	{
		return m_pSelectedEntity;
	}

	void EditorManager::SetSelectedEntity(void* pEntity)
	{
		m_pSelectedEntity = pEntity;
	}

	const std::vector<IEditorWindow*>& EditorManager::GetWindows(void) const
	{
		return m_windows;
	}

	bool EditorManager::IsAINavigationEditingEnabled(void) const
	{
		return m_ai_navigation_editing_enabled;
	}

	void EditorManager::SetAINavigationEditingEnabled(bool value)
	{
		m_ai_navigation_editing_enabled = value;

		if (value)
			this->m_current_editing_mode = EditingMode::kEditingMode_AIGraphNavigation;
	}

	bool EditorManager::IsObjectSelectionEnabled(void) const { return this->m_object_selection_enabled; }

	void EditorManager::SetObjectSelectionEnabled(bool value) { this->m_object_selection_enabled = value; if (value) this->m_current_editing_mode = EditingMode::kEditingMode_ObjectSelection; }

	void EditorManager::DisableEditing()
	{
		this->m_ai_navigation_editing_enabled = false;
		this->m_object_selection_enabled = false;
		this->m_current_editing_mode = EditingMode::kEditingMode_NoSelection;
	}

	bool EditorManager::IsSimulate(void) const
	{
		return this->m_game_simulate;
	}

	void EditorManager::SetSimulate(bool value)
	{
		this->m_game_simulate = value;

		this->m_current_editing_mode = EditingMode::kEditingMode_NoSelection;
		this->m_ai_navigation_editing_enabled = false;
		this->m_object_selection_enabled = false;
	}

	void EditorManager::OnCloseApplication()
	{
		if (!this->m_show_modal_close_application_window)
			this->m_show_modal_close_application_window = true;
	}

	bool EditorManager::IsNeedToCloseApplication() const
	{
		return this->m_close_application;
	}

	void EditorManager::SetWorldXML(char* data, size_t length_of_file)
	{
		if (data)
		{
			if (length_of_file)
			{
				if (this->m_p_allocated_memory)
				{
					delete[] this->m_p_allocated_memory;
					this->m_p_allocated_memory = nullptr;
				}

				this->m_p_allocated_memory = new char[length_of_file+1];
				this->m_p_allocated_memory[length_of_file] = '\0';
				memcpy(this->m_p_allocated_memory, data, length_of_file);

				auto status = this->m_world_xml.Parse(this->m_p_allocated_memory, length_of_file);

				Assert(status == tinyxml2::XML_SUCCESS);
			}
		}
	}
	tinyxml2::XMLDocument& EditorManager::GetWorldXML()
	{
		return this->m_world_xml;
	}

	void EditorManager::Load(tinyxml2::XMLElement& tagWorld)
	{
		for (IEditorWindow* pWindow : this->m_windows)
		{
			if (pWindow)
			{
				pWindow->Load(tagWorld);
			}
		}
	}
}