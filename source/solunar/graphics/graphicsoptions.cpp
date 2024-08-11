#include "graphicspch.h"
#include "graphics/graphicsoptions.h"

#include "core/utils/iniFile.h"

namespace solunar
{
	GraphicsOptions g_graphicsOptions;

	bool GraphicsOptions::loadSettings(const std::string& filename)
	{
		CIniFile settingsFile(filename.c_str());
		if (!settingsFile.ReadFile())
			return false;

		m_width = settingsFile.GetValueI("DisplaySettings", "Width", 800);
		m_height = settingsFile.GetValueI("DisplaySettings", "Height", 600);
		m_fullscreen = settingsFile.GetValueB("DisplaySettings", "Fullscreen", false);
		m_refreshRate = settingsFile.GetValueI("DisplaySettings", "RefreshRate", 60);
		m_vsync = settingsFile.GetValueI("DisplaySettings", "VSync", 0);

		m_texturesMipMapLevel = settingsFile.GetValueI("GraphicsSettings", "TextureMipMapLevel");

		m_anisotropicQuality = settingsFile.GetValueI("GraphicsSettings", "AnisotropicQuality");
		m_shadowsQuality = settingsFile.GetValueI("GraphicsSettings", "ShadowsQuality");

		return true;
	}

	void GraphicsOptions::saveSettings(const std::string& filename)
	{
		CIniFile settingsFile(filename.c_str());
		if (settingsFile.ReadFile())
			settingsFile.Erase();

		settingsFile.SetValueI("DisplaySettings", "Width", m_width);
		settingsFile.SetValueI("DisplaySettings", "Height", m_height);
		settingsFile.SetValueB("DisplaySettings", "Fullscreen", m_fullscreen);
		settingsFile.SetValueI("DisplaySettings", "RefreshRate", m_refreshRate);
		settingsFile.SetValueI("DisplaySettings", "VSync", m_vsync);

		settingsFile.SetValueI("GraphicsSettings", "TextureMipMapLevel", m_texturesMipMapLevel);
		settingsFile.KeyComment("TextureMipMapLevel", "From 0 (higher) to 4 (low)");

		settingsFile.SetValueI("GraphicsSettings", "AnisotropicQuality", m_anisotropicQuality);
		settingsFile.SetValueI("GraphicsSettings", "ShadowsQuality", m_shadowsQuality);

		settingsFile.WriteFile();
	}

	void GraphicsOptions::applyDefaultOptions()
	{
		// GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		// const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

		m_width = 1280;
		m_height = 720;
		m_refreshRate = 60;
		m_fullscreen = false;
		m_vsync = 0;

		m_texturesMipMapLevel = 0;

		m_anisotropicQuality = 4;
		m_shadowsQuality = 3;
		
	}

}