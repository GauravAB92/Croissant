#pragma once
#include <render/backend/DeviceManager.h>


class ApplicationBase : public IRenderPass
{
public:
	typedef IRenderPass Super;

	ApplicationBase(DeviceManager* deviceManager);
	virtual void Render(nvrhi::IFramebuffer* framebuffer) override;
	virtual void RenderScene(nvrhi::IFramebuffer* framebuffer);
    virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) override
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
         //   m_ui.ShowUI = !m_ui.ShowUI;
            return true;
        }

        if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
        {
          //  m_ui.ShowConsole = !m_ui.ShowConsole;
            return true;
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
          //  m_ui.EnableAnimations = !m_ui.EnableAnimations;
            return true;
        }

        if (key == GLFW_KEY_T && action == GLFW_PRESS)
        {
         
            return true;
        }

        if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
        {
			m_RecompileShaders = true;
            return true;
        }

        return true;
    }
    virtual bool MousePosUpdate(double xpos, double ypos) override
    {
       // if (m_ui.MouseOverUI)
       //     return false;

      //  if (!m_ui.ActiveSceneCamera)
       //     GetActiveCamera().MousePosUpdate(xpos, ypos);

     //   m_PickPosition = uint2(static_cast<uint>(xpos), static_cast<uint>(ypos));

        return true;
    }
    virtual bool MouseButtonUpdate(int button, int action, int mods) override
    {
       // if (m_ui.MouseOverUI)
       //     return false;

      //  if (!m_ui.ActiveSceneCamera)
      //      GetActiveCamera().MouseButtonUpdate(button, action, mods);

     //   if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_2)
      //      m_Pick = true;

        return true;
    }
    virtual bool MouseScrollUpdate(double xoffset, double yoffset) override
    {
       // if (m_ui.MouseOverUI)
        //    return false;

       // if (!m_ui.ActiveSceneCamera)
        //    GetActiveCamera().MouseScrollUpdate(xoffset, yoffset);

        return true;
    }
protected:
    bool m_RecompileShaders;
};


