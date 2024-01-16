#include "core/core.h"
#include "core/utils/logger.h"
#include "core/memory/memorymanager.h"
#include "core/file/filesystem_win32.h"
#include "core/file/datastreamfactory.h"
#include "core/file/contentmanager.h"
#include "core/object/typemanager.h"
#include "core/object/object.h"
#include "core/object/serializableobject.h"

namespace engine
{
	namespace
	{
		void registerCoreTypes()
		{
			TypeManager::getInstance()->registerObject<Object>();
			TypeManager::getInstance()->registerObject<SerializableObject>();
		}
	}

	void Core::init()
	{
		Logger::init();

		Logger::logPrint("Core builted at %s %s", __TIME__, __DATE__);

		MemoryManager::initialize();

		// Allocate native filesystem
		g_fileSystem = mem_new<FileSystem_Win32>();

		DataStreamFactory::createInstance();

		g_contentManager = mem_new<ContentManager>();

		// register core types
		registerCoreTypes();
	}

	void Core::shutdown()
	{
		if (g_contentManager)
		{
			mem_delete(g_contentManager);
			g_contentManager = nullptr;
		}

		DataStreamFactory::destroyInstance();

		if (g_fileSystem)
		{
			mem_delete(g_fileSystem);
			g_fileSystem = nullptr;
		}

		MemoryManager::shutdown();

		Logger::shutdown();
	}
}