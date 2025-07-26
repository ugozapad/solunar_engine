#include "core/core.h"
#include "core/timer.h"
#include "core/utils/logger.h"
#include "core/memory/memorymanager.h"
#include "core/file/filesystem_win32.h"
#include "core/file/filesystem_pack.h"
#include "core/file/contentmanager.h"
#include "core/object/typemanager.h"
#include "core/object/object.h"
#include "core/object/serializableobject.h"
#include "core/object/propertymanager.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

namespace solunar
{

	void RegisterCoreTypes()
	{
		TypeManager::GetInstance()->RegisterObject<Object>();
		TypeManager::GetInstance()->RegisterObject<SerializableObject>();
	}

#define ENABLE_UNIT_TESTS

	void RunUnitTests()
	{
#ifdef ENABLE_UNIT_TESTS

		static_vector<int, 4> test;
		int a = sizeof(test);
		test.push_back(1);
		test.push_back(1);
		test.push_back(1);
		test.push_back(1);

		// since static_vector we can't reallocate at all thus we will get std::bad_alloc
		try {
			test.push_back(1);
		}
		catch (const std::bad_alloc& e)
		{
			const char* p_reason = e.what();
		}

		// preallocated acts like a array so it is already allocated memory and ready in use (don't need to specify reserve manually + preallocated memory is stack not heap so very fast initialization and usage!)
		vector<int, 4> test2;

		test2.push_back(1);
		test2.push_back(1);
		test2.push_back(1);
		test2.push_back(1);

		// will be true
		bool is_equal = test == test2;
		is_equal = test2 == test2;
		is_equal = test == test;
		// exceeding current memory, do reallocation but it is heap based as default vector
		// no troubles and no throw exceptions
		test2.push_back(1);

		bool not_equal = test2 == test;

		string<10> kek = "kek";
		int b = kek.size();
		b = kek.max_size();

		try {
			static_string<5> aa = "kekius_maximus";
		}
		// catching this because we can't reallocate due to static nature of memory_resource
		catch (const std::bad_alloc& e)
		{
			int a = 0;
		}

#endif
	}

	void Core::Init()
	{
		// Initialize random
		srand(time(NULL));

		Logger::Init();
		Logger::LogPrint("Core builted at %s %s", __TIME__, __DATE__);

		MemoryManager::Initialize();

		Timer::GetInstance()->Init();

		// Allocate native filesystem
		g_fileSystem = mem_new<FileSystem_Win32>();

		g_fileSystemPack.initialize();

		g_contentManager = mem_new<ContentManager>();
		g_contentManager->Init();

		// register core types
		RegisterCoreTypes();

#ifdef _DEBUG
		RunUnitTests();
#endif
	}

	void Core::Shutdown()
	{
		if (g_contentManager)
		{
			mem_delete(g_contentManager);
			g_contentManager = nullptr;
		}

		g_fileSystemPack.shutdown();

		if (g_fileSystem)
		{
			mem_delete(g_fileSystem);
			g_fileSystem = nullptr;
		}

		PropertyManager::GetInstance()->Shutdown();

		MemoryManager::Shutdown();

		Logger::Shutdown();
	}

}