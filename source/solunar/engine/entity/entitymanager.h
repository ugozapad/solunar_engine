#ifndef ENGINE_ENTITY_ENTITYMANAGER_H
#define ENGINE_ENTITY_ENTITYMANAGER_H

namespace solunar
{

class Entity;

class EntityManager
{
public:
	EntityManager();
	~EntityManager();

	void Destroy();

	void Update();

	void OnWorldLoaded();

	Entity* CreateEntity();
	Entity* CreateEntityEx(const TypeInfo* typeInfo);

	Entity* GetEntityByName(const std::string& name);

	const std::vector<Entity*>& GetEntities() { return m_entities; }

	std::vector<Entity*> GetEntitiesWithComponentTypeInfo(const TypeInfo* typeInfo);

	template <typename T>
	std::vector<Entity*> GetEntitiesWithComponent();

private:
	std::vector<Entity*> m_entities;
	std::vector<Entity*> m_deferredEntities;
	std::vector<Entity*> m_entitiesToDestroy;
	bool m_deferredMode;
};

template<typename T>
inline std::vector<Entity*> EntityManager::GetEntitiesWithComponent()
{
	return GetEntitiesWithComponentTypeInfo(T::GetStaticTypeInfo());
}

}

#endif // !ENGINECORE_ENTITY_ENTITYMANAGER_H
