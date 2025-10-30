#include "Camera.h"
#include <glm/glm/gtc/quaternion.hpp>
#include <glm/glm/gtc/constants.hpp>

//captures continuous keyboard input and mouse movement

void BaseCamera::UpdateWorldToView()
{
	//Make sure orthonormal
	glm::vec3 R = m_CameraRight;
	glm::vec3 U = m_CameraUp;
	glm::vec3 D = m_CameraDir;      // assuming this points *forward*

	glm::mat4 view(
		glm::vec4(R.x, U.x, D.x, 0.0f),
		glm::vec4(R.y, U.y, D.y, 0.0f),
		glm::vec4(R.z, U.z, D.z, 0.0f),
		glm::vec4(-glm::dot(R, m_CameraPos),
			-glm::dot(U, m_CameraPos),
			-glm::dot(D, m_CameraPos),
			1.0f)
	);

	m_MatWorldToView = view;

}


void ThirdPersonCamera::KeyboardUpdate(int key, int scancode, int action, int mods)
{
	if (keyboardMap.find(key) == keyboardMap.end())
	{
		return;
	}

	auto cameraKey = keyboardMap.at(key);
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		keyboardState[cameraKey] = true;
	}
	else {
		keyboardState[cameraKey] = false;
	}
}

void ThirdPersonCamera::MousePosUpdate(double xpos, double ypos)
{
	m_MousePos = glm::vec2(float(-xpos), float(ypos));
}

void ThirdPersonCamera::MouseButtonUpdate(int button, int action, int mods)
{
	const bool pressed = (action == GLFW_PRESS);

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:	    mouseButtonState[MouseButtons::Left] = pressed; break;
	case GLFW_MOUSE_BUTTON_MIDDLE:		mouseButtonState[MouseButtons::Middle] = pressed; break;
	case GLFW_MOUSE_BUTTON_RIGHT:		mouseButtonState[MouseButtons::Right] = pressed; break;
	default: break;
	}
}

void ThirdPersonCamera::MouseScrollUpdate(double xoffset, double yoffset)
{
	const float scrollFactor = 1.15f;
	m_Distance = glm::clamp(m_Distance * (yoffset < 0 ? scrollFactor : 1.0f / scrollFactor), m_MinDistance, m_MaxDistance);
}

void ThirdPersonCamera::JoystickButtonUpdate(int button, bool pressed)
{
}

void ThirdPersonCamera::JoystickUpdate(int axis, float value)
{
}

void ThirdPersonCamera::Animate(float deltaT)
{
	AnimateOrbit(deltaT);


	glm::quat orbit = glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f));

	const auto targetRotation = glm::mat3_cast(orbit);
	AnimateTranslation(targetRotation);

	const glm::vec3 vectorToCamera	 = -m_Distance * targetRotation[2];
	const glm::vec3 camPos			 =  m_TargetPos + vectorToCamera;

	m_CameraPos		= camPos;
	m_CameraRight	= targetRotation[0];
	m_CameraUp		= targetRotation[1];
	m_CameraDir		= targetRotation[2];
	UpdateWorldToView();

	m_MousePosPrev = m_MousePos;
}

void ThirdPersonCamera::SetRotation(float yaw, float pitch)
{
	m_Yaw = yaw;
	m_Pitch = pitch;
}

void ThirdPersonCamera::GetRotation(float& yaw, float& pitch) const
{
	yaw = m_Yaw;
	pitch = m_Pitch;
}

void cartesianToSpherical(const glm::vec3 cameraDir, float& azimuth, float& elevation, float& dirLength)
{
	dirLength = glm::length(cameraDir);

	if (dirLength > 0.0f)
	{
		elevation = glm::asin(cameraDir.y / dirLength);
		azimuth = glm::atan(cameraDir.z, cameraDir.x);
	}
	else
	{
		elevation = 0.0f;
		azimuth = 0.0f;
	}
}

void ThirdPersonCamera::LookAt(glm::vec3 cameraPos, glm::vec3 cameraTarget, glm::vec3 cameraUp)
{
	glm::vec3 cameraDir = cameraTarget - cameraPos;

	float azimuth, elevation, dirLength;
	cartesianToSpherical(cameraDir, azimuth, elevation, dirLength);

	SetTargetPos(cameraTarget);
	SetDistance(dirLength);
	azimuth = -(azimuth + glm::pi<float>() * 0.5f);
	SetRotation(azimuth, elevation);
}

