#ifndef EDITOR_MANAGER_H
#define EDITOR_MANAGER_H

namespace solunar
{
	class IEditorWindow;
	class World;
}

namespace solunar
{
	enum EditingMode
	{
		kEditingMode_AIGraphNavigation,
		kEditingMode_ObjectSelection,
		kEditingMode_NoSelection, // unable to select anything on screen, but editor's GUI i working
		kEditingMode_Null // reserved for windows
	};

	class EditorManager : public Singleton<EditorManager>
	{
		struct EditorCamera
		{
			float yaw{};
			float pitch{};
			glm::vec3 direction{};
		};

	public:
		EditorManager();
		~EditorManager();

		void Init(World* pWorld);
		void PostInit();
		void Shutdown();

		void Update();
		void RegisterWindow(IEditorWindow* pWindow);

		void* GetSelectedEntity();
		void SetSelectedEntity(void* pEntity);

		const std::vector<IEditorWindow*>& GetWindows(void) const;
		
		bool IsAINavigationEditingEnabled(void) const;
		void SetAINavigationEditingEnabled(bool value);

		bool IsObjectSelectionEnabled(void) const;
		void SetObjectSelectionEnabled(bool value);

		void DisableEditing();

		bool IsSimulate(void) const;
		void SetSimulate(bool value);

		// shows modal window
		void OnCloseApplication();
		bool IsNeedToCloseApplication() const;
		void SetWorldXML(char* data, size_t length_of_file);
		tinyxml2::XMLDocument& GetWorldXML();
		void Load(tinyxml2::XMLElement& tagWorld);
	private:
		void InitWindows();
		void UpdateEditingModes();
		void UpdateEditingMode_AIGraphNavigation();
		void UpdateEditingMode_ObjectSelection();

		void UpdateCamera();

		IEditorWindow* GetWindowByEditingMode(EditingMode mode);

		void ShowModalWindowOnCloseApplication();
	private:
		bool m_object_selection_enabled;
		bool m_ai_navigation_editing_enabled;
		bool m_game_simulate;
		bool m_close_application;
		bool m_show_modal_close_application_window;
		EditingMode m_current_editing_mode;
		// todo: kirrik -> change to entity id type like uint32_t or something else, storing a pointer is not a good thing at all, it is temporary implementation
		void* m_pSelectedEntity;
		World* m_pWorld;
		IEditorWindow* m_pEditingMode_AINavigationGraph;
		IEditorWindow* m_pEditingMode_ObjectSelection;
		char* m_p_allocated_memory;
		size_t m_length_of_file;
		EditorCamera m_cam;
		std::vector<IEditorWindow*> m_windows;
		tinyxml2::XMLDocument m_world_xml;

	};
	
	extern EditorManager* g_editorManager;
}

#endif // EDITOR_MANAGER_H