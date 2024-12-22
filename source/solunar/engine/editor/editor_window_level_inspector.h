#ifndef EDITOR_WINDOW_LEVEL_INSPECTOR_H
#define EDITOR_WINDOW_LEVEL_INSPECTOR_H

#include "engine/editor/editor_window.h"

namespace solunar
{
	class EditorWindow_LevelInspector : public IEditorWindow
	{
	public:
		EditorWindow_LevelInspector();
		~EditorWindow_LevelInspector();

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
	};
}

#endif