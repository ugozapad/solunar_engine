#include "fmoddrv_pch.h"

#include "core/file/filesystem.h"

#include "engine/audio/musicmanager.h"

#include "fmoddrv/audiomanagerfmod.h"
#include "fmoddrv/audiosourcefmod.h"

#include <fmod_errors.h>

// #HACK
#if 0
#ifndef NDEBUG
#pragma comment(lib, "fmodstudioL_vc.lib")
#pragma comment(lib, "fmodL_vc.lib")
#else
#pragma comment(lib, "fmodstudio_vc.lib")
#pragma comment(lib, "fmod_vc.lib")
#endif // !NDEBUG
#endif

namespace solunar
{

	static FMOD_RESULT FMOD_LoggerCallback(
		FMOD_DEBUG_FLAGS flags,
		const char* file,
		int line,
		const char* func,
		const char* message
	)
	{
		std::string messageString = message;
		messageString[messageString.size() - 1] = ' ';

		Core::Msg("[fmod]: %s: %s", func, messageString.c_str());

		return FMOD_OK;
	}

	AudioManager* AudioManager::CreateInstance()
	{
		if (!ms_pInstance)
			ms_pInstance = mem_new<AudioManagerFMOD>();

		return ms_pInstance;
	}

	void AudioManager::DestroyInstance()
	{
		if (ms_pInstance)
		{
			mem_delete(ms_pInstance);
			ms_pInstance = nullptr;
		}
	}

	AudioManagerFMOD::AudioManagerFMOD() :
		m_studioSystem(nullptr),
		m_sfxChannelGroup(nullptr)
	{
	}

	AudioManagerFMOD::~AudioManagerFMOD()
	{
	}

	void AudioManagerFMOD::Init()
	{
#ifndef NDEBUG
		FMOD::Debug_Initialize(FMOD_DEBUG_LEVEL_LOG, FMOD_DEBUG_MODE_CALLBACK, &FMOD_LoggerCallback, nullptr);
#endif // !NDEBUG

		// Create the Studio System object.
		FMOD_RESULT result = FMOD::Studio::System::create(&m_studioSystem);
		if (result != FMOD_OK)
		{
			Core::Error("Failed to create FMOD Studio. Error code:%s", FMOD_ErrorString(result));
		}

		// Initialize FMOD Studio, which will also Initialize FMOD Core
		result = m_studioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
		if (result != FMOD_OK)
		{
			Core::Error("Failed to Initialize FMOD Studio. Error code:%s", FMOD_ErrorString(result));
		}

		Core::Msg("[audio]: fmod initialized");

		// Initialize music manager.
		MusicManager::GetInstance()->Init();

		// Create SFX channel group
		GetFMODSystem()->createChannelGroup( "SFX", &m_sfxChannelGroup );
	}

	void AudioManagerFMOD::Shutdown()
	{
		if (m_sfxChannelGroup)
		{
			m_sfxChannelGroup->release();
			m_sfxChannelGroup = nullptr;
		}

		MusicManager::GetInstance()->Shutdown();

		// clear sounds
		for (int i = 0; i < m_audioSources.size(); i++)
		{
			if (m_audioSources[i])
			{
				mem_delete(m_audioSources[i]);
			}
		}

		m_audioSources.clear();

		//m_fmodSystem->close();
		//m_fmodSystem->release();
		//m_fmodSystem = nullptr;
	}

	void AudioManagerFMOD::Update()
	{
		m_sfxChannelGroup->setVolume(g_sfxChannelVolume);
		m_singletonStorer.m_musicManager.GetMusicChannel()->setVolume(g_musicChannelVolume);
	}

	AudioSource* AudioManagerFMOD::CreateSource(const std::string& filename)
	{
		AudioSource* pAudioSource = (AudioSource*)mem_new<AudioSourceFMOD>(filename, GetFMODSystem());
		m_audioSources.push_back(pAudioSource);
		return pAudioSource;
	}

	void AudioManagerFMOD::DeleteSource(AudioSource* source)
	{
		if (source)
		{
			auto it = std::find(m_audioSources.begin(), m_audioSources.end(), source);
			if (it != m_audioSources.end())
				m_audioSources.erase(it);

			if (source->IsPlaying())
				source->Stop();

			mem_delete(source);
		}
	}

	void AudioManagerFMOD::SetListenerPosition(const glm::vec3& position)
	{
		FMOD_VECTOR listenerPosition = { position.x, position.y, position.z };
		GetFMODSystem()->set3DListenerAttributes(0, &listenerPosition, NULL, NULL, NULL);
		//m_sfxChannelGroup->set3d(0, &listenerPosition, NULL, NULL, NULL);
	}

	FMOD::System* AudioManagerFMOD::GetFMODSystem()
	{
		FMOD::System* coreSystem = nullptr;
		m_studioSystem->getCoreSystem(&coreSystem);

		Assert(coreSystem);

		return coreSystem;
	}
}
