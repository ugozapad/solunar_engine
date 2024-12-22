#include "core/core.h"
#include "core/debug.h"
#include "core/file/contentdevice.h"
#include "core/file/contentmanager.h"
#include "core/file/filesystem.h"
#include "core/file/contentdevice.h"
#include "core/object/serializableobject.h"
#include "core/object/typemanager.h"

namespace solunar
{

ContentManager* g_contentManager = nullptr;

ContentManager::ContentManager()
{
}

ContentManager::~ContentManager()
{
}

void ContentManager::Init()
{
}

void ContentManager::Shutdown()
{
	for (auto& it : m_content)
	{
		if (it.second)
		{
			Core::Msg("ContentManager: releasing %s", it.first.c_str());
			it.second.reset();
			it.second = nullptr;
		}
	}

	m_content.clear();
}

void ContentManager::MountDevice(ContentDevice* contentDevice, const std::string& name)
{
	Assert2(m_devices.find(name) == m_devices.end(), "Failed to mound already mounted device.");
	Assert2(contentDevice, "Content device is nullptr.");

	m_devices[name] = contentDevice;

	Core::Msg("ContentManager: mounted device %s (%s/)", name.c_str(), contentDevice->GetPath().c_str());
}

void ContentManager::UnmountDevice(const std::string& name)
{
	Assert2(findContentDevice(name), "Trying to unmount device which was not be mounted or existed.");
	m_devices.erase(name);
}

const char* ContentManager::GetWorkingPath(const char* pDeviceName)
{
	const char* pResult = nullptr;

	ContentDevice* pDevice = findContentDevice(pDeviceName);
	if (pDevice)
	{
		pResult = pDevice->GetWorkingPath();
	}

	return pResult;
}

ContentDevice* ContentManager::findContentDevice(const std::string& name)
{
	auto it = m_devices.find(name);
	if (it == m_devices.end())
		return nullptr;

	return m_devices[name];
}

DataStreamPtr ContentManager::OpenStream(const std::string& path)
{
	// return m_devices[""]->openStream(path);
	return OpenStream("game", path);
}

DataStreamPtr ContentManager::OpenStream(const std::string& name, const std::string& path)
{
	ContentDevice* contentDevice = findContentDevice(name);
	Assert3(contentDevice, "Couldn't find content device, probably it's not mounted or unaccessible in retail build.", name.c_str());

	return contentDevice->OpenStream(path);
}

std::weak_ptr<SerializableObject> ContentManager::Load(const std::string& filename, const TypeInfo* pTypeInfo)
{
	Assert(pTypeInfo);
	Assert2(pTypeInfo->IsA(SerializableObject::GetStaticTypeInfo()), "Canno't load object because it doesn't inherit from SerializableObject");

	ContentDevice* contentDevice = findContentDevice("game");

	auto objectInstance = m_content.find(filename);

	// we will guess content is not loaded for now
	if (objectInstance == m_content.end())
	{
		DataStreamPtr stream = contentDevice->OpenStream(filename);
		if (!stream)
		{
			Core::Msg("ContentManager: failed to load %s from file \"%s\"", pTypeInfo->GetClassName(),
				filename.c_str());

			return std::weak_ptr<SerializableObject>();
		}

		SerializableObject* objectInstance = (SerializableObject*)TypeManager::GetInstance()->CreateObjectByTypeInfo(pTypeInfo);

		std::shared_ptr<SerializableObject> object = std::shared_ptr<SerializableObject>(objectInstance, ObjectDeleter);
		object->Load(stream);
		Core::Msg("ContentManager: loaded %s %s", pTypeInfo->GetClassName(), filename.c_str());
		m_content.emplace(filename, object);
	}

	objectInstance = m_content.find(filename);
	return (*objectInstance).second;
}

void ContentManager::AddExisting(const std::string& filename, std::shared_ptr<SerializableObject> object)
{
	auto objectInstance = m_content.find(filename);
	Assert(objectInstance == m_content.end());
	m_content.emplace(filename, object);
}

//void ContentManager::loadExisted(const std::string& filename, std::shared_ptr<SerializableObject> object)
//{
//	ASSERT(object);
//	ASSERT2(object->IsA(SerializableObject::GetStaticTypeInfo()), "Canno't Load object because it doesn't inherit from SerializableObject");
//
//	Core::Msg("[content]: loading existed %s %s", object->getTypeInfo()->GetClassName(), filename.c_str());
//
//	ContentDevice* contentDevice = findContentDevice();
//
//	DataStreamPtr stream = contentDevice->openStream(filename);
//	object->Load(stream);
//}

}
