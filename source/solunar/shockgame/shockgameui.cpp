#include "shockgamepch.h"
#include "shockgame/shockgameui.h"

#include "core/file/contentmanager.h"

#include "engine/engine.h"
#include "engine/camera.h"

#include "engine/inputmanager.h"

#include "engine/audio/audiomanager.h"
#include "engine/audio/musicmanager.h"

#include "graphics/texturemap.h"
#include "graphics/imguimanager.h"
#include "graphics/graphicsoptions.h"

#include "main/main.h"

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

namespace solunar
{

static std::weak_ptr<TextureMap> g_mainMenuBackground;

IMPLEMENT_OBJECT(ShockGameMainMenuComponent, LogicComponent);

ShockGameMainMenuComponent::ShockGameMainMenuComponent()
{
}

ShockGameMainMenuComponent::~ShockGameMainMenuComponent()
{
}

void ShockGameMainMenuComponent::OnWorldSet(World* world)
{
	Component::OnWorldSet(world);
	 
	g_mainMenuBackground = g_contentManager->LoadObject<TextureMap>("textures/ui/ui_menu_background.png");

	MusicManager::GetInstance()->Play("sounds/music/temp_mainmenu.mp3", true);
}

void ShockGameMainMenuComponent::Update(float dt)
{
	// begin menu window

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(io.DisplaySize);
	if (ImGui::Begin("Main Menu Background", (bool*)true, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs))
	{
		ImGui::Image(g_mainMenuBackground.lock()->getHWTexture(), io.DisplaySize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::End();
	}

	CameraProxy* camera = CameraProxy::GetInstance();

	ImGui::SetNextWindowPos(ImVec2(100, 550));
	ImGui::SetNextWindowSize(ImVec2(camera->GetView()->m_width / 2, camera->GetView()->m_height / 2));

	ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

	const ImVec2 kButtonSize = ImVec2(128.0f, 32.0f);

	if (ImGui::Button("New Game", kButtonSize))
	{
	//	loadLevel("env_test");

		MusicManager::GetInstance()->Stop();
		EngineStateManager::GetInstance()->LoadWorld("worlds/env_test.xml");
	}

	if (ImGui::Button("Quit", kButtonSize))
	{
		//glfwSetWindowShouldClose(g_engineWindow, GLFW_TRUE);
		appFireExit();
	}

	ImGui::End();
}

/////////////////////////////////////////////////////////////////////
// DEMO GAME

class VideoModeManager
{
public:
	struct VideoMode
	{
		std::string name;
		int width, height, refreshRate;
	};
public:
	static bool ms_inited;
	static int ms_currentVidMode;
	static std::vector<VideoMode> ms_vidModes;
public:
	static void Init();
	static void Apply(int idx);
};

bool												VideoModeManager::ms_inited = false;
int													VideoModeManager::ms_currentVidMode = 0;
std::vector<VideoModeManager::VideoMode>			VideoModeManager::ms_vidModes;

void VideoModeManager::Init()
{
	IDXGIFactory* pFactory = nullptr;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

	SIZE_T maxDedicatedVideoMemory = 0;
	SIZE_T adapterId = 0;
	IDXGIAdapter* pAdapter = nullptr;
	for (int i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		pAdapter->GetDesc(&adapterDesc);

		if (adapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
			break;
		}
	}

	if (pAdapter)
		pAdapter->Release();

	pFactory->EnumAdapters(adapterId, &pAdapter);

	IDXGIOutput* pOutput = NULL;
	HRESULT hr = pAdapter->EnumOutputs(0, &pOutput);

	UINT numModes = 0;
	DXGI_MODE_DESC* displayModes = NULL;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Get the number of elements
	hr = pOutput->GetDisplayModeList(format, 0, &numModes, NULL);

	displayModes = new DXGI_MODE_DESC[numModes];

	// Get the list
	hr = pOutput->GetDisplayModeList(format, 0, &numModes, displayModes);

	for (int i = 0; i < numModes; i++)
	{
#if 0
		if (displayModes[i].RefreshRate.Numerator > 60)
			continue;
#else
		if (displayModes[i].RefreshRate.Numerator != 60)
			continue;
#endif

		VideoMode videoMode;
		videoMode.width = displayModes[i].Width;
		videoMode.height = displayModes[i].Height;
		videoMode.refreshRate = displayModes[i].RefreshRate.Numerator;

		char buf[64];
		snprintf(buf, sizeof(buf), "%ix%i (%i)", videoMode.width, videoMode.height, videoMode.refreshRate);
		videoMode.name = buf;

		ms_vidModes.push_back(videoMode);
	}

	// delete allocated display mode list
	delete[] displayModes;

	// release dxgi stuff
	pOutput->Release();
	pAdapter->Release();
	pFactory->Release();

#if 0
	VideoMode videoMode;
	videoMode.width = g_graphicsOptions.m_width;
	videoMode.height = g_graphicsOptions.m_height;
	videoMode.refreshRate = g_graphicsOptions.m_refreshRate;
	
	char buf[64];
	snprintf(buf, sizeof(buf), "%ix%i (%i)", videoMode.width, videoMode.height, videoMode.refreshRate);
	videoMode.name = buf;

	ms_vidModes.push_back(videoMode);
#endif

	//GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	//const GLFWvidmode* videoModes = glfwGetVideoModes(primaryMonitor, &ms_vidModeCount);

	//for (int i = 0; i < ms_vidModeCount; i++)
	//{
	//	spdlog::info("{} {} {}", videoModes[i].width, videoModes[i].height, videoModes[i].refreshRate);

	//	int refresh = videoModes[i].refreshRate;
	//	if (refresh != 60 && refresh != 70 && refresh != 75)
	//		continue;

	//	const GLFWvidmode& vidMode = videoModes[i];

	//	VideoMode mode;
	//	mode.width = vidMode.width;
	//	mode.height = vidMode.height;
	//	mode.refreshRate = vidMode.refreshRate;

	//	char buffer[256];
	//	sprintf(buffer, "%ix%i %i", mode.width, mode.height, mode.refreshRate);
	//	mode.name = buffer;

	//	ms_vidModes.push_back(mode);
	//}


	for (int i = 0; i < ms_vidModes.size(); i++)
	{
		GraphicsOptions* opts = &g_graphicsOptions;

		if (opts->m_width == ms_vidModes[i].width &&
			opts->m_height == ms_vidModes[i].height &&
			opts->m_refreshRate == ms_vidModes[i].refreshRate)
		{
			ms_currentVidMode = i;
			break;
		}
	}

	ms_inited = true;
}

void VideoModeManager::Apply(int idx)
{
	if (ms_vidModes.empty())
		return;

	VideoMode mode = ms_vidModes[idx];
	GraphicsOptions* opts = &g_graphicsOptions;
	opts->m_width = mode.width;
	opts->m_height = mode.height;
	opts->m_refreshRate == mode.refreshRate;
	g_graphicsOptions.SaveSettings("GameSettings.ini");
}

static bool s_showSettingsMenu = false;

void showSettingsMenu()
{
	static bool s_showApplyWindow = false;

	ImGui::Begin("Settings", &s_showSettingsMenu);

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Video"))
		{
			if (!VideoModeManager::ms_inited)
				VideoModeManager::Init();

			static int item_current_idx = VideoModeManager::ms_currentVidMode; // Here we store our selection data as an index.

			if (!VideoModeManager::ms_vidModes.empty())
			{
				const char* combo_preview_value = VideoModeManager::ms_vidModes[item_current_idx].name.c_str();
				if (ImGui::BeginCombo("Video Mode", combo_preview_value))
				{
					for (int i = 0; i < VideoModeManager::ms_vidModes.size(); i++)
					{
						const bool is_selected = (item_current_idx == i);
						if (ImGui::Selectable(VideoModeManager::ms_vidModes[i].name.c_str(), is_selected))
							item_current_idx = i;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
			}

			ImGui::EndTabItem();

			if (ImGui::Button("Apply"))
			{
				VideoModeManager::Apply(item_current_idx);
				s_showApplyWindow = true;
			}
		}
		if (ImGui::BeginTabItem("Graphics"))
		{
			ImGui::SliderInt("Texture Quality", &g_graphicsOptions.m_texturesMipMapLevel, 0, 4);
			ImGui::SliderInt("Anisotropic Quality", &g_graphicsOptions.m_anisotropicQuality, 0, 16);
			ImGui::SliderInt("Shadows Quality", &g_graphicsOptions.m_shadowsQuality, 0, 4);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Sound"))
		{
			ImGui::SliderFloat("SFX Volume", &g_sfxChannelVolume, 0.0f, 1.0f);
			ImGui::SliderFloat("Music Volume", &g_musicChannelVolume, 0.0f, 1.0f);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Input"))
		{
			InputManager* pIM = InputManager::GetInstance();
			ImGui::SliderFloat("Mouse Sensitivity", &pIM->m_mouseSensitivy, 0.1f, 10.0f);
			ImGui::Checkbox("Mouse Invert", &pIM->m_mouseInvert);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();

	if (s_showApplyWindow)
	{
		ImGui::Begin("Warning!", &s_showApplyWindow);
		ImGui::Text("To apply the settings,\nyou need to restart the application.");
		if (ImGui::Button("OK"))
			s_showApplyWindow = false;
		ImGui::End();
	}
}

IMPLEMENT_OBJECT(DemoGameMainMenuComponent, LogicComponent);

DemoGameMainMenuComponent::DemoGameMainMenuComponent()
{
}

DemoGameMainMenuComponent::~DemoGameMainMenuComponent()
{
}

void DemoGameMainMenuComponent::OnWorldSet(World* world)
{
	Component::OnWorldSet(world);

	Assert(g_engineData.m_editor == false && "you can't use this in editor mode!");
	g_mainMenuBackground = g_contentManager->LoadObject<TextureMap>("textures/ui/ui_menu_background.png");

	//MusicManager::GetInstance()->play("sounds/music/temp_mainmenu.mp3", true);
}

void DemoGameMainMenuComponent::Update(float dt)
{
	// begin menu window

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(io.DisplaySize);
	if (ImGui::Begin("Main Menu Background", (bool*)true, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs))
	{
		ImGui::Image(g_mainMenuBackground.lock()->getHWTexture(), io.DisplaySize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::End();
	}

	CameraProxy* camera = CameraProxy::GetInstance();

	int posX = (camera->GetView()->m_width / 2) - 550;
	int posY = (camera->GetView()->m_height / 2) + 150;

	ImGui::SetNextWindowPos(ImVec2(posX, posY));
	ImGui::SetNextWindowSize(ImVec2(camera->GetView()->m_width / 2, camera->GetView()->m_height / 2));

	ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

	const ImVec2 kButtonSize = ImVec2(128.0f, 32.0f);

	if (ImGui::Button("New Game", kButtonSize))
	{
		MusicManager::GetInstance()->Stop();
		EngineStateManager::GetInstance()->LoadWorld("worlds/zapravka.xml");
	}

	if (ImGui::Button("Settings", kButtonSize))
	{
		s_showSettingsMenu = true;
	}

	if (ImGui::Button("Quit", kButtonSize))
	{
		appFireExit();
	}

	ImGui::End();

	if (s_showSettingsMenu)
		showSettingsMenu();
}


}
