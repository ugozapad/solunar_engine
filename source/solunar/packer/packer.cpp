#include "packer.h"

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <shlobj_core.h>

#include "lz4.h"

#include "core/file/packfile.h"

using namespace solunar;

std::string getFileNameWithoutExtension(const std::string& filename)
{
	size_t lastindex = filename.find_last_of(".");
	if (lastindex != std::string::npos) {
		return filename.substr(0, lastindex);
	}

	return filename;
}

std::string getFilePath(const std::string& filename)
{
	size_t lastindex = filename.find_last_of("/");
	if (lastindex == std::string::npos) {
		lastindex = filename.find_last_of("\\");
	}

	if (lastindex != std::string::npos) {

		return filename.substr(0, lastindex);
	}

	return std::string();
}

std::string toWin32Path(const std::string& filename)
{
	std::string result = filename;

	for (int i = 0; i < filename.length(); i++) {
		if (result[i] == '/')
			result[i] = '\\';
	}

	return result;
}


class CommandArgs
{
public:
	void init(int argc, char* argv[])
	{
		m_argc = argc;
		m_argv = argv;
	}

	bool hasOption(const char* name)
	{
		for (int i = 0; i < m_argc; i++) {
			if (m_argv[i]) {
				if (strcmp(m_argv[i], name) == 0)
					return true;
			}
		}

		return false;
	}

	const char* getOptionParameter(const char* name)
	{
		for (int i = 0; i < m_argc; i++) {
			if (m_argv[i]) {
				if (strcmp(m_argv[i], name) == 0) {
					if (!m_argv[i + 1]) {
						printf("Commandline option %s doesn't have parameter!", name);
					}
					else {
						return m_argv[i + 1];
					}
				}
			}
		}

		return nullptr;
	}

private:
	int m_argc;
	char** m_argv;
};

static CommandArgs s_args;

static const char* g_defaultDataPath = "data";
static const char* g_defaultExtension = ".packfile";

PackFileWriter g_packFileWriter;

PackFileWriter::PackFileWriter()
{
	m_file = nullptr;
}

void PackFileWriter::initializePaths()
{
	m_currentPath = std::filesystem::current_path();
	printf("current directory: %s\n", m_currentPath.string().c_str());

	m_currentDataPath = m_currentPath / std::filesystem::path("data");
	printf("data path: %s\n", m_currentDataPath.string().c_str());
}

void PackFileWriter::createPack(const std::string& name)
{
	initializePaths();

	m_file = fopen(name.c_str(), "wb");
	if (!m_file)
		assert(true && "failed to open file!!!");
		
	std::vector<std::filesystem::path> fileList;
		
	// find all files from data directory
	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(m_currentDataPath))
	{
		if (dir_entry.is_regular_file())
		{
			printf("file: %s\n", dir_entry.path().string().c_str());
			fileList.push_back(dir_entry.path());
		}
	}

	printf("writing package %s\n", name.c_str());
		
	fclose(m_file);
}

void PackFileWriter::createPack(const std::string& name, const std::string& fileList)
{
	initializePaths();

	FILE* fList = fopen(fileList.c_str(), "rb");
	if (!fList)
		assert(true && "failed to open file!!!");

	std::vector<std::string> fileListing;

	char buffer[1024];
	while (fscanf(fList, "%s", buffer) == 1)
	{
		if (buffer[0] == '#' || buffer[0] == '\0') while (fread(buffer, 1, 1, fList) == 1 && buffer[0] != '\n');
		else fileListing.push_back(buffer);
	}

	std::string pakName = name;
	pakName += g_defaultExtension;

	printf("writing package %s\n", pakName.c_str());

	m_file = fopen(pakName.c_str(), "wb");
	if (!m_file)
		assert(true && "failed to open file!!!");

	if (!s_args.hasOption("-lz4"))
		writeFiles(fileListing);
	else
		writeCompressedFiles(fileListing);

	printf("ok\n");

	fclose(m_file);
}

void PackFileWriter::writeCompressedFiles(const std::vector<std::string>& files)
{
	PackHeader h;
	strcpy(h.magic, "packfile");
	h.version = 1;
	h.filecount = files.size();
	fwrite(&h, sizeof(h), 1, m_file);

	std::vector<PackFileEntry> fileEntries;
	fileEntries.resize(files.size());
	fwrite((void*)fileEntries.data(), sizeof(PackFileEntry) * files.size(), 1, m_file);

	for (int i = 0; i < files.size(); i++)
	{
		std::vector<char> rawFile = readFileToVector(files[i].c_str());
		std::vector<char> compressedFile;
			
		const int maxCompressedDataSize = LZ4_compressBound(rawFile.size());
		compressedFile.resize(maxCompressedDataSize);

		const int compressedDataSize = LZ4_compress_default((const char*)rawFile.data(), compressedFile.data(), rawFile.size(), maxCompressedDataSize);

		// fix path
		std::filesystem::path originalFilePath = std::filesystem::path(files[i]);
		originalFilePath = originalFilePath.lexically_relative(m_currentDataPath);

		strcpy(fileEntries[i].filename, originalFilePath.generic_string().c_str());
		fileEntries[i].pointerOffset = ftell(m_file);
		fileEntries[i].size = rawFile.size();
		fileEntries[i].compressedSize = compressedDataSize;
		fileEntries[i].isCompressed = true;
			
		fwrite(compressedFile.data(), sizeof(char) * compressedFile.size(), 1, m_file);
		//fwrite(rawFile.data(), sizeof(char) * rawFile.size(), 1, m_file);
	}

	// roll back to fat and change offsets
	fseek(m_file, sizeof(h), SEEK_SET);

	fwrite((void*)fileEntries.data(), sizeof(PackFileEntry) * files.size(), 1, m_file);
}

