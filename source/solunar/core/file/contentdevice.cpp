#include "core/core.h"
#include "core/debug.h"
#include <memory>
#include <string>
#include "core/file/datastream.h"
#include "core/file/contentdevice.h"
#include "core/file/filesystem.h"
#include "core/file/filestream.h"
#include "core/file/filesystem_pack.h"

namespace solunar
{

ContentDevice* g_contentDevice = nullptr;

ContentDevice::ContentDevice(const std::string& dirname, const std::string& working_directory) :
	m_path(dirname), m_working_path{}
{
	Assert(working_directory.empty() == false && "application always has working directory");
	Assert(working_directory.size() <= sizeof(this->m_working_path) / sizeof(this->m_working_path[0]) && "overflow can't write a such large path");

	memcpy(this->m_working_path, working_directory.c_str(), working_directory.size());
}

ContentDevice::~ContentDevice()
{
}

DataStreamPtr ContentDevice::OpenStream(const std::string& filename)
{
	Assert2(g_fileSystem, "Content device called before file system initialization.");

	// compute path
	std::string path = m_working_path;
	
	if (!(path.back() == '/' || path.back() == '\\'))
		path += "/";

	path += m_path;
	path += "/";
	path += filename;

	// open an stream from packfile
	if (g_fileSystemPack.exist(filename))
		return g_fileSystemPack.openStream(filename, true);

	// open an stream from absolute path
	if (g_fileSystem->Exist(filename.c_str()))
		return std::make_shared<FileStream>(filename);

	if (!g_fileSystem->Exist(path.c_str()))
		return DataStreamPtr(nullptr);

	// create object
	return std::make_shared<FileStream>(path);
}

const char* ContentDevice::GetWorkingPath() const
{
	return this->m_working_path;
}


}