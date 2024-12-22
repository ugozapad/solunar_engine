#ifndef SOLUNAR_EDITOR_WINDOW_H
#define SOLUNAR_EDITOR_WINDOW_H

namespace solunar 
{
	class World;
	class InputManager;
}

namespace solunar
{
	class IEditorWindow 
	{
	public:
		virtual ~IEditorWindow(void) {}

		virtual void Init(void) = 0;
		virtual void Draw(World* pWorld) = 0;
		virtual const char* GetName(void) const = 0;
		virtual bool IsShow(void) const = 0;
		virtual void SetShow(bool status) = 0;

		virtual int GetEditingMode() = 0;

		virtual void Load(tinyxml2::XMLElement& tagWorld) = 0;

		// calls only when editor's editing mode == GetEditingMode()
		// it supposed that only one window can have one unique editing mode
		virtual void UpdateEditingMode(InputManager* pInputManager, World* pWorld) = 0;
	};
}

#endif