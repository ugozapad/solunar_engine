#ifndef FILE_CONTENTDEVICE_H
#define FILE_CONTENTDEVICE_H

#include <string>

#include "core/utils/singleton.h"
#include "core/file/datastream.h"

namespace solunar
{

class ContentDevice
{
public:
	ContentDevice(const std::string& dirname, const std::string& working_directory);
	~ContentDevice();

	DataStreamPtr OpenStream(const std::string& filename);

	const std::string& GetPath() { return m_path; }

	const char* GetWorkingPath() const;
private:
	std::string m_path;
	char m_working_path[1024];
};

extern ContentDevice* g_contentDevice;

}

#endif // !FILE_CONTENTDEVICE_H
