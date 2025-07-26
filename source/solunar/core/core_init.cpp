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

	// if any of asserts fail means we unit tests is failed and IT MEANS IT IS IMPORTANT!
	// all tests must be pass OK without assertions
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
		assert(is_equal);
		is_equal = test2 == test2;
		assert(is_equal);
		is_equal = test == test;
		assert(is_equal);
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

		string<10> kekais = "kekais";
		string<10> kekais2(kekais);

		string<5> smol("smol");
		string<15> bigger(smol);

		string<9> good("good");
		string<10> sl(good);

		static_string<10> sl2(good);

		// won't compile because we prevent construction from bigger strings if current string is not support reallocation
	//	string<11> fail("fail");
	//	static_string<10> sl3(fail);

	//	static_string<11> fail("fail");
	//	static_string<10> sl4(fail);

		is_equal = kekais == kekais2;
		assert(is_equal);

		unordered_set<int, 5> us1;
		static_unordered_set<int, 5> u2(us1);

		// won't compile because we prevent where instance is smaller then passed and it doesn't support reallocation
	//	unordered_set<int, 5> us3;
	//	static_unordered_set<int, 3> us4(us3);

		vector<int, 5> v1;
		static_vector<int, 5> v2(v1);

		// won't compile because we prevent where instance is smaller then passed and it doesn't support reallocation
	//	vector<int, 5> v3;
	//	static_vector<int, 3> v4(v3);
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