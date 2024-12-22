#ifndef EDITOR_WINDOW_ENTITY_EDITOR_H
#define EDITOR_WINDOW_ENTITY_EDITOR_H

#include "editor_window.h"

namespace solunar
{
	class Entity;
}

namespace solunar
{
	class EditorWindow_EntityEditor : public IEditorWindow
	{
	public:
		EditorWindow_EntityEditor();
		~EditorWindow_EntityEditor();

		void Init(void) override;
		void Draw(World* pWorld) override;
		const char* GetName(void) const override;
		bool IsShow(void) const override;
		void SetShow(bool status) override;
		int GetEditingMode() override;
		void UpdateEditingMode(InputManager* pInputManager, World* pWorld) override;

		void Load(tinyxml2::XMLElement& tagWorld) override;
	private:
		bool m_show;
		int m_selected_type;
	};
}

#endif