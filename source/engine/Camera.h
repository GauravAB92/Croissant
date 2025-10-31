#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <GLFW/glfw3.h>
#include <array>
#include <Windows.h>

// A camera with position and orientation. Methods for moving it come from derived classes.
class BaseCamera
{
public:
    virtual void KeyboardUpdate(int key, int scancode, int action, int mods) {}
    virtual void MousePosUpdate(double xpos, double ypos) {}
    virtual void MouseButtonUpdate(int button, int action, int mods) {}
    virtual void MouseScrollUpdate(double xoffset, double yoffset) {}
    virtual void JoystickButtonUpdate(int button, bool pressed) {}
    virtual void JoystickUpdate(int axis, float value) {}
    virtual void Animate(float deltaT) {}
    virtual ~BaseCamera() = default;

    void SetMoveSpeed(float value) { m_MoveSpeed = value; }
    void SetRotateSpeed(float value) { m_RotateSpeed = value; }

    [[nodiscard]] const glm::mat4& GetWorldToViewMatrix() const { return m_MatWorldToView; }
    [[nodiscard]] const glm::mat4& GetTranslatedWorldToViewMatrix() const { return m_MatTranslatedWorldToView; }
    [[nodiscard]] const glm::vec3& GetPosition() const { return m_CameraPos; }
    [[nodiscard]] const glm::vec3& GetDir() const { return m_CameraDir; }
    [[nodiscard]] const glm::vec3& GetUp() const { return m_CameraUp; }

protected:
    // This can be useful for derived classes while not necessarily public, i.e., in a third person
    // camera class, public clients cannot direct the gaze point.
    void BaseLookAt(glm::vec3 cameraPos, glm::vec3 cameraTarget, glm::vec3 cameraUp = glm::vec3{ 0.f, 1.f, 0.f });
    void UpdateWorldToView();

    glm::mat4 m_MatWorldToView = glm::mat4(1.0f);
    glm::mat4 m_MatTranslatedWorldToView = glm::mat4(1.0f);

    glm::vec3 m_CameraPos   = glm::vec3(0.f, 0.f, 0.f); // in worldspace
    glm::vec3 m_CameraDir   = glm::vec3(1.f, 0.f, 0.f); // normalized
    glm::vec3 m_CameraUp    = glm::vec3(0.f, 1.f, 0.f); // normalized
    glm::vec3 m_CameraRight = glm::vec3(0.f, 0.f, 1.f); // normalized

    float m_MoveSpeed   = 1.f;      // movement speed in units/second
    float m_RotateSpeed = .001f;  // mouse sensitivity in radians/pixel
};


class ThirdPersonCamera : public BaseCamera
{
public:
    //override keyboard and mouse callbacks
	void KeyboardUpdate(int key, int scancode, int action, int mods) override;
	void MousePosUpdate(double xpos, double ypos) override;
	void MouseButtonUpdate(int button, int action, int mods) override;
	void MouseScrollUpdate(double xoffset, double yoffset) override;
	void JoystickButtonUpdate(int button, bool pressed) override;
	void JoystickUpdate(int axis, float value) override;
	void Animate(float deltaT) override;
	
	//getter setters for camera parameters
	glm::vec3 GetTargetPos() const { return m_TargetPos; }
	void SetTargetPos(const glm::vec3& targetPos) { m_TargetPos = targetPos; }
	
    void SetDistance(float distance) { m_Distance = distance; }
	float GetDistance() const { return m_Distance; }

	float GetMinDistance() const { return m_MinDistance; }
    void SetMinDistance(float minDistance) { m_MinDistance = minDistance; }
	
    float GetMaxDistance() const { return m_MaxDistance; }
	void SetMaxDistance(float maxDistance) { m_MaxDistance = maxDistance; }
	
	void SetYaw(float yaw) { m_Yaw = yaw; }
	float GetYaw() const { return m_Yaw; }
	
    void SetPitch(float pitch) { m_Pitch = pitch; }
	float GetPitch() const { return m_Pitch; }

    void SetRotation(float yaw, float pitch);
    void GetRotation(float& yaw, float& pitch) const;
	
	void SetViewport(const glm::vec2& viewportSize)
	{
		m_ViewportSize = viewportSize * m_ViewportScale;
		m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);
	}

    glm::mat4 GetInverseProjectionMatrix() const
    {
		return m_InverseProjectionMatrix;
    }

    glm::mat4& GetInverseProjectionMatrix() 
    {
        return m_InverseProjectionMatrix;
    }

    glm::vec3 GetCameraRight() const
    {
		return m_CameraRight;
    }

	void SetProjectionMatrix(const glm::mat4& projectionMatrix)
	{
		m_ProjectionMatrix = projectionMatrix;
		m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);
	}

	glm::mat4 GetProjectionMatrix() const { return  m_JitterTranslationMatrix * m_ProjectionMatrix; }

    glm::vec2 GetViewportScale()
    {
		return m_ViewportScale;
    }

    void SetViewportScale(glm::vec2 scale)
    {
        m_ViewportScale = scale;
    }

	void SetJitterTranslationMatrix(const glm::mat4& jitterTranslationMatrix)
	{
		m_JitterTranslationMatrix = jitterTranslationMatrix;
	}

    void LookAt(glm::vec3 cameraPos, glm::vec3 cameraTarget, glm::vec3 cameraUp = glm::vec3{ 0.f, 1.f, 0.f });
    void LookTo(glm::vec3 cameraPos, glm::vec3 cameraDir, std::optional<float> targetDistance = std::optional<float>());


private:
    void AnimateOrbit(float deltaT);
    void AnimateTranslation(const glm::mat3& viewMatrix);

    // View parameters to derive translation amounts
    glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_InverseProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 m_JitterTranslationMatrix = glm::mat4(1.0f);

    glm::vec2 m_ViewportSize = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_ViewportScale = glm::vec2(1.0f, 1.0f);
    glm::vec2 m_MousePos = glm::vec2(0.f,0.f);
    glm::vec2 m_MousePosPrev = glm::vec2(0.f, 0.f);

    glm::vec3 m_TargetPos = glm::vec3(0.f, 0.f, 0.f);
    float m_Distance = 30.f;

    float m_MinDistance = 0.f;
    float m_MaxDistance = std::numeric_limits<float>::max();

    float m_Yaw = 0.f;
    float m_Pitch = 0.0f;

    float m_DeltaYaw = 0.f;
    float m_DeltaPitch = 0.f;
    float m_DeltaDistance = 0.f;

    typedef enum
    {
        HorizontalPan,
        VerticalShiftDown,
        VerticalShiftUp,
        KeyboardControlCount,
    } KeyboardControls;

    const std::unordered_map<int, int> keyboardMap = {
        { GLFW_KEY_LEFT_ALT,    KeyboardControls::HorizontalPan },
		{ GLFW_KEY_PAGE_DOWN,   KeyboardControls::VerticalShiftDown },
        { GLFW_KEY_PAGE_UP,     KeyboardControls::VerticalShiftUp },

    };

    typedef enum
    {
        Left,
        Middle,
        Right,

        MouseButtonCount
    } MouseButtons;

    std::array<bool, KeyboardControls::KeyboardControlCount>    keyboardState = { false };            
    std::array<bool, MouseButtons::MouseButtonCount>            mouseButtonState = { false };
};