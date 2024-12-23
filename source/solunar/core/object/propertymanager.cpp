#include "core/core.h"
#include "core/debug.h"
#include "core/memory/memorymanager.h"
#include "core/object/propertymanager.h"

namespace solunar
{

// Global instance
PropertyManager PropertyManager::ms_instance;

PropertyManager::PropertyManager()
{
}

PropertyManager::~PropertyManager()
{
}

void PropertyManager::Shutdown()
{
	for (auto it : m_properies) {
		for (auto prop : it.second) {
			mem_delete(prop);
		}
	}

	m_properies.clear();
}

void PropertyManager::RegisterProperty(const TypeInfo* typeInfo, IProperty* propertyInstance)
{
	Assert(typeInfo);
	Assert(propertyInstance);

	// try to find vector
	auto it = m_properies.find(typeInfo->GetStringHash());
	if (it == m_properies.end())
	{
		std::vector<IProperty*> properties;
		properties.push_back(propertyInstance);
		m_properies.emplace(typeInfo->GetStringHash(), properties);
	}
	else
	{
		m_properies.find(typeInfo->GetStringHash())->second.push_back(propertyInstance);
	}

	// #TODO DISABLE LOGGING FOR PROPERTY REGISTER
	//Core::Msg("PropertyManager: registered property %s for type %s", propertyInstance->GetName(), typeInfo->GetClassName());
}

IProperty* PropertyManager::FindProperty(const TypeInfo* typeInfo, const char* name)
{
	Assert(m_properies.find(typeInfo->GetStringHash()) != m_properies.end());

	auto it = m_properies.find(typeInfo->GetStringHash());
	for (auto prop : it->second)
	{
		if (strcmp(prop->GetName(), name) == 0)
			return prop;
	}

	return nullptr;
}

void PropertyManager::GetProperties(const TypeInfo* typeInfo, std::vector<IProperty*>& properties)
{
	for (const TypeInfo* pTypeInfo = typeInfo; pTypeInfo; pTypeInfo = pTypeInfo->m_baseInfo)
	{
		auto it = m_properies.find(pTypeInfo->GetStringHash());
		if (it != m_properies.end())
		{
			properties.insert(properties.end(), it->second.begin(), it->second.end());
		}
	}
}

PropertyRegistrator* PropertyRegistrator::GetInstance()
{
	static PropertyRegistrator s_propertyRegistrator;
	return &s_propertyRegistrator;
}

PropertyRegistrator::PropertyRegistrator()
{
}

PropertyRegistrator::~PropertyRegistrator()
{
}

void PropertyRegistrator::AddFunc(RegisterPropertiesFunc func)
{
	m_funcs.push_back(func);
}

void PropertyRegistrator::RegisterClasses()
{
	for (auto it : m_funcs)
		it();

	m_funcs.clear();
}

}