#include "enginepch.h"
#include "engine/audio/audiomanager.h"

namespace solunar
{
	float g_sfxChannelVolume = 1.0f;
	float g_musicChannelVolume = 1.0f;

	AudioManager* AudioManager::ms_pInstance = nullptr;

	AudioManager::AudioManager()
	{

	}

	AudioManager::~AudioManager()
	{

	}

	void AudioManager::Init()
	{

	}

	void AudioManager::Shutdown()
	{

	}

	void AudioManager::Update()
	{

	}

	AudioSource* AudioManager::CreateSource(const std::string& filename)
	{
		return nullptr;
	}

	void AudioManager::DeleteSource(AudioSource* source)
	{

	}

	void AudioManager::SetListenerPosition(const glm::vec3& position)
	{
	}

}
