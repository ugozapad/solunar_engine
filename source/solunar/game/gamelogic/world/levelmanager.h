#ifndef GAMELOGIC_LEVELMANAGER_H
#define GAMELOGIC_LEVELMANAGER_H

namespace solunar
{
	
class LevelManagerComponent : public Component
{
	ImplementObject(LevelManagerComponent, Component);
public:
	LevelManagerComponent();
	~LevelManagerComponent();

	static void registerObject();
	
	void onWorldSet(World* world) override;


private:

};

}

#endif // !GAMELOGIC_LEVELMANAGER_H
