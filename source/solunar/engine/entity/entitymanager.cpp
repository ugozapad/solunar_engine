#include "enginepch.h"
#include "engine/entity/entity.h"
#include "engine/entity/entitymanager.h"

namespace solunar
{

EntityManager::EntityManager() :
	m_deferredMode(false)
{
}

EntityManager::~EntityManager()
{
	Destroy();
}

void EntityManager::Destroy()
{
	if (!m_entities.empty())
	{
		for (auto entity : m_entities)
		{
			if (entity)
			{
				mem_delete(entity);
			}
		}

		m_entities.clear();
	}
}

void EntityManager::Update()
{
	if (!m_deferredMode)
		return;

	if (!m_deferredEntities.empty())
	{
		for (auto entity : m_deferredEntities)
		{
			m_entities.push_back(entity);
		}

		m_deferredEntities.clear();
	}
}

void EntityManager::OnWorldLoaded()
{
	m_deferredMode = true;
}

Entity* EntityManager::CreateEntity()
{
	Entity* entity = g_typeManager->CreateObject<Entity>();

	if (m_deferredMode)
		m_deferredEntities.push_back(entity);
	else
		m_entities.push_back(entity);

	return entity;
}

Entity* EntityManager::CreateEntityEx(const TypeInfo* typeInfo)
{
	Entity* entity = (Entity*)g_typeManager->CreateObjectByTypeInfo(typeInfo);
	
	if (m_deferredMode)
		m_deferredEntities.push_back(entity);
	else
		m_entities.push_back(entity);

	return entity;
}

Entity* EntityManager::GetEntityByName(const std::string& name)
{
	for (auto it : m_entities)
		if (it->GetName() == name)
			return it;

	return nullptr;
}

std::vector<Entity*> EntityManager::GetEntitiesWithComponentTypeInfo(const TypeInfo* typeInfo)
{
	std::vector<Entity*> entities;

	for (auto entity : m_entities)
	{
		if (entity->GetComponentByTypeInfo(typeInfo))
			entities.push_back(entity);
	}

	return entities;
}

}