void ThirdPersonCamera::LookTo(glm::vec3 cameraPos, glm::vec3 cameraDir, std::optional<float> targetDistance)
{
	float azimuth, elevation, dirLength;
	cartesianToSpherical(-cameraDir, azimuth, elevation, dirLength);
	cameraDir /= dirLength;

	float const distance = targetDistance.value_or(GetDistance());
	SetTargetPos(cameraPos + cameraDir * distance);
	SetDistance(distance);
	azimuth = -(azimuth + glm::pi<float>() * 0.5f);
	SetRotation(azimuth, elevation);
}

void ThirdPersonCamera::AnimateOrbit(float deltaT)
{
	if (mouseButtonState[MouseButtons::Right])
	{
		glm::vec2 mouseMove = m_MousePos - m_MousePosPrev;
		float rotateSpeed = m_RotateSpeed;

		m_Yaw -= rotateSpeed * mouseMove.x;
		m_Pitch += rotateSpeed * mouseMove.y;
	}

	

	const float ORBIT_SENSITIVITY = 1.5f;
	const float ZOOM_SENSITIVITY = 40.f;
	m_Distance += ZOOM_SENSITIVITY * deltaT * m_DeltaDistance;
	m_Yaw += ORBIT_SENSITIVITY * deltaT * m_DeltaYaw;
	m_Pitch += ORBIT_SENSITIVITY * deltaT * m_DeltaPitch;

	m_Distance = glm::clamp(m_Distance, m_MinDistance, m_MaxDistance);

	m_Pitch = glm::clamp(m_Pitch, glm::pi<float>() * -0.5f, glm::pi<float>() * 0.5f);

	m_DeltaDistance = 0;
	m_DeltaYaw = 0;
	m_DeltaPitch = 0;
}

void ThirdPersonCamera::AnimateTranslation(const glm::mat3& viewMatrix)
{
	// If the view parameters have never been set, we can't translate
	if (m_ViewportSize.x <= 0.f || m_ViewportSize.y <= 0.f)
		return;

	if (m_MousePos.x == m_MousePosPrev.x && m_MousePos.y == m_MousePosPrev.y)
		return;

	if (mouseButtonState[MouseButtons::Middle])
	{
	
		glm::vec4 oldClipPos = glm::vec4(0.f, 0.f, m_Distance, 1.f) * m_ProjectionMatrix;
		oldClipPos /= oldClipPos.w;
		oldClipPos.x = 2.f * (m_MousePosPrev.x) / m_ViewportSize.x - 1.f;
		oldClipPos.y = 1.f - 2.f * (m_MousePosPrev.y) / m_ViewportSize.y;
		glm::vec4 newClipPos = oldClipPos;
		newClipPos.x = 2.f * (m_MousePos.x) / m_ViewportSize.x - 1.f;
		newClipPos.y = 1.f - 2.f * (m_MousePos.y) / m_ViewportSize.y;

		glm::vec4 oldViewPos = oldClipPos * m_InverseProjectionMatrix;
		oldViewPos /= oldViewPos.w;
		glm::vec4 newViewPos = newClipPos * m_InverseProjectionMatrix;
		newViewPos /= newViewPos.w;

		glm::vec2 viewMotion = glm::vec2(oldViewPos) - glm::vec2(newViewPos);

		m_TargetPos += viewMotion.x  * viewMatrix[0];

		if (keyboardState[KeyboardControls::HorizontalPan])
		{
			glm::vec3 horizontalForward = glm::vec3(viewMatrix[2].x, 0.f, viewMatrix[2].z);
			float horizontalLength = length(horizontalForward);
			if (horizontalLength == 0.f)
				horizontalForward = glm::vec3(viewMatrix[1].x, 0.f, viewMatrix[1].z);
			horizontalForward = normalize(horizontalForward);
			m_TargetPos -= viewMotion.y * horizontalForward * 1.5f;
		}
		else
			m_TargetPos -= viewMotion.y  * viewMatrix[1];
	}
}

void BaseCamera::BaseLookAt(glm::vec3 cameraPos, glm::vec3 cameraTarget, glm::vec3 cameraUp)
{
	this->m_CameraPos = cameraPos;
	this->m_CameraDir = normalize(cameraTarget - cameraPos);
	this->m_CameraUp = normalize(cameraUp);
	this->m_CameraRight = normalize(cross(this->m_CameraDir, this->m_CameraUp));
	this->m_CameraUp = normalize(cross(this->m_CameraRight, this->m_CameraDir));

	UpdateWorldToView();
}
