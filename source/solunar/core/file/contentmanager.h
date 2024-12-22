#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H

#include <string>
#include <unordered_map>

#include "core/utils/thread.h"
#include "core/file/contentdevice.h"
#include "core/file/datastream.h"
#include "core/object/serializableobject.h"

namespace solunar
{

class ContentManager : public Singleton<ContentManager>
{
public:
	ContentManager();
	~ContentManager();

	void Init();
	void Shutdown();

	void MountDevice(ContentDevice* contentDevice, const std::string& name);
	void UnmountDevice(const std::string& name);

	const char* GetWorkingPath(const char* pDeviceName);

	// #TODO: remove
	// Little hack for using legacy code
	DataStreamPtr OpenStream(const std::string& path);
	DataStreamPtr OpenStream(const std::string& name, const std::string& path);

	std::weak_ptr<SerializableObject> Load(const std::string& filename, const TypeInfo* pTypeInfo);

	template <typename T>
	std::weak_ptr<T> LoadObject(const std::string& filename);

	void AddExisting(const std::string& filename, std::shared_ptr<SerializableObject> object);
	
	//void loadExisted(const std::string& filename, std::shared_ptr<SerializableObject> object);

	//std::shared_ptr<TextureMap> loadTexture(const std::string& textureName) { return dynamicCastPtr<TextureMap>(Load(textureName, TextureMap::GetStaticTypeInfo())); }
	//std::shared_ptr<ModelBase> loadModelOld(const std::string& modelName) { return dynamicCastPtr<ModelBase>(Load(modelName, ModelBase::GetStaticTypeInfo())); }
	//std::shared_ptr<Material> loadMaterial(const std::string& materialName) { return dynamicCastPtr<Material>(Load(materialName, Material::GetStaticTypeInfo())); }
	//std::shared_ptr<Model> loadModel(const std::string& modelName) { return dynamicCastPtr<Model>(Load(modelName, Model::GetStaticTypeInfo())); }

private:
	ContentDevice* findContentDevice(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<SerializableObject>> m_content;
	std::unordered_map<std::string, ContentDevice*> m_devices;

};

extern ContentManager* g_contentManager;

template<typename T>
inline std::weak_ptr<T> ContentManager::LoadObject(const std::string& filename)
{
	return dynamicCastPtr<T>(Load(filename, T::GetStaticTypeInfo()));
}

}

#endif // !CONTENTMANAGER_H
