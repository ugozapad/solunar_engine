#include "core/memory/memorymanager.h"
#include "core/core.h"
#include "core/object/typemanager.h"
#include "core/object/object.h"

#include <string.h>

namespace solunar
{

static TypeManager s_typeManager;

Object* createObjectPrivate(const TypeInfo* typeInfo)
{
	IAllocator* pAllocator = MemoryManager::getDefaultAllocator();
	Object* pObject = (Object*)pAllocator->allocate(typeInfo->GetClassSize(), typeInfo->GetClassAlign());

	StaticConstructor_t objectConstructor = typeInfo->GetStaticConstructor();
	Assert3(objectConstructor, "Failed to create object without static constructor", typeInfo->GetClassName());

	objectConstructor(pObject);

	return pObject;
}

TypeManager* TypeManager::GetInstance()
{
	return &s_typeManager;
}

const TypeInfo* TypeManager::GetTypeInfoByName(const char* name)
{
	for (auto it : m_registeredTypes)
	{
		if (strcmp(it->GetClassName(), name) == 0)
			return it;
	}

	return nullptr;
}

const TypeInfo* TypeManager::GetTypeInfoById(size_t hash)
{
	for (auto it : m_registeredTypes)
	{
		if (it->GetStringHash() == hash)
			return it;
	}

	return nullptr;
}

Object* TypeManager::CreateObjectByName(const char* name)
{
	for (auto it : m_registeredTypes)
	{
		if (strcmp(it->m_name, name) == 0)
			return createObjectPrivate(it);
	}

	return nullptr;
}

Object* TypeManager::CreateObjectByTypeInfo(const TypeInfo* typeInfo)
{
	for (auto it : m_registeredTypes)
	{
		if (it->IsExactly(typeInfo))
			return createObjectPrivate(it);
	}

	return nullptr;
}

void TypeManager::RegisterType(const TypeInfo* typeInfo)
{
	Core::Msg("TypeManager: registered type %s", typeInfo->GetClassName());
	m_registeredTypes.push_back(typeInfo);
}

void TypeManager::GetRegisteredTypes(std::vector<const TypeInfo*>& registeredTypes)
{
	registeredTypes = m_registeredTypes;
}

}