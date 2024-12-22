#include "enginepch.h"
#include "engine/camera.h"
#include "engine/entity/entity.h"
#include "engine/entity/cameracomponent.h"

namespace solunar
{
	CameraProxy CameraProxy::ms_cameraProxy;

	void Camera::Update()
	{
		m_view->m_view = getViewMatrix();
		m_view->updateInternalValues();

		glm::mat4 viewProjection = m_view->m_projection * m_view->m_view;
		m_frustum.Update(viewProjection);
	}
	// returns dir
	glm::vec3 Camera::GetScreenRay(float x, float y)
	{
		if (getViewMatrix() != m_view->m_view)
			UpdateInternal();

		float mouse_x = x / (m_view->m_width * 0.5f) - 1.0f;
		float mouse_y = y / (m_view->m_height * 0.5f) - 1.0f;

		glm::mat4 ivp = glm::inverse(m_view->m_projection * m_view->m_view);
		glm::vec4 screen_pos(mouse_x, -mouse_y, 1.0f, 1.0f);

		glm::vec4 world_pos = ivp * screen_pos;

		return glm::normalize(glm::vec3(world_pos));
	}

	void Camera::UpdateInternal()
	{
		m_view->m_view = getViewMatrix();
		m_view->updateInternalValues();

		glm::mat4 viewProjection = m_view->m_projection * m_view->m_view;
		m_frustum.Update(viewProjection);
	}

	glm::mat4 Camera::getViewMatrix()
	{
		return glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void CameraProxy::UpdateProxy()
	{
		//	OPTICK_EVENT("CameraProxy::UpdateProxy");

		if (m_cameraComponent)
		{
			// update direction and position of camera based on component and node info
			m_direction = m_cameraComponent->GetDirection();

			Assert(m_cameraComponent->GetEntity());
			Entity* entity = m_cameraComponent->GetEntity();

			m_position = entity->GetWorldPosition();

			// update internal values
			UpdateInternal();
		}
	}

	void CameraProxy::SetCameraComponent(CameraComponent* cameraComp)
	{
		Assert(cameraComp);
		m_cameraComponent = cameraComp;
	}

	CameraComponent* CameraProxy::GetCameraComponent()
	{
		return m_cameraComponent;
	}

}