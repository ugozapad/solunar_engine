#include "enginepch.h"
#include "engine/entity/cameracomponent.h"
#include "engine/inputmanager.h"
namespace solunar
{

IMPLEMENT_OBJECT(CameraComponent, Component);

CameraComponent::CameraComponent()
{
	m_direction = glm::vec3(0.0f, 0.0f, -1.0f);
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::SetDirection(const glm::vec3& direction)
{
	m_direction = direction;
}

glm::vec3 CameraComponent::GetDirection()
{
	return m_direction;
}


////////////////////////
// Yaw pitch roll camera

IMPLEMENT_OBJECT(CameraYawPitchRollComponent, CameraComponent);

CameraYawPitchRollComponent::CameraYawPitchRollComponent()
{
	m_yaw = 0.0f;
	m_pitch = 0.0f;
	m_roll = 0.0f;
}

CameraYawPitchRollComponent::~CameraYawPitchRollComponent()
{
}

void CameraYawPitchRollComponent::updateFromMousePosition(const glm::vec2& mousePos)
{
	Assert(0);
}

void CameraYawPitchRollComponent::SetEulerRotation(float yaw, float pitch, float roll)
{
	m_yaw = yaw;
	m_pitch = pitch;
	m_roll = roll;
}

//////////////////////
// First Person camera

IMPLEMENT_OBJECT(CameraFirstPersonComponent, CameraYawPitchRollComponent);

CameraFirstPersonComponent::CameraFirstPersonComponent()
{
}

CameraFirstPersonComponent::~CameraFirstPersonComponent()
{
}

float wrapBetween(float value, float min, float max) {
	// Algorithm from http://stackoverflow.com/a/5852628/599884

	if (min > max) {
		// Swap min and max
		float temp = min;
		min = max;
		max = temp;
	}

	float range = max - min;
	if (range == 0) {
		return max;
	}

	return (float)(value - range * floor((value - min) / range));
}

void CameraFirstPersonComponent::updateFromMousePosition(const glm::vec2& mousePos)
{
	float xoffset = mousePos.x;
	float yoffset = mousePos.y;

	xoffset *= (InputManager::GetInstance()->m_mouseSensitivy / 10.0f);
	yoffset *= (InputManager::GetInstance()->m_mouseSensitivy / 10.0f);

	m_yaw += xoffset;
	m_pitch += yoffset;

	m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

	m_yaw = wrapBetween(m_yaw, -180.0f, 180.0f);
	m_pitch = wrapBetween(m_pitch, -180.0f, 180.0f);

	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_direction = glm::normalize(front);

	//m_yaw +=  mousePos.x;
	//m_pitch += mousePos.y;
	//m_pitch = glm::clamp(m_pitch, -90.0f, 90.0f);

//	getEntity()->setEulerRotation(glm::vec3(m_pitch, m_yaw, 0.0f));

	////m_node->setRotation(glm::quat(glm::vec3(m_pitch, m_yaw, 0.0f)));
	//m_node->setEulerRotation(glm::vec3(m_pitch, m_yaw, 0.0f));
	//m_direction = m_node->getEulerRotation();
}

}