void PackFileWriter::writeFiles(const std::vector<std::string>& files)
{
	PackHeader h;
	strcpy(h.magic, "packfile");
	h.version = 1;
	h.filecount = files.size();
	fwrite(&h, sizeof(h), 1, m_file);

	std::vector<PackFileEntry> fileEntries;
	fileEntries.resize(files.size());
	fwrite((void*)fileEntries.data(), sizeof(PackFileEntry) * files.size(), 1, m_file);
		
	for (int i = 0 ; i < files.size(); i++)
	{
		std::vector<char> rawFile = readFileToVector(files[i].c_str());

		// fix path
		std::filesystem::path originalFilePath = std::filesystem::path(files[i]);
		originalFilePath = originalFilePath.lexically_relative(m_currentDataPath);

		strcpy(fileEntries[i].filename, originalFilePath.generic_string().c_str());
		fileEntries[i].pointerOffset = ftell(m_file);
		fileEntries[i].size = rawFile.size();
		fileEntries[i].compressedSize = 0;
		fileEntries[i].isCompressed = false;

		fwrite(rawFile.data(), sizeof(char) * rawFile.size(), 1, m_file);
	}

	// roll back to fat and change offsets
	fseek(m_file, sizeof(h), SEEK_SET);

	fwrite((void*)fileEntries.data(), sizeof(PackFileEntry) * files.size(), 1, m_file);
}

std::vector<char> PackFileWriter::readFileToVector(const std::string& filename)
{
	FILE* f = fopen(filename.c_str(), "rb");
	assert(f && "failed to open file!!!");

	printf("process: %s\n", filename.c_str());

	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	fseek(f, 0, SEEK_SET);

	std::vector<char> tempBuffer;
	tempBuffer.resize(length);

	fread(tempBuffer.data(), sizeof(char) * length, 1, f);
	fclose(f);

	return tempBuffer;
}

#include <fstream>

void generateFileList()
{
	std::ofstream stream("data_main.txt");
	
	std::filesystem::path currentPath = std::filesystem::current_path();
	printf("current directory: %s\n", currentPath.string().c_str());

	std::filesystem::path dataPath = currentPath / std::filesystem::path("data");
	printf("data path: %s\n", dataPath.string().c_str());

	std::vector<std::filesystem::path> fileList;
		
	// find all files from data directory
	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(dataPath))
	{
		if (dir_entry.is_regular_file())
		{
			stream << dir_entry.path().string() << std::endl;
			fileList.push_back(dir_entry.path());
		}
	}

	stream.close();
}

int createDirectoryRecursively(LPCTSTR path)
{
	return SHCreateDirectoryEx(NULL, path, NULL);
}

void unpack() {

	const char* filename = s_args.getOptionParameter("-unpack");
	if (!filename)
		return;

	FILE* f = fopen(filename, "rb");
	if (!f)
		return;

	char currentDir[256];
	GetCurrentDirectoryA(256, currentDir);

	PackHeader h;
	fread(&h, sizeof(h), 1, f);

	PackFileEntry* filetable = (PackFileEntry*)malloc(h.filecount * sizeof(PackFileEntry));
	fread(filetable, sizeof(PackFileEntry), h.filecount, f);

	for (int i = 0; i < h.filecount - 1; i++) {
		fseek(f, filetable[i].pointerOffset, SEEK_SET);
		if (filetable[i].isCompressed) {
			std::vector<char> rawData;
			rawData.resize(filetable[i].compressedSize);
			fread(rawData.data(), rawData.size(), 1, f);
			
			std::vector<char> fileData;
			fileData.resize(filetable[i].size);

			LZ4_decompress_safe(rawData.data(), fileData.data(), filetable[i].compressedSize, filetable[i].size);

			std::string directory = currentDir;
			directory += "\\";
			directory += getFilePath(filetable[i].filename);
			directory = toWin32Path(directory);

			createDirectoryRecursively(directory.c_str());

			DWORD err = GetLastError();

			FILE* fp = fopen(filetable[i].filename, "wb");
			fwrite(fileData.data(), fileData.size(), 1, fp);
			fclose(fp);
		}
	}

	free(filetable);
}

int main(int argc, char* argv[])
{
	s_args.init(argc, argv);
	
	if (s_args.hasOption("-pack"))
	{
		g_packFileWriter.createPack(s_args.getOptionParameter("-pack"));
	}
	else if (s_args.hasOption("-pack_list"))
	{
		g_packFileWriter.createPack(getFileNameWithoutExtension(s_args.getOptionParameter("-pack_list")), s_args.getOptionParameter("-pack_list"));
	}
	else if (s_args.hasOption("-generateFileList"))
	{
		generateFileList();
	}
	else if (s_args.hasOption("-unpack"))
	{
		unpack();
	}
	else
	{
		printf("where is -pack or -pack_list ???");
	}

	system("pause");
	return 0;
}
