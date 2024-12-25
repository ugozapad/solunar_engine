#ifndef SHOCKGAME_SHOCKGAMEUI_H
#define SHOCKGAME_SHOCKGAMEUI_H

#include "engine/entity/logiccomponent.h"

namespace solunar
{

class ShockGameMainMenuComponent : public LogicComponent
{
	DECLARE_OBJECT(ShockGameMainMenuComponent);

public:
	ShockGameMainMenuComponent();
	~ShockGameMainMenuComponent();

	void OnWorldSet(World* world) override;

	void Update(float dt) override;
};

class DemoGameMainMenuComponent : public LogicComponent
{
	DECLARE_OBJECT(DemoGameMainMenuComponent);

public:
	DemoGameMainMenuComponent();
	~DemoGameMainMenuComponent();

	void OnWorldSet(World* world) override;

	void Update(float dt) override;

public:
	bool m_inGame = false;
};

}

#endif // !SHOCKGAME_SHOCKGAMEUI_H
