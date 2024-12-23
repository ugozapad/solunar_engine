#include "enginepch.h"
#include "engine/inputmanager.h"

namespace solunar
{

InputManager::InputManager() :
	m_cursorPos(0.0f),
	m_lastCursorPos(0.0f),
	m_deltaCursorPos(0.0f),
	m_captureCursor(false),
	m_hideCursor(false),
	m_lastCharactedPressed(0)
{
	memset(m_keys, 0, sizeof(m_keys));
}

InputManager::~InputManager()
{
}

void InputManager::Update()
{
	m_lastCharactedPressed = 0;

	m_mouseButtons[MouseButtons::MOUSE_BUTTON_LEFT] = false;

	//memset(m_mouseButtons, 0, sizeof(m_mouseButtons));
}

bool InputManager::IsPressed(uint32_t key)
{
	Assert(key <= KEY_COUNT);
	return m_keys[key];
}

bool InputManager::IsPressedWithReset(uint32_t key)
{
	Assert(key <= KEY_COUNT);

	if (m_keys[key])
	{
		m_keys[key] = false;
		return true;
	}

	return false;
}

bool InputManager::IsMouseButtonPressed(MouseButtons buttonId)
{
	Assert(buttonId <= MOUSE_BUTTON_COUNT);
	return m_mouseButtons[buttonId];
}

void InputManager::SetCursorCapture(bool capture)
{
	m_captureCursor = capture;
}

void InputManager::SetCursorHiding(bool hide)
{
	m_hideCursor = hide;
}

void InputManager::ResetDelta()
{
	m_deltaCursorPos.x = 0.0f;
	m_deltaCursorPos.y = 0.0f;
}

}