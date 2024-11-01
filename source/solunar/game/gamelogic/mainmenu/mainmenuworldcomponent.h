#ifndef MAINMENU_WORLDCOMPONENT_H
#define MAINMENU_WORLDCOMPONENT_H

#include "engine/entity/logiccomponent.h"

namespace solunar {
	
	class MainMenuWorldComponent : public LogicComponent
	{
		DECLARE_OBJECT(MainMenuWorldComponent);
	public:
		static void RegisterObject();
		
	public:
		MainMenuWorldComponent();
		~MainMenuWorldComponent();
		
		// Component inheritance
		void OnWorldSet(World* world);
	};
	
}

#endif